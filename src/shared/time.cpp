#include "time.h"
#include <sstream>

ReportTime::ReportTime(uint64_t value, ReportTime::Unit unit) :
    value_(value),
    unit_(unit)
{
    while (this->value_ >= 1000.0) {
        std::optional<Unit> next_unit = ReportTime::inc_unit(this->unit_);
        if (next_unit) {
            this->value_ /= 1000.0;
            this->unit_ = *next_unit;
        } else {
            break;
        }
    }
}

ReportTime::Unit ReportTime::unit() const
{
    return this->unit_;
}

double ReportTime::value() const
{
    return this->value_;
}

std::string ReportTime::to_string() const
{
    std::stringstream sstream;
    sstream << std::fixed;
    sstream.precision(3);
    sstream << this->value();
    switch (this->unit()) {
        case ReportTime::NS:
            sstream << "ns";
            break;
        case ReportTime::US:
            sstream << "us";
            break;
        case ReportTime::MS:
            sstream << "ms";
            break;
        case ReportTime::S:
            sstream << "s";
            break;
    }
    return sstream.str();
}

std::optional<ReportTime::Unit> ReportTime::inc_unit(ReportTime::Unit unit)
{
    switch (unit) {
        case ReportTime::NS: return std::make_optional(ReportTime::US);
        case ReportTime::US: return std::make_optional(ReportTime::MS);
        case ReportTime::MS: return std::make_optional(ReportTime::S);
        case ReportTime::S: return std::nullopt;
        default: return std::nullopt;
    }
}
