#include "TradeFXForward.h"
#include "PricerForward.h"

namespace minirisk {

ppricer_t TradeFXForward::pricer(const std::string& base_ccy) const {
  return ppricer_t(new PricerForward(*this, base_ccy));
}

} // namespace minirisk
