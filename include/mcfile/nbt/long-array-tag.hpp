#pragma once

namespace mcfile {
namespace nbt {

class LongArrayTag : public detail::VectorTag<int64_t, Tag::TAG_Long_Array> {
public:
    LongArrayTag() : VectorTag() {}

    explicit LongArrayTag(std::vector<int64_t>& input) : VectorTag(input) {}

    explicit LongArrayTag(size_t size) : VectorTag(size) {}

    std::shared_ptr<Tag> clone() const override {
        std::vector<int64_t> copy = value();
        return std::shared_ptr<LongArrayTag>(new LongArrayTag(copy));
    }

private:
    int64_t convert(int64_t v, bool littleEndian) const override {
        uint64_t t = *(uint64_t *)&v;
        if (littleEndian) {
            t = ::mcfile::detail::Int64FromLE(t);
        } else {
            t = ::mcfile::detail::Int64FromBE(t);
        }
        return *(int64_t *)&t;
    }
};

} // namespace nbt
} // namespace mcfile
