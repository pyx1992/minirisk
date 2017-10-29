#pragma once

#include <string>
#include <map>

#include "Date.h"

namespace minirisk {

struct FixingDataServer {
 public:
  explicit FixingDataServer(const std::string& filename);
  double get(const std::string& name, const Date& t) const;
  std::pair<double, bool> lookup(const std::string& name, const Date& t) const;

 private:
  std::map<std::string, std::map<Date, double>> m_data;
};

} // namespace minirisk
