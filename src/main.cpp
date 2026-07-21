#include <iostream>

#include "system_info/system_info.h"
using namespace std;
int main() {
    const AppleSystemInfo& info = AppleSystemInfo::get_instance();

    info.print_summary();

    return 0;
}
