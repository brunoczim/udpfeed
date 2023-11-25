#ifndef TEST_UTILS_H_
#define TEST_UTILS_H_ 1

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <optional>
#include <functional>

#define TEST_ASSERT(msg, cond) \
    test_assert_impl((msg), (cond), __FILE__, __LINE__)

class TestFailure : public std::exception {
    private:
        std::string full_message;
        std::string message_;
        std::string filename_;
        int line_number_;

    public:
        TestFailure(
            std::string const& message,
            std::string const& filename,
            int line_number
        );

        std::string message() const;
        std::string filename() const;
        int line_number() const;

        virtual char const *what() const noexcept;
};

class TestCase {
    private:
        std::string name_;
        std::function<void ()> test;

    public:
        template <typename F>
        TestCase(std::string const& name, F test);

        std::string const& name() const;

        void run();
};

class TestSuite {
    private:
        std::vector<TestCase> test_cases;

    public:
        template <typename F>
        TestSuite& test(std::string const& name, F test);

        TestSuite& append(TestSuite const& subsuite);

        bool run();
};

void test_assert_impl(
    std::string const& message,
    bool condition,
    char const *filename,
    int line_number
);

template <typename F>
TestCase::TestCase(std::string const& name, F test) :
    name_(name),
    test(test)
{
}

template <typename F>
TestSuite& TestSuite::test(std::string const& name, F test)
{
    this->test_cases.push_back(TestCase(name, test));
    return *this;
}

#endif
