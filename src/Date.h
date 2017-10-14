#pragma once

#include "Macros.h"
#include <string>
#include <array>

namespace minirisk {

struct Date
{
public:
    static const unsigned first_year = 1900;
    static const unsigned last_year = 2200;
    static const unsigned n_years = last_year - first_year;

private:
    static std::string padding_dates(unsigned);

    // number of days elapsed from beginning of the year
    unsigned day_of_year() const;

    friend long operator-(const Date& d1, const Date& d2);

    static const std::array<unsigned, 12> days_in_month;  // num of days in month M in a normal year
    static const std::array<unsigned, 12> days_ytd;      // num of days since 1-jan to 1-M in a normal year
    static const std::array<unsigned, n_years> days_epoch;   // num of days since 1-jan-1900 to 1-jan-yyyy (until 2200)

public:
    // Default constructor
    Date() : m_y(1970), m_m(1), m_d(1), m_is_leap(false) {}

    // Constructor where the input value is checked.
    Date(unsigned year, unsigned month, unsigned day)
    {
        init(year, month, day);
    }

    void init(unsigned year, unsigned month, unsigned day)
    {
        check_valid(year, month, day);
        m_y = (unsigned short) year;
        m_m = (unsigned char) month;
        m_d = (unsigned char) day;
        m_is_leap = is_leap_year(year);
    }

    static void check_valid(unsigned y, unsigned m, unsigned d);

    bool operator<(const Date& d) const
    {
        return (m_y < d.m_y) || (m_y == d.m_y && (m_m < d.m_m || (m_m == d.m_m && m_d < d.m_d)));
    }

    bool operator==(const Date& d) const
    {
        return (m_y == d.m_y) && (m_m == d.m_m) && (m_d == d.m_d);
    }

    bool operator>(const Date& d) const
    {
        return d < (*this);
    }

    // number of days since 1-Jan-1900
    unsigned serial() const
    {
        return days_epoch[m_y - 1900] + day_of_year();
    }

    static bool is_leap_year(unsigned yr);

    // In YYYYMMDD format
    std::string to_string(bool pretty = true) const
    {
        return pretty
            ? std::to_string((int)m_d) + "-" + std::to_string((int)m_m) + "-" + std::to_string(m_y)
            : std::to_string(m_y) + padding_dates((int)m_m) + padding_dates((int)m_d);
    }

private:
    unsigned short m_y;
    unsigned char m_m;
    unsigned char m_d;
    bool m_is_leap;
};

long operator-(const Date& d1, const Date& d2);

inline double time_frac(const Date& d1, const Date& d2)
{
    return static_cast<double>(d2 - d1) / 365.0;
}

} // namespace minirisk
