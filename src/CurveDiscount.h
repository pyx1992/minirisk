#pragma once

#include <vector>

#include "ICurve.h"

namespace minirisk {

struct Market;

struct CurveDiscount : ICurveDiscount
{
    virtual string name() const { return m_name; }

    CurveDiscount(Market *mkt, const Date& today, const string& curve_name);

    void init_log_discounting_factors(Market *mkt);

    // compute the discount factor
    double df(const Date& t) const;

    virtual Date today() const { return m_today; }

private:
    int32_t convert_tenor_to_int(std::string& tenor) const;

    Date m_today;
    Date m_last_tenor_date;
    string m_name;
    std::vector<std::pair<double, double>> m_log_dfs;
    double m_rate;
};

} // namespace minirisk
