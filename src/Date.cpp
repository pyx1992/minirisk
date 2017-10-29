#include <iomanip>
#include <algorithm>

#include "Date.h"

namespace minirisk {

struct DateInitializer : std::array<unsigned, Date::n_years> {
  DateInitializer() {
    for (unsigned i = 0, s = 0, y = Date::first_year; i < size(); ++i, ++y) {
      (*this)[i] = s;
      s += 365 + (Date::is_leap_year(y) ? 1 : 0);
    }
  }
};

const std::array<unsigned, 12> Date::days_in_month = {
  {31,28,31,30,31,30,31,31,30,31,30,31} };
const std::array<unsigned, 12> Date::days_in_month_leap = {
  {31,29,31,30,31,30,31,31,30,31,30,31} };
const std::array<unsigned, 12> Date::days_ytd{ 
  {0,31,59,90,120,151,181,212,243,273,304,334} };
const std::array<unsigned, 12> Date::days_ytd_leap{ 
  {0,31,60,91,121,152,182,213,244,274,305,335} };
const std::array<unsigned, Date::n_years> Date::days_epoch(
    static_cast<const std::array<unsigned, Date::n_years>&>(DateInitializer()));

Date::Date(const std::string& yyyymmdd) 
  : Date(std::stoul(yyyymmdd.substr(0, 4)), std::stoul(yyyymmdd.substr(4, 2)),
      std::stoul(yyyymmdd.substr(6))) {}

/* The function checks if a given year is a leap year.
    Leap year must be a multiple of 4, but it cannot be a multiple of 100 
    without also being a multiple of 400.
*/
bool Date::is_leap_year(unsigned year) {
  return ((year % 4 != 0) ? false : 
      (year % 100 != 0) ? true : (year % 400 != 0) ? false : true);
}

// The function pads a zero before the month or day if it has only one digit.
std::string Date::padding_dates(unsigned month_or_day) {
  std::ostringstream os;
  os << std::setw(2) << std::setfill('0') << month_or_day;
  return os.str();
}

bool Date::is_valid_date(unsigned y, unsigned m, unsigned d) {
  if (y < first_year || y >= last_year) return false;
  if (m < 1 || m > 12) return false;
  unsigned dmax = days_in_month[m - 1] + ((m == 2 && is_leap_year(y)) ? 1 : 0);
  if (d < 1 || d > dmax) return false;
  return true;
}

void Date::check_valid(unsigned y, unsigned m, unsigned d) {
  MYASSERT(is_valid_date(y, m, d), "Invalid date" << y << " " << m << " " << d);
}

void Date::to_y_m_d(unsigned *y, unsigned *m, unsigned *d) const {
  auto days_left = m_serial;
  auto y_it = std::upper_bound(days_epoch.begin(), days_epoch.end(), days_left);
  --y_it;
  *y = y_it - days_epoch.begin() + 1900;
  days_left -= *y_it;
  bool is_leap = is_leap_year(*y);
  auto m_it = is_leap
    ? std::upper_bound(days_ytd_leap.begin(), days_ytd_leap.end(), days_left)
    : std::upper_bound(days_ytd.begin(), days_ytd.end(), days_left);
  --m_it;
  *m = m_it - (is_leap ? days_ytd_leap.begin() : days_ytd.begin()) + 1;
  days_left -= *m_it;
  *d = days_left + 1;
}

/*  The function calculates the distance between two Dates.
    d1 > d2 is allowed, which returns the negative of d2-d1.
*/
long operator-(const Date& d1, const Date& d2) {
    unsigned s1 = d1.serial();
    unsigned s2 = d2.serial();
    return static_cast<long>(s1) - static_cast<long>(s2);
}

} // namespace minirisk

