#include "Global.h"
#include "PortfolioUtils.h"
#include "TradePayment.h"

#include <numeric>
#include <set>

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
}

void print_portfolio(const portfolio_t& portfolio)
{
    std::for_each(portfolio.begin(), portfolio.end(), [](auto& pt){ pt->print(std::cout); });
}

std::vector<ppricer_t> get_pricers(const portfolio_t& portfolio)
{
    std::vector<ppricer_t> pricers(portfolio.size());
    std::transform( portfolio.begin(), portfolio.end(), pricers.begin()
                  , [](auto &pt) -> ppricer_t { return pt->pricer(); } );
    return pricers;
}

portfolio_values_t compute_prices(const std::vector<ppricer_t>& pricers, Market& mkt)
{
    portfolio_values_t prices(pricers.size());
    std::transform(pricers.begin(), pricers.end(), prices.begin()
        , [&mkt](auto &pp) -> double { return pp->price(mkt); });
    return prices;
}

double portfolio_total(const portfolio_values_t& values)
{
    return std::accumulate(values.begin(), values.end(), 0.0);
}

std::vector<std::pair<string, portfolio_values_t>> compute_pv01(const std::vector<ppricer_t>& pricers, const Market& mkt)
{
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
        std::vector<double> pv_up, pv_dn;
        std::vector<std::pair<string, double>> bumped(1, d);
        pv01.push_back(std::make_pair(d.first, std::vector<double>(pricers.size())));

        // bump down and price
        bumped[0].second = d.second - bump_size;
        tmpmkt.set_risk_factors(bumped);
        pv_dn = compute_prices(pricers, tmpmkt);

        // bump up and price
        bumped[0].second = d.second + bump_size; // bump up
        tmpmkt.set_risk_factors(bumped);
        pv_up = compute_prices(pricers, tmpmkt);

        // compute estimator of the derivative via central finite differences
        double dr = 2.0 * bump_size;
        std::transform(pv_up.begin(), pv_up.end(), pv_dn.begin(), pv01.back().second.begin()
            , [dr](double hi, double lo) -> double { return (hi - lo) / dr; });
    }

    return pv01;
}

std::vector<std::pair<std::string, portfolio_values_t>> compute_pv01_parallel(
    const std::vector<ppricer_t>& pricers, const Market& mkt) {
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
    auto pv_up = compute_prices(pricers, tmpmkt);
    tmpmkt.set_risk_factors(bumped_dn);
    auto pv_dn = compute_prices(pricers, tmpmkt);
    pv01.push_back(
        std::make_pair(
          "parallel " + ir_rate_prefix + risk_ccy, 
          std::vector<double>(pricers.size())));
    std::transform(
        pv_up.begin(), pv_up.end(), pv_dn.begin(), pv01.back().second.begin(),
        [dr](double hi, double lo) -> double { return (hi - lo) / dr; });
  }
  return pv01;
}

std::vector<std::pair<std::string, portfolio_values_t>> compute_pv01_bucketed(
    const std::vector<ppricer_t>& pricers, const Market& mkt) {
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
    std::vector<std::pair<std::string, double>> bumped_up(base);
    rf.second = original_value - bump_size;
    std::vector<std::pair<std::string, double>> bumped_dn(base);
    rf.second = original_value;

    tmpmkt.set_risk_factors(bumped_up);
    auto pv_up = compute_prices(pricers, tmpmkt);
    tmpmkt.set_risk_factors(bumped_dn);
    auto pv_dn = compute_prices(pricers, tmpmkt);

    pv01.push_back(
        std::make_pair(
          "bucketed " + bucket, std::vector<double>(pricers.size())));
    std::transform(
        pv_up.begin(), pv_up.end(), pv_dn.begin(), pv01.back().second.begin(),
        [dr](double hi, double lo) -> double { return (hi - lo) / dr; });
  }
  return pv01;
}

ptrade_t load_trade(my_ifstream& is)
{
    string name;
    ptrade_t p;

    // read trade identifier
    guid_t id;
    is >> id;

    if (id == TradePayment::m_id)
        p.reset(new TradePayment);
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

void print_price_vector(const string& name, const portfolio_values_t& values)
{
    std::cout
        << "========================\n"
        << name << ":\n"
        << "========================\n"
        << "Total: " << portfolio_total(values)
        << "\n========================\n";

    for (size_t i = 0, n = values.size(); i < n; ++i)
        std::cout << std::setw(5) << i << ": " << values[i] << "\n";

    std::cout << "========================\n\n";
}

} // namespace minirisk
