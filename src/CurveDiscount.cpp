#include "CurveDiscount.h"
#include "Market.h"
#include "Streamer.h"

#include <cmath>

namespace minirisk {
namespace {
struct RatePairDoubleComparator {
  bool operator() (
      const double& left, const std::pair<double, double>& right) {
    return left < right.first;
  }
};
}

CurveDiscount::CurveDiscount(
    Market *mkt, const Date& today, const string& curve_name)
    : m_today(today), m_name(curve_name) {
  init_log_discounting_factors(mkt); 
}

void CurveDiscount::init_log_discounting_factors(Market *mkt) {
  std::string ccy = m_name.substr(m_name.length() - 3);
  std::string regex = ir_rate_prefix + "[0-9]+(D|W|M|Y)\\." + ccy;
  const auto& matched = mkt->fetch_risk_factors(regex);
  m_log_dfs.push_back(std::make_pair(0.0, 0.0));
  std::vector<std::pair<int32_t, double>> tenor_rates;
  for (const auto& rate : matched) {
    std::string tenor = rate.first.substr(
        ir_rate_prefix.length(), 
        rate.first.length() - 4 - ir_rate_prefix.length());
    tenor_rates.push_back(
        std::make_pair(convert_tenor_to_int(tenor), rate.second));
  } 
  std::sort(tenor_rates.begin(), tenor_rates.end());
  for (const auto& rate : tenor_rates) {
    double tf = rate.first / 365.0;
    double df = -rate.second * tf;
    m_log_dfs.push_back(std::make_pair(tf, df));
  }

  // Init from yield.
  if (tenor_rates.empty()) {
    m_rate = mkt->get_yield(ccy);
  } else {
    m_last_tenor_date = m_today + tenor_rates.back().first;
  }
}

int32_t CurveDiscount::convert_tenor_to_int(std::string& tenor) const {
  int numeric = std::stoi(tenor.substr(0, tenor.length() - 1));
  int base = 1;
  switch (tenor.c_str()[tenor.length() - 1]) {
    case 'D':
      base = 1;
      break;
    case 'W':
      base = 7;
      break;
    case 'M':
      base = 30;
      break;
    case 'Y':
      base = 365;
      break;
    default:
      MYASSERT(false, "Unexpected tenor type " << tenor);
  }
  return numeric * base;
}

double CurveDiscount::df(const Date& t) const {
  MYASSERT((!(t < m_today)), 
      "Curve " << m_name << ", DF not available before anchor date " << m_today 
      << ", requested " << t);
  double dt = time_frac(m_today, t);

  // Use yield.
  if (m_log_dfs.size() == 1) {
    return std::exp(-m_rate * dt);
  }

  // Use discounting factors with interpolation.
  auto it = std::upper_bound(
      m_log_dfs.begin(), m_log_dfs.end(), dt, RatePairDoubleComparator());
  if (it == m_log_dfs.end()) {
    if (dt == m_log_dfs.back().first) {
      return std::exp(m_log_dfs.back().second);
    }
    MYASSERT(false, 
        "Curve " << m_name << ", DF not available beyond last tenor date " 
        << m_last_tenor_date << ", requested " << t);
  } else {
    const auto& t2_log_df = *it;
    const auto& t1_log_df = *(--it);
    return std::exp(
        ((t2_log_df.first - dt) * t1_log_df.second 
         + (dt - t1_log_df.first) * t2_log_df.second) 
        / (t2_log_df.first - t1_log_df.first));
  }
}

} // namespace minirisk
