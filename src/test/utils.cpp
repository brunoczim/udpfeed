#include "utils.h"

TestFailure::TestFailure(
    std::string const& message,
    std::string const& filename,
    int line_number
):
    full_message(message),
    message_(message),
    filename_(filename),
    line_number_(line_number)
{
    this->full_message += "(file = ";
    this->full_message += filename;
    this->full_message += ", line = ";
    this->full_message += line_number;
    this->full_message += ")";
}

std::string TestFailure::message() const
{
    return this->message_;
}

std::string TestFailure::filename() const
{
    return this->filename_;
}

int TestFailure::line_number() const
{
    return this->line_number_;
}

const char *TestFailure::what() const noexcept
{
    return this->full_message.c_str();
}


std::string const& TestCase::name() const
{
    return this->name_;
}

void  TestCase::run()
{
    this->test();
}

void test_assert_impl(
    bool condition,
    std::string const& message,
    char const *filename,
    int line_number
)
{
    if (!condition) {
        throw TestFailure(message, filename, line_number);
    }
}

bool TestSuite::run()
{
    std::vector<std::string> failures;
    int successes = 0;
    for (auto test_case : this->test_cases) {
        std::cerr << test_case.name() << "..." << std::flush;
        try {
            std::cerr << "Success" << std::endl;
            successes++;
        } catch (TestFailure const& failure) {
            std::cerr
                << "Failed"
                << std::endl
                << "-----------------------------------------------"
                << std::endl
                << failure.what()
                << std::endl
                << "-----------------------------------------------"
                << std::endl;
            failures.push_back(test_case.name());
        }
    }

    std::cerr
        << "-----------------------------------------------"
        << std::endl
        << "-----------------------------------------------"
        << std::endl
        << successes
        << " passed, "
        << failures.size()
        << " failed."
        << std::endl;

    if (failures.empty()) {
        return true;
    }

    std::cerr
        << "failures:"
        << std::endl;

    for (auto test_name : failures) {
        std::cerr << "- " << test_name << std::endl;
    }

    return false;
}
