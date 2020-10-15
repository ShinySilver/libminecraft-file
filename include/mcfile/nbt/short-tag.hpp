#pragma once

namespace mcfile {
namespace nbt {

class ShortTag : public detail::ScalarTag<int16_t, Tag::TAG_Short> {
public:
    ShortTag() : ScalarTag() {}
    explicit ShortTag(int16_t v) : ScalarTag(v) {}
};

} // namespace nbt
} // namespace mcfile
