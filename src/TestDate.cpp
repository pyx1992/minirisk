#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>

#include "Date.h"
#include "Macros.h"

void test1() {
  int seed = std::time(NULL) ;
  std::cout << "Random seed: " << seed << std::endl;
  int invalid_count = 0;
  int error_count = 0;
  while (invalid_count < 1000) {
    unsigned y, m ,d;
    y = std::rand() % 400 + 1850;
    m = std::rand() % 12 + 1;
    d = std::rand() % 32 + 1;
    if (!minirisk::Date::is_valid_date(y, m ,d)) {
      invalid_count += 1;
      try {
        minirisk::Date date(y, m, d);
      } catch (...) {
        error_count += 1; 
      }
    }
  }
  MYASSERT(invalid_count == 1000 && error_count == 1000, "Error");
}

void test2() {
  std::ifstream mfile("../data/all_dates.txt");
  if (mfile.is_open()) {
    unsigned y_i, m_i, d_i;
    unsigned y_o, m_o, d_o;
    while (mfile.good()) {
      mfile >> y_i >> m_i >> d_i;
      minirisk::Date date(y_i, m_i, d_i);
      date.to_y_m_d(&y_o, &m_o, &d_o);
      MYASSERT(
          d_i == d_o && m_i == m_o && y_i == y_o, 
          "Conversion error " << y_i << " " << m_i << " " << d_i << " vs " 
          << y_o << " " << m_o << " " << d_o);
    } 
    mfile.close();
  } 
}

void test3() {
  std::ifstream mfile("all_dates.txt");
  if (mfile.is_open()) {
    unsigned y, m, d;
    int count = 0;
    minirisk::Date prev_date;
    while (mfile >> y >> m >> d) {
      minirisk::Date date(y, m, d);
      if (count > 0)
        MYASSERT(
            date.serial() - prev_date.serial() == 1, 
            "Wrong " << date.serial() << " " << prev_date.serial());
      ++count;
      prev_date = date;
    } 
    mfile.close();
  } 
}

int main()
{
    test1();
    test2();
    test3();
    std::cout << "SUCCESS" << std::endl;
    return 0;
}

