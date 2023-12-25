#include "seqn_set.h"
#include <utility>

std::optional<SeqnSet::Query> SeqnSet::query(uint64_t seqn)
{
    auto search = this->ranges.lower_bound(seqn);
    Query query;
    if (search != this->ranges.end()) {
        uint64_t search_start = std::get<0>(*search);
        uint64_t search_end = std::get<1>(*search);
        if (search_end < seqn) {
            SeqnSet::Block below;
            below.start = search_start;
            below.end = search_end;
            query.below = std::make_optional(below);
        }
        if (search_start <= seqn) {
            return std::optional<SeqnSet::Query>();
        }
    }
    if (search != this->ranges.begin()) {
        search--;
        uint64_t search_start = std::get<0>(*search);
        uint64_t search_end = std::get<1>(*search);
        SeqnSet::Block above;
        above.start = search_start;
        above.end = search_end;
        query.above = std::make_optional(above);
    }

    return std::make_optional(query);
}

bool SeqnSet::receive(uint64_t seqn)
{
    std::optional<Query> query = this->query(seqn);
    if (!query) {
        return false;
    }
    
    uint64_t block_start = seqn;
    uint64_t block_end = seqn;
    this->ranges.insert(std::make_pair(block_start, block_end));

    if (query->above && query->above->start - 1 == block_end) {
        this->ranges.erase(query->above->start);
        block_end = query->above->end;
        this->ranges[block_start] = block_end;
    }

    if (query->below && query->below->end + 1 == block_start) {
        this->ranges.erase(block_start);
        block_start = query->below->start;
        this->ranges[block_start] = block_end;
    }
    
    return true;
}
