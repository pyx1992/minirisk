#include "PricerPayment.h"
#include "TradePayment.h"
#include "CurveDiscount.h"

namespace minirisk {

PricerPayment::PricerPayment(
    const TradePayment& trd, const std::string& base_ccy)
    : m_amt(trd.quantity())
    , m_dt(trd.delivery_date())
    , m_ir_curve(ir_curve_discount_name(trd.ccy()))
    , m_fx_ccy(fx_spot_name(trd.ccy(), base_ccy)) {}

double PricerPayment::price(Market& mkt, const FixingDataServer* fds) const {
  ptr_disc_curve_t disc = mkt.get_discount_curve(m_ir_curve);
  double df = disc->df(m_dt); // this throws an exception if m_dt<today

  const auto fx_spot = mkt.get_fx_spot(m_fx_ccy); 

  return m_amt * df * fx_spot;
}

} // namespace minirisk


