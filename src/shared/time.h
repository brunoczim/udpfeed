#ifndef SHARED_TIME_H_
#define SHARED_TIME_H_ 1

#include <string>
#include <optional>
#include <cstdint>

class ReportTime {
    public:
        enum Unit {
            NS,
            US,
            MS,
            S
        };

    private:
        Unit unit_;
        double value_;
   
    public:
        ReportTime(uint64_t value, Unit unit);

        Unit unit() const;
        double value() const;

        std::string to_string() const;

        static std::optional<Unit> inc_unit(Unit unit);
};

#endif
