#include "utils.h"

int main(int argc, char const *argv[])
{
    bool success = TestSuite()
        .run();

    if (success) {
        return 0;
    }
    return 1;
}
