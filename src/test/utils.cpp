#include "utils.h"

TestFailure::TestFailure(
    std::string const& message,
    std::string const& filename,
    int line_number
):
    full_message("ASSERTION FAILED: "),
    message_(message),
    filename_(filename),
    line_number_(line_number)
{
    this->full_message += message;
    this->full_message += " (";
    this->full_message += filename;
    this->full_message += ":";
    this->full_message += std::to_string(line_number);
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

void TestCase::run()
{
    this->test();
}

void test_assert_impl(
    std::string const& message,
    bool condition,
    char const *filename,
    int line_number
)
{
    if (!condition) {
        throw TestFailure(message, filename, line_number);
    }
}

TestSuite& TestSuite::append(TestSuite const& subsuite)
{
    this->test_cases.insert(
        this->test_cases.end(),
        subsuite.test_cases.begin(),
        subsuite.test_cases.end()
    );
    return *this;
}

bool TestSuite::run()
{
    std::vector<std::string> failures;
    int successes = 0;

    std::cerr << std::endl;

    for (auto test_case : this->test_cases) {
        std::cerr << test_case.name() << "..." << std::flush;
        try {
            test_case.run();
            std::cerr << "Success" << std::endl;
            successes++;
        } catch (std::exception const& failure) {
            std::cerr
                << "Failed"
                << std::endl
                << std::endl
                << failure.what()
                << std::endl
                << std::endl;
            failures.push_back(test_case.name());
        }
    }

    std::cerr
        << std::endl
        << successes
        << " passed, "
        << failures.size()
        << " failed."
        << std::endl;

    std::cerr << std::endl;

    if (failures.empty()) {
        return true;
    }

    std::cerr << "failures:" << std::endl;

    for (auto test_name : failures) {
        std::cerr << "- " << test_name << std::endl;
    }

    std::cerr << std::endl;

    return false;
}
