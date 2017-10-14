#include <iostream>
#include <iomanip>
#include <cstdint>

using namespace std;

//void out_char_as_hex(int c)
//{
//    cout << hex << setw(2) << setfill('0') << c;
//}

int main()
{
    union { double d; uint64_t u; } tmp;
    double x = -0.15625;
    tmp.d = x;
    cout << hex << tmp.u << endl;
    return 0;
}
