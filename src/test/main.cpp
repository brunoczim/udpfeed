#include "utils.h"
#include "shared.h"
#include "client.h"
#include "server.h"

int main(int argc, char const *argv[])
{
    bool success = TestSuite()
        .append(shared_test_suite())
        .append(client_test_suite())
        .append(server_test_suite())
        .run();

    if (success) {
        return 0;
    }
    return 1;
}
