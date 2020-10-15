#pragma once

namespace mcfile {
namespace nbt {

class ByteTag : public detail::ScalarTag<uint8_t, Tag::TAG_Byte> {
public:
    ByteTag() : ScalarTag() {}
    explicit ByteTag(uint8_t v) : ScalarTag(v) {}
};

} // namespace nbt
} // namespace mcfile
