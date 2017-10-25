#pragma once

#include <string>

#include "ICurve.h"

namespace minirisk {

struct Market;

struct CurveFXSpot : ICurveFXSpot {
 public:
  virtual std::string name() const { return m_name; }

  virtual Date today() const { return m_today; }

  CurveFXSpot(
      Market *mkt, const Date& today, const std::string& name);

  virtual double spot() const { return m_rate; }

 private:
  Date m_today;
  std::string m_name;
  double m_rate;
}; // struct CurveFXSpot

} // namespace minirisk
