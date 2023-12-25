#ifndef SHARED_SEQN_SET_H_
#define SHARED_SEQN_SET_H_ 1

#include <map>
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
                std::optional<Block> above;
        };

        std::map<uint64_t, uint64_t> ranges;

        std::optional<Query> query(uint64_t seqn);

    public:
        bool receive(uint64_t seqn);
};

#endif
