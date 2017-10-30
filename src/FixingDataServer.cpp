#include "FixingDataServer.h"

#include <fstream>

#include "Macros.h"
#include "Global.h"

namespace minirisk {

FixingDataServer::FixingDataServer(const std::string& filename) {
  std::ifstream is(filename);
  MYASSERT(!is.fail(), "Could not open file " << filename);
  std::string name;
  std::string date;
  double value;
  while (is >> name >> date >> value) {
    m_data.emplace(name, std::map<Date, double>());
    auto ins = m_data[name].emplace(Date(date), value);
    MYASSERT(ins.second, "Duplicated fixing: " << date << " " << value);
  }
}

double FixingDataServer::get(const std::string& name, const Date& t) const {
  auto iter = m_data.find(name);
  MYASSERT(iter != m_data.end(), "Fixing not found: " << name << "," 
      << t.to_string());
  auto date_iter = iter->second.find(t);
  MYASSERT(date_iter != iter->second.end(), "Fixing not found: " 
      << name << "," << t.to_string());
  return date_iter->second;
}

std::pair<double, bool> FixingDataServer::lookup(
    const std::string& name, const Date& t) const {
  auto iter = m_data.find(name);
  if (iter != m_data.end()) {
    auto date_iter = iter->second.find(t);
    if (date_iter != iter->second.end())
      return std::make_pair(date_iter->second, true);
  }
  return std::make_pair(nan<double>(), false);
}
  
} // namespace minirisk
