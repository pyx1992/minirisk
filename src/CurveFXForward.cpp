#include "CurveFXForward.h"
#include "Global.h"
#include "Market.h"

namespace minirisk {

CurveFXForward::CurveFXForward(
    Market *mkt, const Date& today, const std::string& base,
    const std::string& quote) 
    : m_today(today), 
      m_df1(mkt->get_discount_curve(ir_curve_discount_name(base))),
      m_df2(mkt->get_discount_curve(ir_curve_discount_name(quote))),
      m_spot(mkt->get_fx_spot_curve(fx_spot_name(base, quote))) {}

std::string CurveFXForward::name() const {
  return "FX.FWD." + m_df1->name().substr(m_df1->name().length() - 3) + "."
    + m_df2->name().substr(m_df2->name().length() - 3); 
}

double CurveFXForward::fwd(const Date& t) const {
  return m_spot->spot() * m_df1->df(t) / m_df2->df(t);
}

} // namespace minirisk
