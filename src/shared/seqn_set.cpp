#include "seqn_set.h"
#include <utility>

SeqnSet::Query SeqnSet::query(uint64_t seqn) const
{
    Query query;
    auto search = this->ranges.lower_bound(seqn);

    if (search != this->ranges.end()) {
        uint64_t search_end = std::get<0>(*search);
        uint64_t search_start = std::get<1>(*search);

        if (search_start <= seqn) {
            SeqnSet::Block containing;
            containing.start = search_start;
            containing.end = search_end;
            query.containing = std::make_optional(containing);
        } else {
            SeqnSet::Block above;
            above.start = search_start;
            above.end = search_end;
            query.above = std::make_optional(above);
        }
    }
    if (search != this->ranges.begin()) {
        search--;
        uint64_t search_end = std::get<0>(*search);
        uint64_t search_start = std::get<1>(*search);
        SeqnSet::Block below;
        below.start = search_start;
        below.end = search_end;
        query.below = std::make_optional(below);
    }

    return query;
}

bool SeqnSet::add(uint64_t seqn)
{
    Query query = this->query(seqn);
    if (query.containing) {
        return false;
    }
    
    uint64_t block_start = seqn;
    uint64_t block_end = seqn;
    this->ranges[block_end] = block_start;

    if (query.above && query.above->start - 1 == block_end) {
        this->ranges.erase(block_end);
        block_end = query.above->end;
        this->ranges[block_end] = block_start;
    }

    if (query.below && query.below->end + 1 == block_start) {
        this->ranges.erase(query.below->end);
        block_start = query.below->start;
        this->ranges[block_end] = block_start;
    }

    return true;
}

bool SeqnSet::remove(uint64_t seqn)
{
    Query query = this->query(seqn);
    if (!query.containing) {
        return false;
    }

    if (query.containing->end == seqn) {
        this->ranges.erase(query.containing->end);
    } else {
        this->ranges[query.containing->end] = seqn + 1;
    }

    if (query.containing->start != seqn) {
        this->ranges[seqn - 1] = query.containing->start;
    }

    return true;
}

bool SeqnSet::contains(uint64_t seqn) const
{
    return this->query(seqn).containing.has_value();
}

std::set<uint64_t> SeqnSet::missing_below() const
{
    std::set<uint64_t> set;
    uint64_t previous_end = 0;
    for (auto range_entry : this->ranges) {
        uint64_t end = std::get<0>(range_entry);
        uint64_t start = std::get<1>(range_entry);

        for (uint64_t i = previous_end; i < start; i++) {
            set.insert(i);
        }

        previous_end = end + 1;
    }
    return set;
}
