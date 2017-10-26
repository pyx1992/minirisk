#include "TradePayment.h"
#include "PricerPayment.h"

namespace minirisk {

ppricer_t TradePayment::pricer(const std::string& base_ccy) const {
    return ppricer_t(new PricerPayment(*this, base_ccy));
}

} // namespace minirisk
