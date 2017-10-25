#include "CurveFXSpot.h"
#include "Market.h"

namespace minirisk {

CurveFXSpot::CurveFXSpot(
    Market *mkt, const Date& today, const std::string& name) 
    : m_today(today), m_name(name), m_rate(mkt->get_fx_spot(name)) {}

} // namespace minirisk
