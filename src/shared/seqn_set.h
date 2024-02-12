#ifndef SHARED_SEQN_SET_H_
#define SHARED_SEQN_SET_H_ 1

#include <map>
#include <set>
#include <cstdint>
#include <optional>

class SeqnSet {
    private:
        class Block {
            public:
                uint64_t start;
                uint64_t end;
        };

        class Query {
            public:
                std::optional<Block> below;
                std::optional<Block> containing;
                std::optional<Block> above;
        };

        std::map<uint64_t, uint64_t> ranges;

        Query query(uint64_t seqn) const;

    public:
        bool add(uint64_t seqn);
        
        bool remove(uint64_t seqn);

        bool contains(uint64_t seqn) const;

        std::set<uint64_t> missing_below() const;
};

#endif
