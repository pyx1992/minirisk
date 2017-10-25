#include <iostream>
#include <vector>

#include "Macros.h"
#include "CurveFXSpot.h"
#include "MarketDataServer.h"
#include "PortfolioUtils.h"

using namespace::minirisk;

void run(const string& risk_factors_file) {
  std::shared_ptr<const MarketDataServer> mds(
      new MarketDataServer(risk_factors_file));
  // Init market object
  Date today(2017,8,5);
  Market mkt(mds, today);

  std::vector<std::pair<std::string, std::string>> test_cases = {
    {"EUR", "USD"}, {"USD", "EUR"}, {"GBP", "USD"}, {"USD", "GBP"}, 
    {"JPY", "USD"}, {"USD", "JPY"}, {"EUR", "JPY"}, {"GBP", "JPY"}};
  for (const auto& test_case : test_cases) {
    CurveFXSpot spot(
        &mkt, today, fx_spot_name(test_case.first, test_case.second));
    std::cout << spot.name() << ": " << spot.spot() << std::endl;
  }
}

int main(int argc, const char **argv)
{
    // parse command line arguments
    try {
        run("../data/risk_factors_5.txt");
        return 0;  // report success to the caller
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return -1; // report an error to the caller
    }
    catch (...)
    {
        std::cerr << "Unknown exception occurred\n";
        return -1; // report an error to the caller
    }
}
