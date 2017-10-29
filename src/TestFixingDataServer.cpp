#include <iostream>

#include "FixingDataServer.h"

using namespace minirisk;

void run() {
  FixingDataServer fds("../data/fixings.txt");
  MYASSERT(fds.lookup("FX.SPOT.EUR.USD", Date("20170805")).first == 1.1213, 
      "Wrong value.");
  MYASSERT(fds.lookup("FX.SPOT.EUR.GBP", Date("20170804")).first == 0.74, 
      "Wrong value.");
  MYASSERT(fds.get("FX.SPOT.EUR.USD", Date("20170804")) == 1.1213, 
      "Wrong value.");
}

int main() {
  run();
}
