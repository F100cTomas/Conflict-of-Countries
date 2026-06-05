#include "common/common.hpp"
#include <stdlib.h>

void ExampleLinuxSpecificFunction() {
    system("notify-send 'Hello' 'Hello from Linux'");
}