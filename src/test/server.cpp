#include "server.h"

static TestSuite server_data_test_suite();

TestSuite server_test_suite()
{
    return TestSuite()
        .append(server_data_test_suite())
    ;
}

static TestSuite server_data_test_suite()
{
    return TestSuite()
    ;
}
