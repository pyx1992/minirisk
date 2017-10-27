#include "CurveFXForward.h"
#include "Global.h"
#include "Market.h"

namespace minirisk {

CurveFXForward::CurveFXForward(
    Market *mkt, const Date& today, const std::string& name) 
    : m_today(today), m_name(name) {
  const auto& ccys = mkt->fx_fwd_name_to_ccy_pair(name);
  m_df1 = mkt->get_discount_curve(ir_curve_discount_name(ccys.first));
  m_df2 = mkt->get_discount_curve(ir_curve_discount_name(ccys.second));
  m_spot = mkt->get_fx_spot_curve(fx_spot_name(ccys.first, ccys.second));
}

double CurveFXForward::fwd(const Date& t) const {
  return m_spot->spot() * m_df1->df(t) / m_df2->df(t);
}

} // namespace minirisk
