#include "Global.h"
#include "PortfolioUtils.h"
#include "TradePayment.h"
#include "TradeFXForward.h"

#include <cmath>
#include <set>
#include <exception>

namespace minirisk {
namespace {
void bump_risk_factors(const double bump_size, 
    std::vector<std::pair<std::string, double>>* bumped_up, 
    std::vector<std::pair<std::string, double>>* bumped_dn) {
  for (auto& p : *bumped_up) {
    p.second += bump_size;
  }
  for (auto& p : *bumped_dn) {
    p.second -= bump_size;
  }
}

void find_all_risk_ccy(
    const std::vector<std::pair<std::string, double>>& risk_factors, 
    std::vector<std::string> *ccys) {
  std::set<std::string> risk_ccys;
  for (const auto& rf : risk_factors) {
    std::string ccy = rf.first.substr(rf.first.length() - 3);
    if (risk_ccys.find(ccy) == risk_ccys.end()) {
      risk_ccys.insert(ccy);
      ccys->push_back(ccy);
    }
  }
}

trade_value_t pv01_or_nan(trade_value_t& hi, trade_value_t& lo, double dr) {
  if (std::isnan(hi.first))
    return std::make_pair(nan<double>(), hi.second);
  if (std::isnan(lo.first))
    return std::make_pair(nan<double>(), lo.second);
  return std::make_pair((hi.first - lo.first) / dr, "");
}
}

void print_portfolio(const portfolio_t& portfolio) {
  std::for_each(portfolio.begin(), portfolio.end(), [](auto& pt){ pt->print(std::cout); });
}

std::vector<ppricer_t> get_pricers(
    const portfolio_t& portfolio, const std::string& base_ccy) {
  std::vector<ppricer_t> pricers(portfolio.size());
  std::transform(
      portfolio.begin(), portfolio.end(), pricers.begin(), 
      [base_ccy](auto &pt) -> ppricer_t { return pt->pricer(base_ccy); } );
  return pricers;
}

portfolio_values_t compute_prices(
    const std::vector<ppricer_t>& pricers, Market& mkt, 
    std::shared_ptr<const FixingDataServer> fds) {
  portfolio_values_t prices;
  for (const auto& pricer : pricers) {
    try {
      auto price = pricer->price(mkt, fds.get());
      prices.push_back(std::make_pair(price, ""));
    } catch (std::exception& e) {
      prices.push_back(std::make_pair(nan<double>(), e.what()));
    }
  }
  return prices;
}

std::pair<double, std::vector<std::pair<size_t, std::string>>> portfolio_total(
    const portfolio_values_t& values) {
  double total = 0.0;
  std::vector<std::pair<size_t, std::string>> errors;
  for (int i = 0; i < values.size(); ++i) {
    const auto& value = values[i];
    if (std::isnan(value.first)) {
      errors.push_back(std::make_pair(i, value.second));
    } else {
      total += value.first;
    }
  }
  return std::make_pair(total, errors);
}

std::vector<std::pair<string, portfolio_values_t>> compute_pv01(
    const std::vector<ppricer_t>& pricers, const Market& mkt,
    std::shared_ptr<const FixingDataServer> fds) {
    std::vector<std::pair<string, portfolio_values_t>> pv01;  // PV01 per trade

    const double bump_size = 0.01 / 100;

    // filter risk factors related to IR
    auto base = mkt.get_risk_factors(ir_rate_prefix + "[A-Z]{3}");

    // Make a local copy of the Market object, because we will modify it applying bumps
    // Note that the actual market objects are shared, as they are referred to via pointers
    Market tmpmkt(mkt);

    // compute prices for perturbated markets and aggregate results
    pv01.reserve(base.size());
    for (const auto& d : base) {
        std::vector<std::pair<string, double>> bumped(1, d);
        pv01.push_back(std::make_pair(
              d.first, std::vector<trade_value_t>(pricers.size())));

        // bump down and price
        bumped[0].second = d.second - bump_size;
        tmpmkt.set_risk_factors(bumped);
        auto pv_dn = compute_prices(pricers, tmpmkt, fds);

        // bump up and price
        bumped[0].second = d.second + bump_size; // bump up
        tmpmkt.set_risk_factors(bumped);
        auto pv_up = compute_prices(pricers, tmpmkt, fds);

        // compute estimator of the derivative via central finite differences
        double dr = 2.0 * bump_size;
        std::transform(pv_up.begin(), pv_up.end(), pv_dn.begin(), pv01.back().second.begin()
            , [dr](auto& hi, auto& lo) -> trade_value_t { return pv01_or_nan(hi, lo, dr); });
    }

    return pv01;
}

std::vector<std::pair<std::string, portfolio_values_t>> compute_pv01_parallel(
    const std::vector<ppricer_t>& pricers, const Market& mkt,
    std::shared_ptr<const FixingDataServer> fds) {
  std::vector<std::pair<std::string, portfolio_values_t>> pv01;
  const double bump_size = 0.01 / 100;
  const double dr = 2.0 * bump_size;
  auto risk_factors = mkt.get_risk_factors(
      ir_rate_prefix + "([0-9]+(D|W|M|Y)\\.)?[A-Z]{3}");
  std::vector<std::string> risk_ccys;
  find_all_risk_ccy(risk_factors, &risk_ccys);
  Market tmpmkt(mkt);

  for (const auto& risk_ccy : risk_ccys) {
    auto base = mkt.get_risk_factors(
        ir_rate_prefix + "([0-9]+(D|W|M|Y)\\.)?" + risk_ccy);
    std::vector<std::pair<std::string, double>> bumped_up(base);
    std::vector<std::pair<std::string, double>> bumped_dn(base);
    bump_risk_factors(bump_size, &bumped_up, &bumped_dn);
    tmpmkt.set_risk_factors(bumped_up);
    auto pv_up = compute_prices(pricers, tmpmkt, fds);
    tmpmkt.set_risk_factors(bumped_dn);
    auto pv_dn = compute_prices(pricers, tmpmkt, fds);
    tmpmkt.set_risk_factors(base);

    pv01.push_back(
        std::make_pair(
          "parallel " + ir_rate_prefix + risk_ccy, 
          std::vector<trade_value_t>(pricers.size())));
    std::transform(
        pv_up.begin(), pv_up.end(), pv_dn.begin(), pv01.back().second.begin(),
        [dr](auto& hi, auto& lo) -> 
        trade_value_t { return pv01_or_nan(hi, lo, dr); });
  }
  return pv01;
}

std::vector<std::pair<std::string, portfolio_values_t>> compute_pv01_bucketed(
    const std::vector<ppricer_t>& pricers, const Market& mkt,
    std::shared_ptr<const FixingDataServer> fds) {
  std::vector<std::pair<std::string, portfolio_values_t>> pv01;
  const double bump_size = 0.01 / 100;
  const double dr = 2.0 * bump_size;
  std::set<std::string> buckets; 
  Market tmpmkt(mkt);
  auto base = mkt.get_risk_factors(
      ir_rate_prefix + "[0-9]+(D|W|M|Y)\\.[A-Z]{3}");
  for (auto& rf : base) {
    auto original_value = rf.second;
    const auto& bucket = rf.first;

    rf.second = original_value + bump_size;
    tmpmkt.set_risk_factors(base);
    auto pv_up = compute_prices(pricers, tmpmkt, fds);

    rf.second = original_value - bump_size;
    tmpmkt.set_risk_factors(base);
    auto pv_dn = compute_prices(pricers, tmpmkt, fds);

    // Revert changes.
    rf.second = original_value;

    pv01.push_back(
        std::make_pair(
          "bucketed " + bucket, std::vector<trade_value_t>(pricers.size())));
    std::transform(
        pv_up.begin(), pv_up.end(), pv_dn.begin(), pv01.back().second.begin(),
        [dr](auto& hi, auto& lo) -> 
        trade_value_t { return pv01_or_nan(hi, lo, dr); });
  }
  return pv01;
}

std::vector<std::pair<std::string, portfolio_values_t>> compute_fx_delta(
     const std::vector<ppricer_t>& pricers, const Market& mkt,
     std::shared_ptr<const FixingDataServer> fds) {
  std::vector<std::pair<std::string, portfolio_values_t>> fx_delta;
  auto fx_spots = mkt.get_risk_factors(fx_spot_prefix + "[A-Z]{3}");
  std::vector<std::string> risk_ccys;
  find_all_risk_ccy(fx_spots, &risk_ccys);
  Market tmpmkt(mkt);

  for (const auto& risk_ccy : risk_ccys) {
    auto risk_factors = mkt.get_risk_factors(fx_spot_prefix + risk_ccy);
    MYASSERT(risk_factors.size() == 1, 
        "Duplicate fx spot rate." << fx_spot_prefix + risk_ccy);
    const double original_value = risk_factors[0].second;
    double bump_size = original_value * 0.1 / 100;
    double dr = 2 * bump_size;
    risk_factors[0].second = original_value + bump_size;
    tmpmkt.set_risk_factors(risk_factors);
    auto pv_up = compute_prices(pricers, tmpmkt, fds);
    risk_factors[0].second = original_value - bump_size;
    tmpmkt.set_risk_factors(risk_factors);
    auto pv_dn = compute_prices(pricers, tmpmkt, fds);
    risk_factors[0].second = original_value;
    tmpmkt.set_risk_factors(risk_factors);

    fx_delta.push_back(
        std::make_pair(fx_spot_prefix + risk_ccy,
          std::vector<trade_value_t>(pricers.size())));

    std::transform(
        pv_up.begin(), pv_up.end(), pv_dn.begin(), 
        fx_delta.back().second.begin(), [dr](auto& hi, auto& lo) ->
        trade_value_t { return pv01_or_nan(hi, lo, dr); });
  }
  return fx_delta;
}

ptrade_t load_trade(my_ifstream& is) {
  string name;
  ptrade_t p;

  // read trade identifier
  guid_t id;
  is >> id;

  if (id == TradePayment::m_id)
    p.reset(new TradePayment);
  else if (id == TradeFXForward::m_id)
    p.reset(new TradeFXForward);
  else
    THROW("Unknown trade type:" << id);

  p->load(is);

  return p;
}

void save_portfolio(const string& filename, const std::vector<ptrade_t>& portfolio)
{
    // test saving to file
    my_ofstream of(filename);
    for( const auto& pt : portfolio) {
        pt->save(of);
        of.endl();
    }
    of.close();
}

std::vector<ptrade_t> load_portfolio(const string& filename)
{
    std::vector<ptrade_t> portfolio;

    // test reloading the portfolio
    my_ifstream is(filename);
    while (is.read_line())
        portfolio.push_back(load_trade(is));

    return portfolio;
}

void print_price_vector(const string& name, const portfolio_values_t& values) {
  const auto& res = portfolio_total(values);
  std::cout
      << "========================\n"
      << name << ":\n"
      << "========================\n"
      << "Total:  " << res.first << "\n"
      << "Errors: " << res.second.size() << "\n"
      << "\n========================\n";

  for (size_t i = 0, n = values.size(); i < n; ++i)
    if (std::isnan(values[i].first)) 
      std::cout << std::setw(5) << i << ": " << values[i].second << "\n";
    else
      std::cout << std::setw(5) << i << ": " << values[i].first << "\n";

  std::cout << "========================\n\n";
}

} // namespace minirisk
