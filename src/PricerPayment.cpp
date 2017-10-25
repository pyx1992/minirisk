#include "PricerPayment.h"
#include "TradePayment.h"
#include "CurveDiscount.h"

namespace minirisk {

PricerPayment::PricerPayment(const TradePayment& trd)
    : m_amt(trd.quantity())
    , m_dt(trd.delivery_date())
    , m_ir_curve(ir_curve_discount_name(trd.ccy()))
    , m_fx_ccy(fx_spot_name(trd.ccy(), "USD"))
{
}

double PricerPayment::price(Market& mkt) const
{
    ptr_disc_curve_t disc = mkt.get_discount_curve(m_ir_curve);
    double df = disc->df(m_dt); // this throws an exception if m_dt<today

    ptr_fx_spot_curve_t fx_spot = mkt.get_fx_spot_curve(m_fx_ccy); 
    double fx_rate = fx_spot->spot();

    return m_amt * df * fx_rate;
}

} // namespace minirisk


