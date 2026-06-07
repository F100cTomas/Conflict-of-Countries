#include "common/common.hpp"
#include <cstdlib>

void ExampleOSSpecificFunction() {
    system("notify-send 'Hello' 'Hello from Linux'");
}
