#include "Market.h"
#include "CurveDiscount.h"
#include "CurveFXSpot.h"
#include "CurveFXForward.h"

#include <cmath>
#include <vector>

namespace minirisk {

template <typename I, typename T>
std::shared_ptr<const I> Market::get_curve(const string& name) {
  ptr_curve_t& curve_ptr = m_curves[name];
  if (!curve_ptr.get())
      curve_ptr.reset(new T(this, m_today, name));
  std::shared_ptr<const I> res = 
    std::dynamic_pointer_cast<const I>(curve_ptr);
  MYASSERT(res, "Cannot cast object with name " << name << " to type " 
      << typeid(I).name());
  return res;
}

const ptr_disc_curve_t Market::get_discount_curve(const string& name) {
  return get_curve<ICurveDiscount, CurveDiscount>(name);
}

const ptr_fx_spot_curve_t Market::get_fx_spot_curve(const string& name) {
  return get_curve<ICurveFXSpot, CurveFXSpot>(name);
}

const ptr_fx_fwd_curve_t Market::get_fx_fwd_curve(const string& name) {
  return get_curve<ICurveFXForward, CurveFXForward>(name);
}

double Market::from_mds(const string& objtype, const string& name) {
  auto ins = m_risk_factors.emplace(name, nan<double>());
  if (ins.second) { // just inserted, need to be populated
      MYASSERT(m_mds, "Cannot fetch " << objtype << " " << name 
          << " because the market data server has been disconnnected");
      if (objtype == "fx spot" && !m_mds->lookup(name).second) {
        ins.first->second = m_mds->get(mds_spot_name(name));
      } else {
        ins.first->second = m_mds->get(name);
      }
  }
  return ins.first->second;
}

double Market::get_yield(const string& ccyname)
{
    string name(ir_rate_prefix + ccyname);
    return from_mds("yield curve", name);
};

Market::vec_risk_factor_t Market::fetch_risk_factors(const string& regex) {
  if (m_fetched_regex.find(regex) != m_fetched_regex.end())
    return get_risk_factors(regex);
  auto rate_names = m_mds->match(regex);
  std::vector<std::pair<std::string, double>> rates;
  for (const auto& name : rate_names) {
    rates.push_back(std::make_pair(name, from_mds("curve rate", name)));
  }
  m_fetched_regex.insert(regex);
  return rates;
}

double Market::get_fx_spot(const string& name) {
  const auto ccy_pair = fx_spot_name_to_ccy_pair(name);
  return get_fx_spot(ccy_pair.first, ccy_pair.second);
}

double Market::get_fx_spot(const std::string& base, const std::string& quote) {
  if (m_fx_ccy_idx.find(base) == m_fx_ccy_idx.end() ||
      m_fx_ccy_idx.find(quote) == m_fx_ccy_idx.end())
    MYASSERT(false, "Rate not available for " << base << quote);
  const auto rate = m_fx_spot_rate[m_fx_ccy_idx[base]][m_fx_ccy_idx[quote]];
  MYASSERT(rate > 0, "Rate not available for " << base << quote);
  return rate;
}

void Market::set_risk_factors(const vec_risk_factor_t& risk_factors) {
  clear();
  for (const auto& d : risk_factors) {
      auto i = m_risk_factors.find(d.first);
      MYASSERT((i != m_risk_factors.end()), "Risk factor not found " 
          << d.first);
      i->second = d.second;
  }
  construct_fx_spot_rate_matrix();
}

Market::vec_risk_factor_t Market::get_risk_factors(
    const std::string& expr) const {
  vec_risk_factor_t result;
  std::regex r(expr);
  for (const auto& d : m_risk_factors)
      if (std::regex_match(d.first, r))
          result.push_back(d);
  return result;
}

void Market::construct_fx_spot_rate_matrix() {
  m_fx_ccy_idx.clear();
  std::memset(m_fx_spot_rate, 0, sizeof m_fx_spot_rate);
  const auto& fx_rates = fetch_risk_factors(
      "FX\\.SPOT\\.[A-Z]{3}(\\.[A-Z]{3})?");
  u_int32_t idx = 0;
  for (const auto& fx_rate : fx_rates) {
    const auto ccy_pair = fx_spot_name_to_ccy_pair(fx_rate.first);
    for (const auto ccy : {ccy_pair.first, ccy_pair.second}) {
      if (m_fx_ccy_idx.find(ccy) == m_fx_ccy_idx.end()) {
        m_fx_ccy_idx.emplace(ccy, idx);
        ++idx;
      }
    }
    m_fx_spot_rate[m_fx_ccy_idx[ccy_pair.first]][m_fx_ccy_idx[ccy_pair.second]] 
      = fx_rate.second;
    m_fx_spot_rate[m_fx_ccy_idx[ccy_pair.second]][m_fx_ccy_idx[ccy_pair.first]] 
      = 1.0 / fx_rate.second;
  }
  
  // Run Floyd-Warshall algorithm to get all pairs' value.
  size_t size = m_fx_ccy_idx.size();
  for (int k = 0; k < size; ++k) {
    m_fx_spot_rate[k][k] = 1.0;
    for (int i = 0; i < size; ++i)
      for (int j = 0; j < size; ++j)
        if (m_fx_spot_rate[i][j] == 0)
          m_fx_spot_rate[i][j] = m_fx_spot_rate[i][k] * m_fx_spot_rate[k][j];
  }
}

std::pair<std::string, std::string> Market::fx_spot_name_to_ccy_pair(
    const std::string& name) {
  const auto base = name.substr(fx_spot_prefix.length(), 3);
  const auto quote = 
    name.length() == fx_spot_prefix.length() + 7 ? 
    name.substr(fx_spot_prefix.length() + 4) : "USD";
  return {base, quote};
}

std::pair<std::string, std::string> Market::fx_fwd_name_to_ccy_pair(
    const std::string& name) {
  const auto base = name.substr(fx_fwd_prefix.length(), 3);
  const auto quote = 
    name.length() == fx_fwd_prefix.length() + 7 ?
    name.substr(fx_fwd_prefix.length() + 4) : "USD";
  return {base, quote};
}

} // namespace minirisk
