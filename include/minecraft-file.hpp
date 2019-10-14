#pragma once

/// @file

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <cstdint>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include <sstream>
#include <limits>
#include <filesystem>

#if defined(__APPLE__)
#    include <libkern/OSByteOrder.h>
#elif defined(__linux__) || defined(__CYGWIN__)
#    include <endian.h>
#endif

namespace mcfile {

namespace detail {

class Stream {
public:
    Stream() = default;

    Stream(Stream const &) = delete;

    Stream &operator=(Stream const &) = delete;

    virtual ~Stream() {}

    virtual long length() const = 0;

    virtual bool read(void *buffer, size_t size, size_t count) = 0;

    virtual bool seek(long offset) = 0;

    virtual bool valid() const = 0;

    virtual long pos() const = 0;
};


class FileStream : public Stream {
public:
    explicit FileStream(std::string const &filePath)
            : fFile(nullptr), fLoc(0) {
        FILE *fp = fopen(filePath.c_str(), "rb");

        if (!fp) {
            return;
        }
        if (fseek(fp, 0, SEEK_END) != 0) {
            fclose(fp);
            return;
        }
        long length = ftell(fp);
        if (length == -1L) {
            fclose(fp);
            return;
        }
        if (fseek(fp, 0, SEEK_SET) != 0) {
            fclose(fp);
            return;
        }
        this->fFile = fp;
        this->fLength = length;
    }

    ~FileStream() {
        if (fFile) {
            fclose(fFile);
        }
    }

    FileStream(FileStream const &) = delete;

    FileStream &operator=(FileStream const &) = delete;

    bool read(void *buffer, size_t size, size_t count) override {
        if (!fFile) {
            return false;
        }
        fLoc += size * count;
        return fread(buffer, size, count, fFile) == count;
    }

    bool seek(long offset) override {
        if (!fFile) {
            return false;
        }
        fLoc = offset;
        return fseek(fFile, offset, SEEK_SET) == 0;
    }

    bool valid() const override {
        return fFile != nullptr;
    }

    long length() const override { return fLength; }

    long pos() const override { return fLoc; }

    static bool copy(FILE *in, FILE *out, size_t length) {
        if (!in || !out) {
            return false;
        }
        if (length == 0) {
            return true;
        }
        uint8_t buffer[512]= { 0 };
        size_t const size = sizeof(buffer[0]);
        size_t const nitems = sizeof(buffer) / size;
        size_t remain = length;
        while (remain > 0) {
            size_t const num = std::min(nitems, remain);
            size_t const read = fread(buffer, size, num, in);
            if (read != num) {
                return false;
            }
            size_t const written = fwrite(buffer, size, read, out);
            if (written != read) {
                return false;
            }
            assert(remain >= read);
            remain -= read;
        }
        return true;
    }
    
private:
    FILE *fFile;
    long fLength;
    long fLoc;
};


class ByteStream : public Stream {
public:
    explicit ByteStream(std::vector<uint8_t> &buffer)
            : fLoc(0) {
        this->fBuffer.swap(buffer);
    }

    ~ByteStream() {}

    ByteStream(ByteStream const &) = delete;

    ByteStream &operator=(ByteStream const &) = delete;

    bool read(void *buf, size_t size, size_t count) override {
        if (fBuffer.size() <= fLoc + size * count) {
            return false;
        }
        std::copy(fBuffer.begin() + fLoc, fBuffer.begin() + fLoc + size * count, (uint8_t *) buf);
        fLoc += size * count;
        return true;
    }

    bool seek(long offset) override {
        if (offset < 0 || fBuffer.size() <= offset) {
            return false;
        }
        fLoc = offset;
        return true;
    }

    long length() const override { return fBuffer.size(); }

    bool valid() const override { return true; }

    long pos() const override { return fLoc; }

private:
    std::vector<uint8_t> fBuffer;
    long fLoc;
};


class StreamReader {
public:
    explicit StreamReader(std::shared_ptr<Stream> stream)
            : fStream(stream) {
    }

    StreamReader(StreamReader const &) = delete;

    StreamReader &operator=(StreamReader const &) = delete;

    bool valid() const {
        if (!fStream) {
            return false;
        }
        return fStream->valid();
    }

    bool seek(int64_t pos) {
        return fStream->seek(pos);
    }

    bool read(uint8_t *v) {
        return fStream->read(v, sizeof(uint8_t), 1);
    }

    bool read(int8_t *v) {
        return fStream->read(v, sizeof(int8_t), 1);
    }

    bool read(int16_t *v) {
        uint16_t t;
        if (!readRaw(&t)) {
            return false;
        }
        t = Int16FromBE(t);
        *v = *(int16_t *) &t;
        return true;
    }

    bool read(uint16_t *v) {
        uint16_t t;
        if (!readRaw(&t)) {
            return false;
        }
        *v = Int16FromBE(t);
        return true;
    }

    bool read(int32_t *v) {
        uint32_t t;
        if (!readRaw(&t)) {
            return false;
        }
        t = Int32FromBE(t);
        *v = *(int32_t *) &t;
        return true;
    }

    bool read(uint32_t *v) {
        uint32_t t;
        if (!readRaw(&t)) {
            return false;
        }
        *v = Int32FromBE(t);
        return true;
    }

    bool read(int64_t *v) {
        uint64_t t;
        if (!readRaw(&t)) {
            return false;
        }
        t = Int64FromBE(t);
        *v = *(int64_t *) &t;
        return true;
    }

    bool read(uint64_t *v) {
        uint64_t t;
        if (!readRaw(&t)) {
            return false;
        }
        *v = Int64FromBE(t);
        return true;
    }

    bool read(std::vector<uint8_t> &buffer) {
        size_t const count = buffer.size();
        return fStream->read(buffer.data(), sizeof(uint8_t), count);
    }

    template<typename T>
    bool copy(std::vector<T>& buffer) {
        return fStream->read(buffer.data(), sizeof(T), buffer.size());
    }

    bool read(std::string &s) {
        uint16_t length;
        if (!read(&length)) {
            return false;
        }
        std::vector<uint8_t> buffer(length);
        if (!read(buffer)) {
            return false;
        }
        buffer.push_back(0);
        std::string tmp((char const *) buffer.data());
        s.swap(tmp);
        return true;
    }

    long length() const {
        if (!fStream) {
            return 0;
        }
        return fStream->length();
    }

    long pos() const {
        if (!fStream) {
            return 0;
        }
        return fStream->pos();
    }

    static uint64_t Int64FromBE(uint64_t v) {
        #if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
            return v;
        #else
            return SwapInt64(v);
        #endif
    }

    static uint32_t Int32FromBE(uint32_t v) {
        #if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
            return v;
        #else
            return SwapInt32(v);
        #endif
    }

    static uint16_t Int16FromBE(uint16_t v) {
        #if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
            return v;
        #else
            return SwapInt16(v);
        #endif
    }

private:
    template<typename T>
    bool readRaw(T *v) {
        return fStream->read(v, sizeof(T), 1);
    }

    static uint64_t SwapInt64(uint64_t v) {
        return ((v & 0x00000000000000ffLL) << 56)
               | ((v & 0x000000000000ff00LL) << 40)
               | ((v & 0x0000000000ff0000LL) << 24)
               | ((v & 0x00000000ff000000LL) << 8)
               | ((v & 0x000000ff00000000LL) >> 8)
               | ((v & 0x0000ff0000000000LL) >> 24)
               | ((v & 0x00ff000000000000LL) >> 40)
               | ((v & 0xff00000000000000LL) >> 56);
    }

    static uint32_t SwapInt32(uint32_t v) {
        uint32_t r;
        r = v << 24;
        r |= (v & 0x0000FF00) << 8;
        r |= (v & 0x00FF0000) >> 8;
        r |= v >> 24;
        return r;
    }

    static uint16_t SwapInt16(uint16_t v) {
        uint16_t r;
        r = v << 8;
        r |= (v & 0xFF00) >> 8;
        return r;
    }

private:
    std::shared_ptr<Stream> fStream;
};


class Compression {
public:
    Compression() = delete;

    static bool compress(std::vector<uint8_t> &inout) {
        z_stream zs;
        char buff[kSegSize];
        std::vector<uint8_t> outData;

        memset(&zs, 0, sizeof(zs));
        if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) {
            return false;
        }
        zs.next_in = (Bytef *) inout.data();
        zs.avail_in = inout.size();

        int ret;
        do {
            zs.next_out = reinterpret_cast<Bytef *>(buff);
            zs.avail_out = kSegSize;

            ret = deflate(&zs, Z_FINISH);
            outData.insert(outData.end(), buff, buff + zs.total_out);
        } while (ret == Z_OK);

        int r = deflateEnd(&zs);
        if (ret != Z_STREAM_END) {
            return false;
        }
        if (r != Z_OK) {
            return false;
        }

        inout.swap(outData);
        return true;
    }

    static bool decompress(std::vector<uint8_t> &inout) {
        int ret;
        z_stream zs;
        char buff[kSegSize];
        std::vector<uint8_t> outData;
        unsigned long prevOut = 0;

        memset(&zs, 0, sizeof(zs));
        if (inflateInit(&zs) != Z_OK) {
            return false;
        }

        zs.next_in = (Bytef *) inout.data();
        zs.avail_in = inout.size();

        do {
            zs.next_out = reinterpret_cast<Bytef *>(buff);
            zs.avail_out = kSegSize;

            ret = inflate(&zs, 0);
            outData.insert(outData.end(), buff, buff + (zs.total_out - prevOut));
            prevOut = zs.total_out;
        } while (ret == Z_OK);

        inflateEnd(&zs);
        if (ret != Z_STREAM_END) {
            return false;
        }

        inout.swap(outData);
        return true;
    }

private:
    static const unsigned int kSegSize = 16384;
};


class String {
public:
    String() = delete;

    static std::vector<std::string> Split(std::string const &sentence, char delimiter) {
        std::istringstream input(sentence);
        std::vector<std::string> tokens;
        for (std::string line; std::getline(input, line, delimiter);) {
            tokens.push_back(line);
        }
        return tokens;
    }
};

} // namespace detail

namespace nbt {

class Tag;

class TagFactory {
public:
    static inline std::shared_ptr<Tag> makeTag(uint8_t id, std::string const& name);

private:
    template<typename T>
    static std::shared_ptr<T> makeNamedTag(std::string const& name) {
        auto p = std::make_shared<T>();
        p->fName = name;
        return p;
    }
};

class EndTag;
class ByteTag;
class ShortTag;
class IntTag;
class LongTag;
class FloatTag;
class DoubleTag;
class ByteArrayTag;
class StringTag;
class ListTag;
class CompoundTag;
class IntArrayTag;
class LongArrayTag;

class Tag {
public:
    friend class TagFactory;

    Tag() : fValid(false) {}
    virtual ~Tag() {}

    Tag(Tag const&) = delete;
    Tag& operator = (Tag const&) = delete;

    void read(::mcfile::detail::StreamReader& reader) {
        fValid = readImpl(reader);
    }

    bool valid() const { return fValid; }

    virtual uint8_t id() const = 0;

    std::string name() const { return fName; }

    EndTag       const* asEnd() const       { return this->id() == TAG_End ? reinterpret_cast<EndTag const*>(this) : nullptr; }
    ByteTag      const* asByte() const      { return this->id() == TAG_Byte ? reinterpret_cast<ByteTag const*>(this) : nullptr; }
    ShortTag     const* asShort() const     { return this->id() == TAG_Short ? reinterpret_cast<ShortTag const*>(this) : nullptr; }
    IntTag       const* asInt() const       { return this->id() == TAG_Int ? reinterpret_cast<IntTag const*>(this) : nullptr; }
    LongTag      const* asLong() const      { return this->id() == TAG_Long ? reinterpret_cast<LongTag const*>(this) : nullptr; }
    FloatTag     const* asFloat() const     { return this->id() == TAG_Float ? reinterpret_cast<FloatTag const*>(this) : nullptr; }
    DoubleTag    const* asDouble() const    { return this->id() == TAG_Double ? reinterpret_cast<DoubleTag const*>(this) : nullptr; }
    ByteArrayTag const* asByteArray() const { return this->id() == TAG_Byte_Array ? reinterpret_cast<ByteArrayTag const*>(this) : nullptr; }
    StringTag    const* asString() const    { return this->id() == TAG_String ? reinterpret_cast<StringTag const*>(this) : nullptr; }
    ListTag      const* asList() const      { return this->id() == TAG_List ? reinterpret_cast<ListTag const*>(this) : nullptr; }
    CompoundTag  const* asCompound() const  { return this->id() == TAG_Compound ? reinterpret_cast<CompoundTag const*>(this) : nullptr; }
    IntArrayTag  const* asIntArray() const  { return this->id() == TAG_Int_Array ? reinterpret_cast<IntArrayTag const*>(this) : nullptr; }
    LongArrayTag const* asLongArray() const { return this->id() == TAG_Long_Array ? reinterpret_cast<LongArrayTag const*>(this) : nullptr; }

protected:
    virtual bool readImpl(::mcfile::detail::StreamReader& reader) = 0;

public:
    static uint8_t const TAG_End = 0;
    static uint8_t const TAG_Byte = 1;
    static uint8_t const TAG_Short = 2;
    static uint8_t const TAG_Int = 3;
    static uint8_t const TAG_Long = 4;
    static uint8_t const TAG_Float = 5;
    static uint8_t const TAG_Double = 6;
    static uint8_t const TAG_Byte_Array = 7;
    static uint8_t const TAG_String = 8;
    static uint8_t const TAG_List = 9;
    static uint8_t const TAG_Compound = 10;
    static uint8_t const TAG_Int_Array = 11;
    static uint8_t const TAG_Long_Array = 12;

protected:
    std::string fName;

private:
    bool fValid;
};


class EndTag : public Tag {
public:
    bool readImpl(::mcfile::detail::StreamReader&) override { return true; }
    uint8_t id() const override { return TAG_End; }

    static EndTag const* instance() {
        static EndTag s;
        return &s;
    }
};

namespace detail {

template< typename T, uint8_t ID>
class ScalarTag : public Tag {
public:
    bool readImpl(::mcfile::detail::StreamReader& r) override {
        T v;
        if (!r.read(&v)) {
            return false;
        }
        fValue = v;
        return true;
    }

    uint8_t id() const override { return ID; }

public:
    T fValue;
};

} // namespace detail

class ByteTag : public detail::ScalarTag<uint8_t, Tag::TAG_Byte> {};
class ShortTag : public detail::ScalarTag<int16_t, Tag::TAG_Short> {};
class IntTag : public detail::ScalarTag<int32_t, Tag::TAG_Int> {};
class LongTag : public detail::ScalarTag<int64_t, Tag::TAG_Long> {};


class FloatTag : public Tag {
public:
    bool readImpl(::mcfile::detail::StreamReader& r) override {
        uint32_t v;
        if (!r.read(&v)) {
            return false;
        }
        fValue = *(float *)&v;
        return true;
    }

    uint8_t id() const override { return Tag::TAG_Float; }

public:
    float fValue;
};


class DoubleTag : public Tag {
public:
    bool readImpl(::mcfile::detail::StreamReader& r) override {
        uint64_t v;
        if (!r.read(&v)) {
            return false;
        }
        fValue = *(double *)&v;
        return true;
    }

    uint8_t id() const override { return Tag::TAG_Double; }

public:
    double fValue;
};

namespace detail {

template<typename T, uint8_t ID>
class VectorTag : public Tag {
public:
    VectorTag()
        : Tag()
        , fPrepared(false)
    {
    }

    bool readImpl(::mcfile::detail::StreamReader& r) override {
        uint32_t length;
        if (!r.read(&length)) {
            return false;
        }
        fValue.resize(length);
        if (!r.copy(fValue)) {
            return false;
        }
        return true;
    }

    uint8_t id() const override { return ID; }

    std::vector<T> const& value() const {
        if (!fPrepared) {
            for (size_t i = 0; i < fValue.size(); i++) {
                fValue[i] = convert(fValue[i]);
            }
            fPrepared = true;
        }
        return fValue;
    }

protected:
    virtual T convert(T v) const = 0;

private:
    std::vector<T> mutable fValue;
    bool mutable fPrepared = false;
};

} // namespace detail

class ByteArrayTag : public detail::VectorTag<uint8_t, Tag::TAG_Byte_Array> {
private:
    uint8_t convert(uint8_t v) const override { return v; }
};


class IntArrayTag : public detail::VectorTag<int32_t, Tag::TAG_Int_Array> {
private:
    int32_t convert(int32_t v) const override {
        uint32_t t = *(uint32_t *)&v;
        t = ::mcfile::detail::StreamReader::Int32FromBE(t);
        return *(int32_t *)&t;
    }
};


class LongArrayTag : public detail::VectorTag<int64_t, Tag::TAG_Long_Array> {
private:
    int64_t convert(int64_t v) const override {
        uint64_t t = *(uint64_t *)&v;
        t = ::mcfile::detail::StreamReader::Int64FromBE(t);
        return *(int64_t *)&t;
    }
};


class StringTag : public Tag {
public:
    bool readImpl(::mcfile::detail::StreamReader& r) override {
        std::string tmp;
        if (!r.read(tmp)) {
            return false;
        }
        fValue = tmp;
        return true;
    }

    uint8_t id() const override { return Tag::TAG_String; }

public:
    std::string fValue;
};


class ListTag : public Tag {
public:
    bool readImpl(::mcfile::detail::StreamReader& r) override {
        uint8_t type;
        if (!r.read(&type)) {
            return false;
        }
        int32_t size;
        if (!r.read(&size)) {
            return false;
        }
        std::vector<std::shared_ptr<Tag>> tmp;
        for (int32_t i = 0; i < size; i++) {
            auto tag = TagFactory::makeTag(type, "");
            tag->read(r);
            if (!tag->valid()) {
                return false;
            }
            tmp.push_back(tag);
        }
        tmp.swap(fValue);
        return true;
    }

    uint8_t id() const override { return Tag::TAG_List; }

public:
    std::vector<std::shared_ptr<Tag>> fValue;
};


class CompoundTag : public Tag {
public:
    bool readImpl(::mcfile::detail::StreamReader& r) override {
        std::map<std::string, std::shared_ptr<Tag>> tmp;
        while (r.pos() < r.length()) {
            uint8_t type;
            if (!r.read(&type)) {
                break;
            }
            if (type == Tag::TAG_End) {
                break;
            }

            std::string name;
            if (!r.read(name)) {
                return false;
            }

            auto tag = TagFactory::makeTag(type, name);
            tag->read(r);
            if (!tag->valid()) {
                return false;
            }
            tmp.insert(std::make_pair(name, tag));
        }
        fValue.swap(tmp);
        return true;
    }

    uint8_t id() const override { return Tag::TAG_Compound; }

    Tag const* query(std::string const& path) const {
        // path: /Level/Sections
        if (path.empty()) {
            return EndTag::instance();
        }
        std::string p = path;
        CompoundTag const* pivot = this;
        while (!p.empty()) {
            if (p[0] == '/') {
                if (fValue.size() != 1) {
                    return EndTag::instance();
                }
                auto child = fValue.begin()->second;
                if (p.size() == 1) {
                    return child.get();
                }
                if (child->id() != Tag::TAG_Compound) {
                    return EndTag::instance();
                }
                pivot = child->asCompound();
                p = p.substr(1);
            } else {
                auto pos = p.find_first_of('/');
                std::string name;
                if (pos == std::string::npos) {
                    name = p;
                } else {
                    name = p.substr(0, pos);
                }
                auto child = pivot->fValue.find(name);
                if (child == pivot->fValue.end()) {
                    return EndTag::instance();
                }
                if (pos == std::string::npos) {
                    return child->second.get();
                }
                if (child->second->id() != Tag::TAG_Compound) {
                    return EndTag::instance();
                }
                pivot = child->second->asCompound();
                p = p.substr(pos + 1);
            }
        }
        return EndTag::instance();
    }

public:
    std::map<std::string, std::shared_ptr<Tag>> fValue;
};


inline std::shared_ptr<Tag> TagFactory::makeTag(uint8_t id, std::string const& name) {
    switch (id) {
        case Tag::TAG_End:
            return makeNamedTag<EndTag>(name);
        case Tag::TAG_Byte:
            return makeNamedTag<ByteTag>(name);
        case Tag::TAG_Short:
            return makeNamedTag<ShortTag>(name);
        case Tag::TAG_Int:
            return makeNamedTag<IntTag>(name);
        case Tag::TAG_Long:
            return makeNamedTag<LongTag>(name);
        case Tag::TAG_Float:
            return makeNamedTag<FloatTag>(name);
        case Tag::TAG_Double:
            return makeNamedTag<DoubleTag>(name);
        case Tag::TAG_Byte_Array:
            return makeNamedTag<ByteArrayTag>(name);
        case Tag::TAG_String:
            return makeNamedTag<StringTag>(name);
        case Tag::TAG_List:
            return makeNamedTag<ListTag>(name);
        case Tag::TAG_Compound:
            return makeNamedTag<CompoundTag>(name);
        case Tag::TAG_Int_Array:
            return makeNamedTag<IntArrayTag>(name);
        case Tag::TAG_Long_Array:
            return makeNamedTag<LongArrayTag>(name);
        default:
            return nullptr;
    }
    return nullptr;
}

} // namespace nbt


class Coordinate {
private:
    Coordinate() = delete;

public:
    static int ChunkFromBlock(int block) {
        return block >> 4;
    }

    static int RegionFromChunk(int region) {
        return region >> 5;
    }

    static int RegionFromBlock(int block) {
        return block >> 9;
    }
};

namespace blocks {

/// @typedef BlockId
/// @brief BlockId
using BlockId = uint32_t;

enum : BlockId {
    unknown = 0,
};

namespace minecraft {

enum : BlockId {
    acacia_button = 1,
    acacia_door,
    acacia_fence,
    acacia_fence_gate,
    acacia_leaves,
    acacia_log,
    acacia_planks,
    acacia_pressure_plate,
    acacia_sapling,
    acacia_sign,
    acacia_slab,
    acacia_stairs,
    acacia_trapdoor,
    acacia_wall_sign,
    acacia_wood,
    activator_rail,
    air,
    allium,
    andesite,
    andesite_slab,
    andesite_stairs,
    andesite_wall,
    anvil,
    attached_melon_stem,
    attached_pumpkin_stem,
    azure_bluet,
    bamboo,
    bamboo_sapling,
    barrel,
    barrier,
    beacon,
    bedrock,
    beetroots,
    bell,
    birch_button,
    birch_door,
    birch_fence,
    birch_fence_gate,
    birch_leaves,
    birch_log,
    birch_planks,
    birch_pressure_plate,
    birch_sapling,
    birch_sign,
    birch_slab,
    birch_stairs,
    birch_trapdoor,
    birch_wall_sign,
    birch_wood,
    black_banner,
    black_bed,
    black_carpet,
    black_concrete,
    black_concrete_powder,
    black_glazed_terracotta,
    black_shulker_box,
    black_stained_glass,
    black_stained_glass_pane,
    black_terracotta,
    black_wall_banner,
    black_wool,
    blast_furnace,
    coal_block,
    diamond_block,
    emerald_block,
    gold_block,
    iron_block,
    quartz_block,
    redstone_block,
    blue_banner,
    blue_bed,
    blue_carpet,
    blue_concrete,
    blue_concrete_powder,
    blue_glazed_terracotta,
    blue_ice,
    blue_orchid,
    blue_shulker_box,
    blue_stained_glass,
    blue_stained_glass_pane,
    blue_terracotta,
    blue_wall_banner,
    blue_wool,
    bone_block,
    bookshelf,
    brain_coral,
    brain_coral_block,
    brain_coral_fan,
    brain_coral_wall_fan,
    brewing_stand,
    brick_slab,
    brick_stairs,
    brick_wall,
    bricks,
    brown_banner,
    brown_bed,
    brown_carpet,
    brown_concrete,
    brown_concrete_powder,
    brown_glazed_terracotta,
    brown_mushroom,
    brown_mushroom_block,
    brown_shulker_box,
    brown_stained_glass,
    brown_stained_glass_pane,
    brown_terracotta,
    brown_wall_banner,
    brown_wool,
    bubble_column,
    bubble_coral,
    bubble_coral_block,
    bubble_coral_fan,
    bubble_coral_wall_fan,
    cactus,
    cake,
    campfire,
    carrots,
    cartography_table,
    carved_pumpkin,
    cauldron,
    cave_air,
    chain_command_block,
    chest,
    chipped_anvil,
    chiseled_quartz_block,
    chiseled_red_sandstone,
    chiseled_sandstone,
    chiseled_stone_bricks,
    chorus_flower,
    chorus_plant,
    clay,
    coal_ore,
    coarse_dirt,
    cobblestone,
    cobblestone_slab,
    cobblestone_stairs,
    cobblestone_wall,
    cobweb,
    cocoa,
    command_block,
    composter,
    conduit,
    cornflower,
    cracked_stone_bricks,
    crafting_table,
    creeper_head,
    creeper_wall_head,
    cut_red_sandstone,
    cut_red_sandstone_slab,
    cut_sandstone,
    cut_sandstone_slab,
    cyan_banner,
    cyan_bed,
    cyan_carpet,
    cyan_concrete,
    cyan_concrete_powder,
    cyan_glazed_terracotta,
    cyan_shulker_box,
    cyan_stained_glass,
    cyan_stained_glass_pane,
    cyan_terracotta,
    cyan_wall_banner,
    cyan_wool,
    damaged_anvil,
    dandelion,
    dark_oak_button,
    dark_oak_door,
    dark_oak_fence,
    dark_oak_fence_gate,
    dark_oak_leaves,
    dark_oak_log,
    dark_oak_planks,
    dark_oak_pressure_plate,
    dark_oak_sapling,
    dark_oak_sign,
    dark_oak_slab,
    dark_oak_stairs,
    dark_oak_trapdoor,
    dark_oak_wall_sign,
    dark_oak_wood,
    dark_prismarine,
    dark_prismarine_slab,
    dark_prismarine_stairs,
    daylight_detector,
    dead_brain_coral,
    dead_brain_coral_block,
    dead_brain_coral_fan,
    dead_brain_coral_wall_fan,
    dead_bubble_coral,
    dead_bubble_coral_block,
    dead_bubble_coral_fan,
    dead_bubble_coral_wall_fan,
    dead_bush,
    dead_fire_coral,
    dead_fire_coral_block,
    dead_fire_coral_fan,
    dead_fire_coral_wall_fan,
    dead_horn_coral,
    dead_horn_coral_block,
    dead_horn_coral_fan,
    dead_horn_coral_wall_fan,
    dead_tube_coral,
    dead_tube_coral_block,
    dead_tube_coral_fan,
    dead_tube_coral_wall_fan,
    detector_rail,
    diamond_ore,
    diorite,
    diorite_slab,
    diorite_stairs,
    diorite_wall,
    dirt,
    dispenser,
    dragon_egg,
    dragon_head,
    dragon_wall_head,
    dried_kelp_block,
    dropper,
    emerald_ore,
    enchanting_table,
    end_gateway,
    end_portal,
    end_portal_frame,
    end_rod,
    end_stone,
    end_stone_brick_slab,
    end_stone_brick_stairs,
    end_stone_brick_wall,
    end_stone_bricks,
    ender_chest,
    farmland,
    fern,
    fire,
    fire_coral,
    fire_coral_block,
    fire_coral_fan,
    fire_coral_wall_fan,
    fletching_table,
    flower_pot,
    frosted_ice,
    furnace,
    glass,
    glass_pane,
    glowstone,
    gold_ore,
    granite,
    granite_slab,
    granite_stairs,
    granite_wall,
    grass,
    grass_block,
    grass_path,
    gravel,
    gray_banner,
    gray_bed,
    gray_carpet,
    gray_concrete,
    gray_concrete_powder,
    gray_glazed_terracotta,
    gray_shulker_box,
    gray_stained_glass,
    gray_stained_glass_pane,
    gray_terracotta,
    gray_wall_banner,
    gray_wool,
    green_banner,
    green_bed,
    green_carpet,
    green_concrete,
    green_concrete_powder,
    green_glazed_terracotta,
    green_shulker_box,
    green_stained_glass,
    green_stained_glass_pane,
    green_terracotta,
    green_wall_banner,
    green_wool,
    grindstone,
    hay_block,
    heavy_weighted_pressure_plate,
    hopper,
    horn_coral,
    horn_coral_block,
    horn_coral_fan,
    horn_coral_wall_fan,
    ice,
    infested_chiseled_stone_bricks,
    infested_cobblestone,
    infested_cracked_stone_bricks,
    infested_mossy_stone_bricks,
    infested_stone,
    infested_stone_bricks,
    iron_bars,
    iron_door,
    iron_ore,
    iron_trapdoor,
    jack_o_lantern,
    jigsaw,
    jukebox,
    jungle_button,
    jungle_door,
    jungle_fence,
    jungle_fence_gate,
    jungle_leaves,
    jungle_log,
    jungle_planks,
    jungle_pressure_plate,
    jungle_sapling,
    jungle_sign,
    jungle_slab,
    jungle_stairs,
    jungle_trapdoor,
    jungle_wall_sign,
    jungle_wood,
    kelp,
    kelp_plant,
    ladder,
    lantern,
    lapis_block,
    lapis_ore,
    large_fern,
    lava,
    lectern,
    lever,
    light_blue_banner,
    light_blue_bed,
    light_blue_carpet,
    light_blue_concrete,
    light_blue_concrete_powder,
    light_blue_glazed_terracotta,
    light_blue_shulker_box,
    light_blue_stained_glass,
    light_blue_stained_glass_pane,
    light_blue_terracotta,
    light_blue_wall_banner,
    light_blue_wool,
    light_gray_banner,
    light_gray_bed,
    light_gray_carpet,
    light_gray_concrete,
    light_gray_concrete_powder,
    light_gray_glazed_terracotta,
    light_gray_shulker_box,
    light_gray_stained_glass,
    light_gray_stained_glass_pane,
    light_gray_terracotta,
    light_gray_wall_banner,
    light_gray_wool,
    light_weighted_pressure_plate,
    lilac,
    lily_of_the_valley,
    lily_pad,
    lime_banner,
    lime_bed,
    lime_carpet,
    lime_concrete,
    lime_concrete_powder,
    lime_glazed_terracotta,
    lime_shulker_box,
    lime_stained_glass,
    lime_stained_glass_pane,
    lime_terracotta,
    lime_wall_banner,
    lime_wool,
    loom,
    magenta_banner,
    magenta_bed,
    magenta_carpet,
    magenta_concrete,
    magenta_concrete_powder,
    magenta_glazed_terracotta,
    magenta_shulker_box,
    magenta_stained_glass,
    magenta_stained_glass_pane,
    magenta_terracotta,
    magenta_wall_banner,
    magenta_wool,
    magma_block,
    melon,
    melon_stem,
    mossy_cobblestone,
    mossy_cobblestone_slab,
    mossy_cobblestone_stairs,
    mossy_cobblestone_wall,
    mossy_stone_brick_slab,
    mossy_stone_brick_stairs,
    mossy_stone_brick_wall,
    mossy_stone_bricks,
    moving_piston,
    mushroom_stem,
    mycelium,
    nether_brick_fence,
    nether_brick_slab,
    nether_brick_stairs,
    nether_brick_wall,
    nether_bricks,
    nether_portal,
    nether_quartz_ore,
    nether_wart,
    nether_wart_block,
    netherrack,
    note_block,
    oak_button,
    oak_door,
    oak_fence,
    oak_fence_gate,
    oak_leaves,
    oak_log,
    oak_planks,
    oak_pressure_plate,
    oak_sapling,
    oak_sign,
    oak_slab,
    oak_stairs,
    oak_trapdoor,
    oak_wall_sign,
    oak_wood,
    observer,
    obsidian,
    orange_banner,
    orange_bed,
    orange_carpet,
    orange_concrete,
    orange_concrete_powder,
    orange_glazed_terracotta,
    orange_shulker_box,
    orange_stained_glass,
    orange_stained_glass_pane,
    orange_terracotta,
    orange_tulip,
    orange_wall_banner,
    orange_wool,
    oxeye_daisy,
    packed_ice,
    peony,
    petrified_oak_slab,
    pink_banner,
    pink_bed,
    pink_carpet,
    pink_concrete,
    pink_concrete_powder,
    pink_glazed_terracotta,
    pink_shulker_box,
    pink_stained_glass,
    pink_stained_glass_pane,
    pink_terracotta,
    pink_tulip,
    pink_wall_banner,
    pink_wool,
    piston,
    piston_head,
    player_head,
    player_wall_head,
    podzol,
    polished_andesite,
    polished_andesite_slab,
    polished_andesite_stairs,
    polished_diorite,
    polished_diorite_slab,
    polished_diorite_stairs,
    polished_granite,
    polished_granite_slab,
    polished_granite_stairs,
    poppy,
    potatoes,
    potted_acacia_sapling,
    potted_allium,
    potted_azure_bluet,
    potted_bamboo,
    potted_birch_sapling,
    potted_blue_orchid,
    potted_brown_mushroom,
    potted_cactus,
    potted_cornflower,
    potted_dandelion,
    potted_dark_oak_sapling,
    potted_dead_bush,
    potted_fern,
    potted_jungle_sapling,
    potted_lily_of_the_valley,
    potted_oak_sapling,
    potted_orange_tulip,
    potted_oxeye_daisy,
    potted_pink_tulip,
    potted_poppy,
    potted_red_mushroom,
    potted_red_tulip,
    potted_spruce_sapling,
    potted_white_tulip,
    potted_wither_rose,
    powered_rail,
    prismarine,
    prismarine_brick_slab,
    prismarine_brick_stairs,
    prismarine_bricks,
    prismarine_slab,
    prismarine_stairs,
    prismarine_wall,
    pumpkin,
    pumpkin_stem,
    purple_banner,
    purple_bed,
    purple_carpet,
    purple_concrete,
    purple_concrete_powder,
    purple_glazed_terracotta,
    purple_shulker_box,
    purple_stained_glass,
    purple_stained_glass_pane,
    purple_terracotta,
    purple_wall_banner,
    purple_wool,
    purpur_block,
    purpur_pillar,
    purpur_slab,
    purpur_stairs,
    quartz_pillar,
    quartz_slab,
    quartz_stairs,
    rail,
    red_banner,
    red_bed,
    red_carpet,
    red_concrete,
    red_concrete_powder,
    red_glazed_terracotta,
    red_mushroom,
    red_mushroom_block,
    red_nether_brick_slab,
    red_nether_brick_stairs,
    red_nether_brick_wall,
    red_nether_bricks,
    red_sand,
    red_sandstone,
    red_sandstone_slab,
    red_sandstone_stairs,
    red_sandstone_wall,
    red_shulker_box,
    red_stained_glass,
    red_stained_glass_pane,
    red_terracotta,
    red_tulip,
    red_wall_banner,
    red_wool,
    comparator,
    redstone_wire,
    redstone_lamp,
    redstone_ore,
    repeater,
    redstone_torch,
    redstone_wall_torch,
    repeating_command_block,
    rose_bush,
    sand,
    sandstone,
    sandstone_slab,
    sandstone_stairs,
    sandstone_wall,
    scaffolding,
    sea_lantern,
    sea_pickle,
    seagrass,
    shulker_box,
    skeleton_skull,
    skeleton_wall_skull,
    slime_block,
    smithing_table,
    smoker,
    smooth_quartz,
    smooth_quartz_slab,
    smooth_quartz_stairs,
    smooth_red_sandstone,
    smooth_red_sandstone_slab,
    smooth_red_sandstone_stairs,
    smooth_sandstone,
    smooth_sandstone_slab,
    smooth_sandstone_stairs,
    smooth_stone,
    smooth_stone_slab,
    snow,
    snow_block,
    soul_sand,
    spawner,
    sponge,
    spruce_button,
    spruce_door,
    spruce_fence,
    spruce_fence_gate,
    spruce_leaves,
    spruce_log,
    spruce_planks,
    spruce_pressure_plate,
    spruce_sapling,
    spruce_sign,
    spruce_slab,
    spruce_stairs,
    spruce_trapdoor,
    spruce_wall_sign,
    spruce_wood,
    sticky_piston,
    stone,
    stone_brick_slab,
    stone_brick_stairs,
    stone_brick_wall,
    stone_bricks,
    stone_button,
    stone_pressure_plate,
    stone_slab,
    stone_stairs,
    stonecutter,
    stripped_acacia_log,
    stripped_acacia_wood,
    stripped_birch_log,
    stripped_birch_wood,
    stripped_dark_oak_log,
    stripped_dark_oak_wood,
    stripped_jungle_log,
    stripped_jungle_wood,
    stripped_oak_log,
    stripped_oak_wood,
    stripped_spruce_log,
    stripped_spruce_wood,
    structure_block,
    structure_void,
    sugar_cane,
    sunflower,
    sweet_berry_bush,
    tall_grass,
    tall_seagrass,
    terracotta,
    tnt,
    torch,
    trapped_chest,
    tripwire,
    tripwire_hook,
    tube_coral,
    tube_coral_block,
    tube_coral_fan,
    tube_coral_wall_fan,
    turtle_egg,
    vine,
    void_air,
    wall_torch,
    water,
    wet_sponge,
    wheat,
    white_banner,
    white_bed,
    white_carpet,
    white_concrete,
    white_concrete_powder,
    white_glazed_terracotta,
    white_shulker_box,
    white_stained_glass,
    white_stained_glass_pane,
    white_terracotta,
    white_tulip,
    white_wall_banner,
    white_wool,
    wither_rose,
    wither_skeleton_skull,
    wither_skeleton_wall_skull,
    yellow_banner,
    yellow_bed,
    yellow_carpet,
    yellow_concrete,
    yellow_concrete_powder,
    yellow_glazed_terracotta,
    yellow_shulker_box,
    yellow_stained_glass,
    yellow_stained_glass_pane,
    yellow_terracotta,
    yellow_wall_banner,
    yellow_wool,
    zombie_head,
    zombie_wall_head,

    minecraft_max_block_id,
};

} // namespace minecraft

static inline BlockId FromName(std::string const& name) {
    static std::map<std::string, BlockId> const mapping = {
        {"minecraft:acacia_button", minecraft::acacia_button},
        {"minecraft:acacia_door", minecraft::acacia_door},
        {"minecraft:acacia_fence", minecraft::acacia_fence},
        {"minecraft:acacia_fence_gate", minecraft::acacia_fence_gate},
        {"minecraft:acacia_leaves", minecraft::acacia_leaves},
        {"minecraft:acacia_log", minecraft::acacia_log},
        {"minecraft:acacia_planks", minecraft::acacia_planks},
        {"minecraft:acacia_pressure_plate", minecraft::acacia_pressure_plate},
        {"minecraft:acacia_sapling", minecraft::acacia_sapling},
        {"minecraft:acacia_sign", minecraft::acacia_sign},
        {"minecraft:acacia_slab", minecraft::acacia_slab},
        {"minecraft:acacia_stairs", minecraft::acacia_stairs},
        {"minecraft:acacia_trapdoor", minecraft::acacia_trapdoor},
        {"minecraft:acacia_wall_sign", minecraft::acacia_wall_sign},
        {"minecraft:acacia_wood", minecraft::acacia_wood},
        {"minecraft:activator_rail", minecraft::activator_rail},
        {"minecraft:air", minecraft::air},
        {"minecraft:allium", minecraft::allium},
        {"minecraft:andesite", minecraft::andesite},
        {"minecraft:andesite_slab", minecraft::andesite_slab},
        {"minecraft:andesite_stairs", minecraft::andesite_stairs},
        {"minecraft:andesite_wall", minecraft::andesite_wall},
        {"minecraft:anvil", minecraft::anvil},
        {"minecraft:attached_melon_stem", minecraft::attached_melon_stem},
        {"minecraft:attached_pumpkin_stem", minecraft::attached_pumpkin_stem},
        {"minecraft:azure_bluet", minecraft::azure_bluet},
        {"minecraft:bamboo", minecraft::bamboo},
        {"minecraft:bamboo_sapling", minecraft::bamboo_sapling},
        {"minecraft:barrel", minecraft::barrel},
        {"minecraft:barrier", minecraft::barrier},
        {"minecraft:beacon", minecraft::beacon},
        {"minecraft:bedrock", minecraft::bedrock},
        {"minecraft:beetroots", minecraft::beetroots},
        {"minecraft:bell", minecraft::bell},
        {"minecraft:birch_button", minecraft::birch_button},
        {"minecraft:birch_door", minecraft::birch_door},
        {"minecraft:birch_fence", minecraft::birch_fence},
        {"minecraft:birch_fence_gate", minecraft::birch_fence_gate},
        {"minecraft:birch_leaves", minecraft::birch_leaves},
        {"minecraft:birch_log", minecraft::birch_log},
        {"minecraft:birch_planks", minecraft::birch_planks},
        {"minecraft:birch_pressure_plate", minecraft::birch_pressure_plate},
        {"minecraft:birch_sapling", minecraft::birch_sapling},
        {"minecraft:birch_sign", minecraft::birch_sign},
        {"minecraft:birch_slab", minecraft::birch_slab},
        {"minecraft:birch_stairs", minecraft::birch_stairs},
        {"minecraft:birch_trapdoor", minecraft::birch_trapdoor},
        {"minecraft:birch_wall_sign", minecraft::birch_wall_sign},
        {"minecraft:birch_wood", minecraft::birch_wood},
        {"minecraft:black_banner", minecraft::black_banner},
        {"minecraft:black_bed", minecraft::black_bed},
        {"minecraft:black_carpet", minecraft::black_carpet},
        {"minecraft:black_concrete", minecraft::black_concrete},
        {"minecraft:black_concrete_powder", minecraft::black_concrete_powder},
        {"minecraft:black_glazed_terracotta", minecraft::black_glazed_terracotta},
        {"minecraft:black_shulker_box", minecraft::black_shulker_box},
        {"minecraft:black_stained_glass", minecraft::black_stained_glass},
        {"minecraft:black_stained_glass_pane", minecraft::black_stained_glass_pane},
        {"minecraft:black_terracotta", minecraft::black_terracotta},
        {"minecraft:black_wall_banner", minecraft::black_wall_banner},
        {"minecraft:black_wool", minecraft::black_wool},
        {"minecraft:blast_furnace", minecraft::blast_furnace},
        {"minecraft:coal_block", minecraft::coal_block},
        {"minecraft:diamond_block", minecraft::diamond_block},
        {"minecraft:emerald_block", minecraft::emerald_block},
        {"minecraft:gold_block", minecraft::gold_block},
        {"minecraft:iron_block", minecraft::iron_block},
        {"minecraft:quartz_block", minecraft::quartz_block},
        {"minecraft:redstone_block", minecraft::redstone_block},
        {"minecraft:blue_banner", minecraft::blue_banner},
        {"minecraft:blue_bed", minecraft::blue_bed},
        {"minecraft:blue_carpet", minecraft::blue_carpet},
        {"minecraft:blue_concrete", minecraft::blue_concrete},
        {"minecraft:blue_concrete_powder", minecraft::blue_concrete_powder},
        {"minecraft:blue_glazed_terracotta", minecraft::blue_glazed_terracotta},
        {"minecraft:blue_ice", minecraft::blue_ice},
        {"minecraft:blue_orchid", minecraft::blue_orchid},
        {"minecraft:blue_shulker_box", minecraft::blue_shulker_box},
        {"minecraft:blue_stained_glass", minecraft::blue_stained_glass},
        {"minecraft:blue_stained_glass_pane", minecraft::blue_stained_glass_pane},
        {"minecraft:blue_terracotta", minecraft::blue_terracotta},
        {"minecraft:blue_wall_banner", minecraft::blue_wall_banner},
        {"minecraft:blue_wool", minecraft::blue_wool},
        {"minecraft:bone_block", minecraft::bone_block},
        {"minecraft:bookshelf", minecraft::bookshelf},
        {"minecraft:brain_coral", minecraft::brain_coral},
        {"minecraft:brain_coral_block", minecraft::brain_coral_block},
        {"minecraft:brain_coral_fan", minecraft::brain_coral_fan},
        {"minecraft:brain_coral_wall_fan", minecraft::brain_coral_wall_fan},
        {"minecraft:brewing_stand", minecraft::brewing_stand},
        {"minecraft:brick_slab", minecraft::brick_slab},
        {"minecraft:brick_stairs", minecraft::brick_stairs},
        {"minecraft:brick_wall", minecraft::brick_wall},
        {"minecraft:bricks", minecraft::bricks},
        {"minecraft:brown_banner", minecraft::brown_banner},
        {"minecraft:brown_bed", minecraft::brown_bed},
        {"minecraft:brown_carpet", minecraft::brown_carpet},
        {"minecraft:brown_concrete", minecraft::brown_concrete},
        {"minecraft:brown_concrete_powder", minecraft::brown_concrete_powder},
        {"minecraft:brown_glazed_terracotta", minecraft::brown_glazed_terracotta},
        {"minecraft:brown_mushroom", minecraft::brown_mushroom},
        {"minecraft:brown_mushroom_block", minecraft::brown_mushroom_block},
        {"minecraft:brown_shulker_box", minecraft::brown_shulker_box},
        {"minecraft:brown_stained_glass", minecraft::brown_stained_glass},
        {"minecraft:brown_stained_glass_pane", minecraft::brown_stained_glass_pane},
        {"minecraft:brown_terracotta", minecraft::brown_terracotta},
        {"minecraft:brown_wall_banner", minecraft::brown_wall_banner},
        {"minecraft:brown_wool", minecraft::brown_wool},
        {"minecraft:bubble_column", minecraft::bubble_column},
        {"minecraft:bubble_coral", minecraft::bubble_coral},
        {"minecraft:bubble_coral_block", minecraft::bubble_coral_block},
        {"minecraft:bubble_coral_fan", minecraft::bubble_coral_fan},
        {"minecraft:bubble_coral_wall_fan", minecraft::bubble_coral_wall_fan},
        {"minecraft:cactus", minecraft::cactus},
        {"minecraft:cake", minecraft::cake},
        {"minecraft:campfire", minecraft::campfire},
        {"minecraft:carrots", minecraft::carrots},
        {"minecraft:cartography_table", minecraft::cartography_table},
        {"minecraft:carved_pumpkin", minecraft::carved_pumpkin},
        {"minecraft:cauldron", minecraft::cauldron},
        {"minecraft:cave_air", minecraft::cave_air},
        {"minecraft:chain_command_block", minecraft::chain_command_block},
        {"minecraft:chest", minecraft::chest},
        {"minecraft:chipped_anvil", minecraft::chipped_anvil},
        {"minecraft:chiseled_quartz_block", minecraft::chiseled_quartz_block},
        {"minecraft:chiseled_red_sandstone", minecraft::chiseled_red_sandstone},
        {"minecraft:chiseled_sandstone", minecraft::chiseled_sandstone},
        {"minecraft:chiseled_stone_bricks", minecraft::chiseled_stone_bricks},
        {"minecraft:chorus_flower", minecraft::chorus_flower},
        {"minecraft:chorus_plant", minecraft::chorus_plant},
        {"minecraft:clay", minecraft::clay},
        {"minecraft:coal_ore", minecraft::coal_ore},
        {"minecraft:coarse_dirt", minecraft::coarse_dirt},
        {"minecraft:cobblestone", minecraft::cobblestone},
        {"minecraft:cobblestone_slab", minecraft::cobblestone_slab},
        {"minecraft:cobblestone_stairs", minecraft::cobblestone_stairs},
        {"minecraft:cobblestone_wall", minecraft::cobblestone_wall},
        {"minecraft:cobweb", minecraft::cobweb},
        {"minecraft:cocoa", minecraft::cocoa},
        {"minecraft:command_block", minecraft::command_block},
        {"minecraft:composter", minecraft::composter},
        {"minecraft:conduit", minecraft::conduit},
        {"minecraft:cornflower", minecraft::cornflower},
        {"minecraft:cracked_stone_bricks", minecraft::cracked_stone_bricks},
        {"minecraft:crafting_table", minecraft::crafting_table},
        {"minecraft:creeper_head", minecraft::creeper_head},
        {"minecraft:creeper_wall_head", minecraft::creeper_wall_head},
        {"minecraft:cut_red_sandstone", minecraft::cut_red_sandstone},
        {"minecraft:cut_red_sandstone_slab", minecraft::cut_red_sandstone_slab},
        {"minecraft:cut_sandstone", minecraft::cut_sandstone},
        {"minecraft:cut_sandstone_slab", minecraft::cut_sandstone_slab},
        {"minecraft:cyan_banner", minecraft::cyan_banner},
        {"minecraft:cyan_bed", minecraft::cyan_bed},
        {"minecraft:cyan_carpet", minecraft::cyan_carpet},
        {"minecraft:cyan_concrete", minecraft::cyan_concrete},
        {"minecraft:cyan_concrete_powder", minecraft::cyan_concrete_powder},
        {"minecraft:cyan_glazed_terracotta", minecraft::cyan_glazed_terracotta},
        {"minecraft:cyan_shulker_box", minecraft::cyan_shulker_box},
        {"minecraft:cyan_stained_glass", minecraft::cyan_stained_glass},
        {"minecraft:cyan_stained_glass_pane", minecraft::cyan_stained_glass_pane},
        {"minecraft:cyan_terracotta", minecraft::cyan_terracotta},
        {"minecraft:cyan_wall_banner", minecraft::cyan_wall_banner},
        {"minecraft:cyan_wool", minecraft::cyan_wool},
        {"minecraft:damaged_anvil", minecraft::damaged_anvil},
        {"minecraft:dandelion", minecraft::dandelion},
        {"minecraft:dark_oak_button", minecraft::dark_oak_button},
        {"minecraft:dark_oak_door", minecraft::dark_oak_door},
        {"minecraft:dark_oak_fence", minecraft::dark_oak_fence},
        {"minecraft:dark_oak_fence_gate", minecraft::dark_oak_fence_gate},
        {"minecraft:dark_oak_leaves", minecraft::dark_oak_leaves},
        {"minecraft:dark_oak_log", minecraft::dark_oak_log},
        {"minecraft:dark_oak_planks", minecraft::dark_oak_planks},
        {"minecraft:dark_oak_pressure_plate", minecraft::dark_oak_pressure_plate},
        {"minecraft:dark_oak_sapling", minecraft::dark_oak_sapling},
        {"minecraft:dark_oak_sign", minecraft::dark_oak_sign},
        {"minecraft:dark_oak_slab", minecraft::dark_oak_slab},
        {"minecraft:dark_oak_stairs", minecraft::dark_oak_stairs},
        {"minecraft:dark_oak_trapdoor", minecraft::dark_oak_trapdoor},
        {"minecraft:dark_oak_wall_sign", minecraft::dark_oak_wall_sign},
        {"minecraft:dark_oak_wood", minecraft::dark_oak_wood},
        {"minecraft:dark_prismarine", minecraft::dark_prismarine},
        {"minecraft:dark_prismarine_slab", minecraft::dark_prismarine_slab},
        {"minecraft:dark_prismarine_stairs", minecraft::dark_prismarine_stairs},
        {"minecraft:daylight_detector", minecraft::daylight_detector},
        {"minecraft:dead_brain_coral", minecraft::dead_brain_coral},
        {"minecraft:dead_brain_coral_block", minecraft::dead_brain_coral_block},
        {"minecraft:dead_brain_coral_fan", minecraft::dead_brain_coral_fan},
        {"minecraft:dead_brain_coral_wall_fan", minecraft::dead_brain_coral_wall_fan},
        {"minecraft:dead_bubble_coral", minecraft::dead_bubble_coral},
        {"minecraft:dead_bubble_coral_block", minecraft::dead_bubble_coral_block},
        {"minecraft:dead_bubble_coral_fan", minecraft::dead_bubble_coral_fan},
        {"minecraft:dead_bubble_coral_wall_fan", minecraft::dead_bubble_coral_wall_fan},
        {"minecraft:dead_bush", minecraft::dead_bush},
        {"minecraft:dead_fire_coral", minecraft::dead_fire_coral},
        {"minecraft:dead_fire_coral_block", minecraft::dead_fire_coral_block},
        {"minecraft:dead_fire_coral_fan", minecraft::dead_fire_coral_fan},
        {"minecraft:dead_fire_coral_wall_fan", minecraft::dead_fire_coral_wall_fan},
        {"minecraft:dead_horn_coral", minecraft::dead_horn_coral},
        {"minecraft:dead_horn_coral_block", minecraft::dead_horn_coral_block},
        {"minecraft:dead_horn_coral_fan", minecraft::dead_horn_coral_fan},
        {"minecraft:dead_horn_coral_wall_fan", minecraft::dead_horn_coral_wall_fan},
        {"minecraft:dead_tube_coral", minecraft::dead_tube_coral},
        {"minecraft:dead_tube_coral_block", minecraft::dead_tube_coral_block},
        {"minecraft:dead_tube_coral_fan", minecraft::dead_tube_coral_fan},
        {"minecraft:dead_tube_coral_wall_fan", minecraft::dead_tube_coral_wall_fan},
        {"minecraft:detector_rail", minecraft::detector_rail},
        {"minecraft:diamond_ore", minecraft::diamond_ore},
        {"minecraft:diorite", minecraft::diorite},
        {"minecraft:diorite_slab", minecraft::diorite_slab},
        {"minecraft:diorite_stairs", minecraft::diorite_stairs},
        {"minecraft:diorite_wall", minecraft::diorite_wall},
        {"minecraft:dirt", minecraft::dirt},
        {"minecraft:dispenser", minecraft::dispenser},
        {"minecraft:dragon_egg", minecraft::dragon_egg},
        {"minecraft:dragon_head", minecraft::dragon_head},
        {"minecraft:dragon_wall_head", minecraft::dragon_wall_head},
        {"minecraft:dried_kelp_block", minecraft::dried_kelp_block},
        {"minecraft:dropper", minecraft::dropper},
        {"minecraft:emerald_ore", minecraft::emerald_ore},
        {"minecraft:enchanting_table", minecraft::enchanting_table},
        {"minecraft:end_gateway", minecraft::end_gateway},
        {"minecraft:end_portal", minecraft::end_portal},
        {"minecraft:end_portal_frame", minecraft::end_portal_frame},
        {"minecraft:end_rod", minecraft::end_rod},
        {"minecraft:end_stone", minecraft::end_stone},
        {"minecraft:end_stone_brick_slab", minecraft::end_stone_brick_slab},
        {"minecraft:end_stone_brick_stairs", minecraft::end_stone_brick_stairs},
        {"minecraft:end_stone_brick_wall", minecraft::end_stone_brick_wall},
        {"minecraft:end_stone_bricks", minecraft::end_stone_bricks},
        {"minecraft:ender_chest", minecraft::ender_chest},
        {"minecraft:farmland", minecraft::farmland},
        {"minecraft:fern", minecraft::fern},
        {"minecraft:fire", minecraft::fire},
        {"minecraft:fire_coral", minecraft::fire_coral},
        {"minecraft:fire_coral_block", minecraft::fire_coral_block},
        {"minecraft:fire_coral_fan", minecraft::fire_coral_fan},
        {"minecraft:fire_coral_wall_fan", minecraft::fire_coral_wall_fan},
        {"minecraft:fletching_table", minecraft::fletching_table},
        {"minecraft:flower_pot", minecraft::flower_pot},
        {"minecraft:frosted_ice", minecraft::frosted_ice},
        {"minecraft:furnace", minecraft::furnace},
        {"minecraft:glass", minecraft::glass},
        {"minecraft:glass_pane", minecraft::glass_pane},
        {"minecraft:glowstone", minecraft::glowstone},
        {"minecraft:gold_ore", minecraft::gold_ore},
        {"minecraft:granite", minecraft::granite},
        {"minecraft:granite_slab", minecraft::granite_slab},
        {"minecraft:granite_stairs", minecraft::granite_stairs},
        {"minecraft:granite_wall", minecraft::granite_wall},
        {"minecraft:grass", minecraft::grass},
        {"minecraft:grass_block", minecraft::grass_block},
        {"minecraft:grass_path", minecraft::grass_path},
        {"minecraft:gravel", minecraft::gravel},
        {"minecraft:gray_banner", minecraft::gray_banner},
        {"minecraft:gray_bed", minecraft::gray_bed},
        {"minecraft:gray_carpet", minecraft::gray_carpet},
        {"minecraft:gray_concrete", minecraft::gray_concrete},
        {"minecraft:gray_concrete_powder", minecraft::gray_concrete_powder},
        {"minecraft:gray_glazed_terracotta", minecraft::gray_glazed_terracotta},
        {"minecraft:gray_shulker_box", minecraft::gray_shulker_box},
        {"minecraft:gray_stained_glass", minecraft::gray_stained_glass},
        {"minecraft:gray_stained_glass_pane", minecraft::gray_stained_glass_pane},
        {"minecraft:gray_terracotta", minecraft::gray_terracotta},
        {"minecraft:gray_wall_banner", minecraft::gray_wall_banner},
        {"minecraft:gray_wool", minecraft::gray_wool},
        {"minecraft:green_banner", minecraft::green_banner},
        {"minecraft:green_bed", minecraft::green_bed},
        {"minecraft:green_carpet", minecraft::green_carpet},
        {"minecraft:green_concrete", minecraft::green_concrete},
        {"minecraft:green_concrete_powder", minecraft::green_concrete_powder},
        {"minecraft:green_glazed_terracotta", minecraft::green_glazed_terracotta},
        {"minecraft:green_shulker_box", minecraft::green_shulker_box},
        {"minecraft:green_stained_glass", minecraft::green_stained_glass},
        {"minecraft:green_stained_glass_pane", minecraft::green_stained_glass_pane},
        {"minecraft:green_terracotta", minecraft::green_terracotta},
        {"minecraft:green_wall_banner", minecraft::green_wall_banner},
        {"minecraft:green_wool", minecraft::green_wool},
        {"minecraft:grindstone", minecraft::grindstone},
        {"minecraft:hay_block", minecraft::hay_block},
        {"minecraft:heavy_weighted_pressure_plate", minecraft::heavy_weighted_pressure_plate},
        {"minecraft:hopper", minecraft::hopper},
        {"minecraft:horn_coral", minecraft::horn_coral},
        {"minecraft:horn_coral_block", minecraft::horn_coral_block},
        {"minecraft:horn_coral_fan", minecraft::horn_coral_fan},
        {"minecraft:horn_coral_wall_fan", minecraft::horn_coral_wall_fan},
        {"minecraft:ice", minecraft::ice},
        {"minecraft:infested_chiseled_stone_bricks", minecraft::infested_chiseled_stone_bricks},
        {"minecraft:infested_cobblestone", minecraft::infested_cobblestone},
        {"minecraft:infested_cracked_stone_bricks", minecraft::infested_cracked_stone_bricks},
        {"minecraft:infested_mossy_stone_bricks", minecraft::infested_mossy_stone_bricks},
        {"minecraft:infested_stone", minecraft::infested_stone},
        {"minecraft:infested_stone_bricks", minecraft::infested_stone_bricks},
        {"minecraft:iron_bars", minecraft::iron_bars},
        {"minecraft:iron_door", minecraft::iron_door},
        {"minecraft:iron_ore", minecraft::iron_ore},
        {"minecraft:iron_trapdoor", minecraft::iron_trapdoor},
        {"minecraft:jack_o_lantern", minecraft::jack_o_lantern},
        {"minecraft:jigsaw", minecraft::jigsaw},
        {"minecraft:jukebox", minecraft::jukebox},
        {"minecraft:jungle_button", minecraft::jungle_button},
        {"minecraft:jungle_door", minecraft::jungle_door},
        {"minecraft:jungle_fence", minecraft::jungle_fence},
        {"minecraft:jungle_fence_gate", minecraft::jungle_fence_gate},
        {"minecraft:jungle_leaves", minecraft::jungle_leaves},
        {"minecraft:jungle_log", minecraft::jungle_log},
        {"minecraft:jungle_planks", minecraft::jungle_planks},
        {"minecraft:jungle_pressure_plate", minecraft::jungle_pressure_plate},
        {"minecraft:jungle_sapling", minecraft::jungle_sapling},
        {"minecraft:jungle_sign", minecraft::jungle_sign},
        {"minecraft:jungle_slab", minecraft::jungle_slab},
        {"minecraft:jungle_stairs", minecraft::jungle_stairs},
        {"minecraft:jungle_trapdoor", minecraft::jungle_trapdoor},
        {"minecraft:jungle_wall_sign", minecraft::jungle_wall_sign},
        {"minecraft:jungle_wood", minecraft::jungle_wood},
        {"minecraft:kelp", minecraft::kelp},
        {"minecraft:kelp_plant", minecraft::kelp_plant},
        {"minecraft:ladder", minecraft::ladder},
        {"minecraft:lantern", minecraft::lantern},
        {"minecraft:lapis_block", minecraft::lapis_block},
        {"minecraft:lapis_ore", minecraft::lapis_ore},
        {"minecraft:large_fern", minecraft::large_fern},
        {"minecraft:lava", minecraft::lava},
        {"minecraft:lectern", minecraft::lectern},
        {"minecraft:lever", minecraft::lever},
        {"minecraft:light_blue_banner", minecraft::light_blue_banner},
        {"minecraft:light_blue_bed", minecraft::light_blue_bed},
        {"minecraft:light_blue_carpet", minecraft::light_blue_carpet},
        {"minecraft:light_blue_concrete", minecraft::light_blue_concrete},
        {"minecraft:light_blue_concrete_powder", minecraft::light_blue_concrete_powder},
        {"minecraft:light_blue_glazed_terracotta", minecraft::light_blue_glazed_terracotta},
        {"minecraft:light_blue_shulker_box", minecraft::light_blue_shulker_box},
        {"minecraft:light_blue_stained_glass", minecraft::light_blue_stained_glass},
        {"minecraft:light_blue_stained_glass_pane", minecraft::light_blue_stained_glass_pane},
        {"minecraft:light_blue_terracotta", minecraft::light_blue_terracotta},
        {"minecraft:light_blue_wall_banner", minecraft::light_blue_wall_banner},
        {"minecraft:light_blue_wool", minecraft::light_blue_wool},
        {"minecraft:light_gray_banner", minecraft::light_gray_banner},
        {"minecraft:light_gray_bed", minecraft::light_gray_bed},
        {"minecraft:light_gray_carpet", minecraft::light_gray_carpet},
        {"minecraft:light_gray_concrete", minecraft::light_gray_concrete},
        {"minecraft:light_gray_concrete_powder", minecraft::light_gray_concrete_powder},
        {"minecraft:light_gray_glazed_terracotta", minecraft::light_gray_glazed_terracotta},
        {"minecraft:light_gray_shulker_box", minecraft::light_gray_shulker_box},
        {"minecraft:light_gray_stained_glass", minecraft::light_gray_stained_glass},
        {"minecraft:light_gray_stained_glass_pane", minecraft::light_gray_stained_glass_pane},
        {"minecraft:light_gray_terracotta", minecraft::light_gray_terracotta},
        {"minecraft:light_gray_wall_banner", minecraft::light_gray_wall_banner},
        {"minecraft:light_gray_wool", minecraft::light_gray_wool},
        {"minecraft:light_weighted_pressure_plate", minecraft::light_weighted_pressure_plate},
        {"minecraft:lilac", minecraft::lilac},
        {"minecraft:lily_of_the_valley", minecraft::lily_of_the_valley},
        {"minecraft:lily_pad", minecraft::lily_pad},
        {"minecraft:lime_banner", minecraft::lime_banner},
        {"minecraft:lime_bed", minecraft::lime_bed},
        {"minecraft:lime_carpet", minecraft::lime_carpet},
        {"minecraft:lime_concrete", minecraft::lime_concrete},
        {"minecraft:lime_concrete_powder", minecraft::lime_concrete_powder},
        {"minecraft:lime_glazed_terracotta", minecraft::lime_glazed_terracotta},
        {"minecraft:lime_shulker_box", minecraft::lime_shulker_box},
        {"minecraft:lime_stained_glass", minecraft::lime_stained_glass},
        {"minecraft:lime_stained_glass_pane", minecraft::lime_stained_glass_pane},
        {"minecraft:lime_terracotta", minecraft::lime_terracotta},
        {"minecraft:lime_wall_banner", minecraft::lime_wall_banner},
        {"minecraft:lime_wool", minecraft::lime_wool},
        {"minecraft:loom", minecraft::loom},
        {"minecraft:magenta_banner", minecraft::magenta_banner},
        {"minecraft:magenta_bed", minecraft::magenta_bed},
        {"minecraft:magenta_carpet", minecraft::magenta_carpet},
        {"minecraft:magenta_concrete", minecraft::magenta_concrete},
        {"minecraft:magenta_concrete_powder", minecraft::magenta_concrete_powder},
        {"minecraft:magenta_glazed_terracotta", minecraft::magenta_glazed_terracotta},
        {"minecraft:magenta_shulker_box", minecraft::magenta_shulker_box},
        {"minecraft:magenta_stained_glass", minecraft::magenta_stained_glass},
        {"minecraft:magenta_stained_glass_pane", minecraft::magenta_stained_glass_pane},
        {"minecraft:magenta_terracotta", minecraft::magenta_terracotta},
        {"minecraft:magenta_wall_banner", minecraft::magenta_wall_banner},
        {"minecraft:magenta_wool", minecraft::magenta_wool},
        {"minecraft:magma_block", minecraft::magma_block},
        {"minecraft:melon", minecraft::melon},
        {"minecraft:melon_stem", minecraft::melon_stem},
        {"minecraft:mossy_cobblestone", minecraft::mossy_cobblestone},
        {"minecraft:mossy_cobblestone_slab", minecraft::mossy_cobblestone_slab},
        {"minecraft:mossy_cobblestone_stairs", minecraft::mossy_cobblestone_stairs},
        {"minecraft:mossy_cobblestone_wall", minecraft::mossy_cobblestone_wall},
        {"minecraft:mossy_stone_brick_slab", minecraft::mossy_stone_brick_slab},
        {"minecraft:mossy_stone_brick_stairs", minecraft::mossy_stone_brick_stairs},
        {"minecraft:mossy_stone_brick_wall", minecraft::mossy_stone_brick_wall},
        {"minecraft:mossy_stone_bricks", minecraft::mossy_stone_bricks},
        {"minecraft:moving_piston", minecraft::moving_piston},
        {"minecraft:mushroom_stem", minecraft::mushroom_stem},
        {"minecraft:mycelium", minecraft::mycelium},
        {"minecraft:nether_brick_fence", minecraft::nether_brick_fence},
        {"minecraft:nether_brick_slab", minecraft::nether_brick_slab},
        {"minecraft:nether_brick_stairs", minecraft::nether_brick_stairs},
        {"minecraft:nether_brick_wall", minecraft::nether_brick_wall},
        {"minecraft:nether_bricks", minecraft::nether_bricks},
        {"minecraft:nether_portal", minecraft::nether_portal},
        {"minecraft:nether_quartz_ore", minecraft::nether_quartz_ore},
        {"minecraft:nether_wart", minecraft::nether_wart},
        {"minecraft:nether_wart_block", minecraft::nether_wart_block},
        {"minecraft:netherrack", minecraft::netherrack},
        {"minecraft:note_block", minecraft::note_block},
        {"minecraft:oak_button", minecraft::oak_button},
        {"minecraft:oak_door", minecraft::oak_door},
        {"minecraft:oak_fence", minecraft::oak_fence},
        {"minecraft:oak_fence_gate", minecraft::oak_fence_gate},
        {"minecraft:oak_leaves", minecraft::oak_leaves},
        {"minecraft:oak_log", minecraft::oak_log},
        {"minecraft:oak_planks", minecraft::oak_planks},
        {"minecraft:oak_pressure_plate", minecraft::oak_pressure_plate},
        {"minecraft:oak_sapling", minecraft::oak_sapling},
        {"minecraft:oak_sign", minecraft::oak_sign},
        {"minecraft:oak_slab", minecraft::oak_slab},
        {"minecraft:oak_stairs", minecraft::oak_stairs},
        {"minecraft:oak_trapdoor", minecraft::oak_trapdoor},
        {"minecraft:oak_wall_sign", minecraft::oak_wall_sign},
        {"minecraft:oak_wood", minecraft::oak_wood},
        {"minecraft:observer", minecraft::observer},
        {"minecraft:obsidian", minecraft::obsidian},
        {"minecraft:orange_banner", minecraft::orange_banner},
        {"minecraft:orange_bed", minecraft::orange_bed},
        {"minecraft:orange_carpet", minecraft::orange_carpet},
        {"minecraft:orange_concrete", minecraft::orange_concrete},
        {"minecraft:orange_concrete_powder", minecraft::orange_concrete_powder},
        {"minecraft:orange_glazed_terracotta", minecraft::orange_glazed_terracotta},
        {"minecraft:orange_shulker_box", minecraft::orange_shulker_box},
        {"minecraft:orange_stained_glass", minecraft::orange_stained_glass},
        {"minecraft:orange_stained_glass_pane", minecraft::orange_stained_glass_pane},
        {"minecraft:orange_terracotta", minecraft::orange_terracotta},
        {"minecraft:orange_tulip", minecraft::orange_tulip},
        {"minecraft:orange_wall_banner", minecraft::orange_wall_banner},
        {"minecraft:orange_wool", minecraft::orange_wool},
        {"minecraft:oxeye_daisy", minecraft::oxeye_daisy},
        {"minecraft:packed_ice", minecraft::packed_ice},
        {"minecraft:peony", minecraft::peony},
        {"minecraft:petrified_oak_slab", minecraft::petrified_oak_slab},
        {"minecraft:pink_banner", minecraft::pink_banner},
        {"minecraft:pink_bed", minecraft::pink_bed},
        {"minecraft:pink_carpet", minecraft::pink_carpet},
        {"minecraft:pink_concrete", minecraft::pink_concrete},
        {"minecraft:pink_concrete_powder", minecraft::pink_concrete_powder},
        {"minecraft:pink_glazed_terracotta", minecraft::pink_glazed_terracotta},
        {"minecraft:pink_shulker_box", minecraft::pink_shulker_box},
        {"minecraft:pink_stained_glass", minecraft::pink_stained_glass},
        {"minecraft:pink_stained_glass_pane", minecraft::pink_stained_glass_pane},
        {"minecraft:pink_terracotta", minecraft::pink_terracotta},
        {"minecraft:pink_tulip", minecraft::pink_tulip},
        {"minecraft:pink_wall_banner", minecraft::pink_wall_banner},
        {"minecraft:pink_wool", minecraft::pink_wool},
        {"minecraft:piston", minecraft::piston},
        {"minecraft:piston_head", minecraft::piston_head},
        {"minecraft:player_head", minecraft::player_head},
        {"minecraft:player_wall_head", minecraft::player_wall_head},
        {"minecraft:podzol", minecraft::podzol},
        {"minecraft:polished_andesite", minecraft::polished_andesite},
        {"minecraft:polished_andesite_slab", minecraft::polished_andesite_slab},
        {"minecraft:polished_andesite_stairs", minecraft::polished_andesite_stairs},
        {"minecraft:polished_diorite", minecraft::polished_diorite},
        {"minecraft:polished_diorite_slab", minecraft::polished_diorite_slab},
        {"minecraft:polished_diorite_stairs", minecraft::polished_diorite_stairs},
        {"minecraft:polished_granite", minecraft::polished_granite},
        {"minecraft:polished_granite_slab", minecraft::polished_granite_slab},
        {"minecraft:polished_granite_stairs", minecraft::polished_granite_stairs},
        {"minecraft:poppy", minecraft::poppy},
        {"minecraft:potatoes", minecraft::potatoes},
        {"minecraft:potted_acacia_sapling", minecraft::potted_acacia_sapling},
        {"minecraft:potted_allium", minecraft::potted_allium},
        {"minecraft:potted_azure_bluet", minecraft::potted_azure_bluet},
        {"minecraft:potted_bamboo", minecraft::potted_bamboo},
        {"minecraft:potted_birch_sapling", minecraft::potted_birch_sapling},
        {"minecraft:potted_blue_orchid", minecraft::potted_blue_orchid},
        {"minecraft:potted_brown_mushroom", minecraft::potted_brown_mushroom},
        {"minecraft:potted_cactus", minecraft::potted_cactus},
        {"minecraft:potted_cornflower", minecraft::potted_cornflower},
        {"minecraft:potted_dandelion", minecraft::potted_dandelion},
        {"minecraft:potted_dark_oak_sapling", minecraft::potted_dark_oak_sapling},
        {"minecraft:potted_dead_bush", minecraft::potted_dead_bush},
        {"minecraft:potted_fern", minecraft::potted_fern},
        {"minecraft:potted_jungle_sapling", minecraft::potted_jungle_sapling},
        {"minecraft:potted_lily_of_the_valley", minecraft::potted_lily_of_the_valley},
        {"minecraft:potted_oak_sapling", minecraft::potted_oak_sapling},
        {"minecraft:potted_orange_tulip", minecraft::potted_orange_tulip},
        {"minecraft:potted_oxeye_daisy", minecraft::potted_oxeye_daisy},
        {"minecraft:potted_pink_tulip", minecraft::potted_pink_tulip},
        {"minecraft:potted_poppy", minecraft::potted_poppy},
        {"minecraft:potted_red_mushroom", minecraft::potted_red_mushroom},
        {"minecraft:potted_red_tulip", minecraft::potted_red_tulip},
        {"minecraft:potted_spruce_sapling", minecraft::potted_spruce_sapling},
        {"minecraft:potted_white_tulip", minecraft::potted_white_tulip},
        {"minecraft:potted_wither_rose", minecraft::potted_wither_rose},
        {"minecraft:powered_rail", minecraft::powered_rail},
        {"minecraft:prismarine", minecraft::prismarine},
        {"minecraft:prismarine_brick_slab", minecraft::prismarine_brick_slab},
        {"minecraft:prismarine_brick_stairs", minecraft::prismarine_brick_stairs},
        {"minecraft:prismarine_bricks", minecraft::prismarine_bricks},
        {"minecraft:prismarine_slab", minecraft::prismarine_slab},
        {"minecraft:prismarine_stairs", minecraft::prismarine_stairs},
        {"minecraft:prismarine_wall", minecraft::prismarine_wall},
        {"minecraft:pumpkin", minecraft::pumpkin},
        {"minecraft:pumpkin_stem", minecraft::pumpkin_stem},
        {"minecraft:purple_banner", minecraft::purple_banner},
        {"minecraft:purple_bed", minecraft::purple_bed},
        {"minecraft:purple_carpet", minecraft::purple_carpet},
        {"minecraft:purple_concrete", minecraft::purple_concrete},
        {"minecraft:purple_concrete_powder", minecraft::purple_concrete_powder},
        {"minecraft:purple_glazed_terracotta", minecraft::purple_glazed_terracotta},
        {"minecraft:purple_shulker_box", minecraft::purple_shulker_box},
        {"minecraft:purple_stained_glass", minecraft::purple_stained_glass},
        {"minecraft:purple_stained_glass_pane", minecraft::purple_stained_glass_pane},
        {"minecraft:purple_terracotta", minecraft::purple_terracotta},
        {"minecraft:purple_wall_banner", minecraft::purple_wall_banner},
        {"minecraft:purple_wool", minecraft::purple_wool},
        {"minecraft:purpur_block", minecraft::purpur_block},
        {"minecraft:purpur_pillar", minecraft::purpur_pillar},
        {"minecraft:purpur_slab", minecraft::purpur_slab},
        {"minecraft:purpur_stairs", minecraft::purpur_stairs},
        {"minecraft:quartz_pillar", minecraft::quartz_pillar},
        {"minecraft:quartz_slab", minecraft::quartz_slab},
        {"minecraft:quartz_stairs", minecraft::quartz_stairs},
        {"minecraft:rail", minecraft::rail},
        {"minecraft:red_banner", minecraft::red_banner},
        {"minecraft:red_bed", minecraft::red_bed},
        {"minecraft:red_carpet", minecraft::red_carpet},
        {"minecraft:red_concrete", minecraft::red_concrete},
        {"minecraft:red_concrete_powder", minecraft::red_concrete_powder},
        {"minecraft:red_glazed_terracotta", minecraft::red_glazed_terracotta},
        {"minecraft:red_mushroom", minecraft::red_mushroom},
        {"minecraft:red_mushroom_block", minecraft::red_mushroom_block},
        {"minecraft:red_nether_brick_slab", minecraft::red_nether_brick_slab},
        {"minecraft:red_nether_brick_stairs", minecraft::red_nether_brick_stairs},
        {"minecraft:red_nether_brick_wall", minecraft::red_nether_brick_wall},
        {"minecraft:red_nether_bricks", minecraft::red_nether_bricks},
        {"minecraft:red_sand", minecraft::red_sand},
        {"minecraft:red_sandstone", minecraft::red_sandstone},
        {"minecraft:red_sandstone_slab", minecraft::red_sandstone_slab},
        {"minecraft:red_sandstone_stairs", minecraft::red_sandstone_stairs},
        {"minecraft:red_sandstone_wall", minecraft::red_sandstone_wall},
        {"minecraft:red_shulker_box", minecraft::red_shulker_box},
        {"minecraft:red_stained_glass", minecraft::red_stained_glass},
        {"minecraft:red_stained_glass_pane", minecraft::red_stained_glass_pane},
        {"minecraft:red_terracotta", minecraft::red_terracotta},
        {"minecraft:red_tulip", minecraft::red_tulip},
        {"minecraft:red_wall_banner", minecraft::red_wall_banner},
        {"minecraft:red_wool", minecraft::red_wool},
        {"minecraft:comparator", minecraft::comparator},
        {"minecraft:redstone_wire", minecraft::redstone_wire},
        {"minecraft:redstone_lamp", minecraft::redstone_lamp},
        {"minecraft:redstone_ore", minecraft::redstone_ore},
        {"minecraft:repeater", minecraft::repeater},
        {"minecraft:redstone_torch", minecraft::redstone_torch},
        {"minecraft:redstone_wall_torch", minecraft::redstone_wall_torch},
        {"minecraft:repeating_command_block", minecraft::repeating_command_block},
        {"minecraft:rose_bush", minecraft::rose_bush},
        {"minecraft:sand", minecraft::sand},
        {"minecraft:sandstone", minecraft::sandstone},
        {"minecraft:sandstone_slab", minecraft::sandstone_slab},
        {"minecraft:sandstone_stairs", minecraft::sandstone_stairs},
        {"minecraft:sandstone_wall", minecraft::sandstone_wall},
        {"minecraft:scaffolding", minecraft::scaffolding},
        {"minecraft:sea_lantern", minecraft::sea_lantern},
        {"minecraft:sea_pickle", minecraft::sea_pickle},
        {"minecraft:seagrass", minecraft::seagrass},
        {"minecraft:shulker_box", minecraft::shulker_box},
        {"minecraft:skeleton_skull", minecraft::skeleton_skull},
        {"minecraft:skeleton_wall_skull", minecraft::skeleton_wall_skull},
        {"minecraft:slime_block", minecraft::slime_block},
        {"minecraft:smithing_table", minecraft::smithing_table},
        {"minecraft:smoker", minecraft::smoker},
        {"minecraft:smooth_quartz", minecraft::smooth_quartz},
        {"minecraft:smooth_quartz_slab", minecraft::smooth_quartz_slab},
        {"minecraft:smooth_quartz_stairs", minecraft::smooth_quartz_stairs},
        {"minecraft:smooth_red_sandstone", minecraft::smooth_red_sandstone},
        {"minecraft:smooth_red_sandstone_slab", minecraft::smooth_red_sandstone_slab},
        {"minecraft:smooth_red_sandstone_stairs", minecraft::smooth_red_sandstone_stairs},
        {"minecraft:smooth_sandstone", minecraft::smooth_sandstone},
        {"minecraft:smooth_sandstone_slab", minecraft::smooth_sandstone_slab},
        {"minecraft:smooth_sandstone_stairs", minecraft::smooth_sandstone_stairs},
        {"minecraft:smooth_stone", minecraft::smooth_stone},
        {"minecraft:smooth_stone_slab", minecraft::smooth_stone_slab},
        {"minecraft:snow", minecraft::snow},
        {"minecraft:snow_block", minecraft::snow_block},
        {"minecraft:soul_sand", minecraft::soul_sand},
        {"minecraft:spawner", minecraft::spawner},
        {"minecraft:sponge", minecraft::sponge},
        {"minecraft:spruce_button", minecraft::spruce_button},
        {"minecraft:spruce_door", minecraft::spruce_door},
        {"minecraft:spruce_fence", minecraft::spruce_fence},
        {"minecraft:spruce_fence_gate", minecraft::spruce_fence_gate},
        {"minecraft:spruce_leaves", minecraft::spruce_leaves},
        {"minecraft:spruce_log", minecraft::spruce_log},
        {"minecraft:spruce_planks", minecraft::spruce_planks},
        {"minecraft:spruce_pressure_plate", minecraft::spruce_pressure_plate},
        {"minecraft:spruce_sapling", minecraft::spruce_sapling},
        {"minecraft:spruce_sign", minecraft::spruce_sign},
        {"minecraft:spruce_slab", minecraft::spruce_slab},
        {"minecraft:spruce_stairs", minecraft::spruce_stairs},
        {"minecraft:spruce_trapdoor", minecraft::spruce_trapdoor},
        {"minecraft:spruce_wall_sign", minecraft::spruce_wall_sign},
        {"minecraft:spruce_wood", minecraft::spruce_wood},
        {"minecraft:sticky_piston", minecraft::sticky_piston},
        {"minecraft:stone", minecraft::stone},
        {"minecraft:stone_brick_slab", minecraft::stone_brick_slab},
        {"minecraft:stone_brick_stairs", minecraft::stone_brick_stairs},
        {"minecraft:stone_brick_wall", minecraft::stone_brick_wall},
        {"minecraft:stone_bricks", minecraft::stone_bricks},
        {"minecraft:stone_button", minecraft::stone_button},
        {"minecraft:stone_pressure_plate", minecraft::stone_pressure_plate},
        {"minecraft:stone_slab", minecraft::stone_slab},
        {"minecraft:stone_stairs", minecraft::stone_stairs},
        {"minecraft:stonecutter", minecraft::stonecutter},
        {"minecraft:stripped_acacia_log", minecraft::stripped_acacia_log},
        {"minecraft:stripped_acacia_wood", minecraft::stripped_acacia_wood},
        {"minecraft:stripped_birch_log", minecraft::stripped_birch_log},
        {"minecraft:stripped_birch_wood", minecraft::stripped_birch_wood},
        {"minecraft:stripped_dark_oak_log", minecraft::stripped_dark_oak_log},
        {"minecraft:stripped_dark_oak_wood", minecraft::stripped_dark_oak_wood},
        {"minecraft:stripped_jungle_log", minecraft::stripped_jungle_log},
        {"minecraft:stripped_jungle_wood", minecraft::stripped_jungle_wood},
        {"minecraft:stripped_oak_log", minecraft::stripped_oak_log},
        {"minecraft:stripped_oak_wood", minecraft::stripped_oak_wood},
        {"minecraft:stripped_spruce_log", minecraft::stripped_spruce_log},
        {"minecraft:stripped_spruce_wood", minecraft::stripped_spruce_wood},
        {"minecraft:structure_block", minecraft::structure_block},
        {"minecraft:structure_void", minecraft::structure_void},
        {"minecraft:sugar_cane", minecraft::sugar_cane},
        {"minecraft:sunflower", minecraft::sunflower},
        {"minecraft:sweet_berry_bush", minecraft::sweet_berry_bush},
        {"minecraft:tall_grass", minecraft::tall_grass},
        {"minecraft:tall_seagrass", minecraft::tall_seagrass},
        {"minecraft:terracotta", minecraft::terracotta},
        {"minecraft:tnt", minecraft::tnt},
        {"minecraft:torch", minecraft::torch},
        {"minecraft:trapped_chest", minecraft::trapped_chest},
        {"minecraft:tripwire", minecraft::tripwire},
        {"minecraft:tripwire_hook", minecraft::tripwire_hook},
        {"minecraft:tube_coral", minecraft::tube_coral},
        {"minecraft:tube_coral_block", minecraft::tube_coral_block},
        {"minecraft:tube_coral_fan", minecraft::tube_coral_fan},
        {"minecraft:tube_coral_wall_fan", minecraft::tube_coral_wall_fan},
        {"minecraft:turtle_egg", minecraft::turtle_egg},
        {"minecraft:vine", minecraft::vine},
        {"minecraft:void_air", minecraft::void_air},
        {"minecraft:wall_torch", minecraft::wall_torch},
        {"minecraft:water", minecraft::water},
        {"minecraft:wet_sponge", minecraft::wet_sponge},
        {"minecraft:wheat", minecraft::wheat},
        {"minecraft:white_banner", minecraft::white_banner},
        {"minecraft:white_bed", minecraft::white_bed},
        {"minecraft:white_carpet", minecraft::white_carpet},
        {"minecraft:white_concrete", minecraft::white_concrete},
        {"minecraft:white_concrete_powder", minecraft::white_concrete_powder},
        {"minecraft:white_glazed_terracotta", minecraft::white_glazed_terracotta},
        {"minecraft:white_shulker_box", minecraft::white_shulker_box},
        {"minecraft:white_stained_glass", minecraft::white_stained_glass},
        {"minecraft:white_stained_glass_pane", minecraft::white_stained_glass_pane},
        {"minecraft:white_terracotta", minecraft::white_terracotta},
        {"minecraft:white_tulip", minecraft::white_tulip},
        {"minecraft:white_wall_banner", minecraft::white_wall_banner},
        {"minecraft:white_wool", minecraft::white_wool},
        {"minecraft:wither_rose", minecraft::wither_rose},
        {"minecraft:wither_skeleton_skull", minecraft::wither_skeleton_skull},
        {"minecraft:wither_skeleton_wall_skull", minecraft::wither_skeleton_wall_skull},
        {"minecraft:yellow_banner", minecraft::yellow_banner},
        {"minecraft:yellow_bed", minecraft::yellow_bed},
        {"minecraft:yellow_carpet", minecraft::yellow_carpet},
        {"minecraft:yellow_concrete", minecraft::yellow_concrete},
        {"minecraft:yellow_concrete_powder", minecraft::yellow_concrete_powder},
        {"minecraft:yellow_glazed_terracotta", minecraft::yellow_glazed_terracotta},
        {"minecraft:yellow_shulker_box", minecraft::yellow_shulker_box},
        {"minecraft:yellow_stained_glass", minecraft::yellow_stained_glass},
        {"minecraft:yellow_stained_glass_pane", minecraft::yellow_stained_glass_pane},
        {"minecraft:yellow_terracotta", minecraft::yellow_terracotta},
        {"minecraft:yellow_wall_banner", minecraft::yellow_wall_banner},
        {"minecraft:yellow_wool", minecraft::yellow_wool},
        {"minecraft:zombie_head", minecraft::zombie_head},
        {"minecraft:zombie_wall_head", minecraft::zombie_wall_head},
    };
    auto mappingIt = mapping.find(name);
    if (mappingIt == mapping.end()) {
        return unknown;
    }
    return mappingIt->second;
}

static inline std::string Name(BlockId id) {
    static std::map<BlockId, std::string> mapping = {
        {minecraft::acacia_button, "minecraft:acacia_button"},
        {minecraft::acacia_door, "minecraft:acacia_door"},
        {minecraft::acacia_fence, "minecraft:acacia_fence"},
        {minecraft::acacia_fence_gate, "minecraft:acacia_fence_gate"},
        {minecraft::acacia_leaves, "minecraft:acacia_leaves"},
        {minecraft::acacia_log, "minecraft:acacia_log"},
        {minecraft::acacia_planks, "minecraft:acacia_planks"},
        {minecraft::acacia_pressure_plate, "minecraft:acacia_pressure_plate"},
        {minecraft::acacia_sapling, "minecraft:acacia_sapling"},
        {minecraft::acacia_sign, "minecraft:acacia_sign"},
        {minecraft::acacia_slab, "minecraft:acacia_slab"},
        {minecraft::acacia_stairs, "minecraft:acacia_stairs"},
        {minecraft::acacia_trapdoor, "minecraft:acacia_trapdoor"},
        {minecraft::acacia_wall_sign, "minecraft:acacia_wall_sign"},
        {minecraft::acacia_wood, "minecraft:acacia_wood"},
        {minecraft::activator_rail, "minecraft:activator_rail"},
        {minecraft::air, "minecraft:air"},
        {minecraft::allium, "minecraft:allium"},
        {minecraft::andesite, "minecraft:andesite"},
        {minecraft::andesite_slab, "minecraft:andesite_slab"},
        {minecraft::andesite_stairs, "minecraft:andesite_stairs"},
        {minecraft::andesite_wall, "minecraft:andesite_wall"},
        {minecraft::anvil, "minecraft:anvil"},
        {minecraft::attached_melon_stem, "minecraft:attached_melon_stem"},
        {minecraft::attached_pumpkin_stem, "minecraft:attached_pumpkin_stem"},
        {minecraft::azure_bluet, "minecraft:azure_bluet"},
        {minecraft::bamboo, "minecraft:bamboo"},
        {minecraft::bamboo_sapling, "minecraft:bamboo_sapling"},
        {minecraft::barrel, "minecraft:barrel"},
        {minecraft::barrier, "minecraft:barrier"},
        {minecraft::beacon, "minecraft:beacon"},
        {minecraft::bedrock, "minecraft:bedrock"},
        {minecraft::beetroots, "minecraft:beetroots"},
        {minecraft::bell, "minecraft:bell"},
        {minecraft::birch_button, "minecraft:birch_button"},
        {minecraft::birch_door, "minecraft:birch_door"},
        {minecraft::birch_fence, "minecraft:birch_fence"},
        {minecraft::birch_fence_gate, "minecraft:birch_fence_gate"},
        {minecraft::birch_leaves, "minecraft:birch_leaves"},
        {minecraft::birch_log, "minecraft:birch_log"},
        {minecraft::birch_planks, "minecraft:birch_planks"},
        {minecraft::birch_pressure_plate, "minecraft:birch_pressure_plate"},
        {minecraft::birch_sapling, "minecraft:birch_sapling"},
        {minecraft::birch_sign, "minecraft:birch_sign"},
        {minecraft::birch_slab, "minecraft:birch_slab"},
        {minecraft::birch_stairs, "minecraft:birch_stairs"},
        {minecraft::birch_trapdoor, "minecraft:birch_trapdoor"},
        {minecraft::birch_wall_sign, "minecraft:birch_wall_sign"},
        {minecraft::birch_wood, "minecraft:birch_wood"},
        {minecraft::black_banner, "minecraft:black_banner"},
        {minecraft::black_bed, "minecraft:black_bed"},
        {minecraft::black_carpet, "minecraft:black_carpet"},
        {minecraft::black_concrete, "minecraft:black_concrete"},
        {minecraft::black_concrete_powder, "minecraft:black_concrete_powder"},
        {minecraft::black_glazed_terracotta, "minecraft:black_glazed_terracotta"},
        {minecraft::black_shulker_box, "minecraft:black_shulker_box"},
        {minecraft::black_stained_glass, "minecraft:black_stained_glass"},
        {minecraft::black_stained_glass_pane, "minecraft:black_stained_glass_pane"},
        {minecraft::black_terracotta, "minecraft:black_terracotta"},
        {minecraft::black_wall_banner, "minecraft:black_wall_banner"},
        {minecraft::black_wool, "minecraft:black_wool"},
        {minecraft::blast_furnace, "minecraft:blast_furnace"},
        {minecraft::coal_block, "minecraft:coal_block"},
        {minecraft::diamond_block, "minecraft:diamond_block"},
        {minecraft::emerald_block, "minecraft:emerald_block"},
        {minecraft::gold_block, "minecraft:gold_block"},
        {minecraft::iron_block, "minecraft:iron_block"},
        {minecraft::quartz_block, "minecraft:quartz_block"},
        {minecraft::redstone_block, "minecraft:redstone_block"},
        {minecraft::blue_banner, "minecraft:blue_banner"},
        {minecraft::blue_bed, "minecraft:blue_bed"},
        {minecraft::blue_carpet, "minecraft:blue_carpet"},
        {minecraft::blue_concrete, "minecraft:blue_concrete"},
        {minecraft::blue_concrete_powder, "minecraft:blue_concrete_powder"},
        {minecraft::blue_glazed_terracotta, "minecraft:blue_glazed_terracotta"},
        {minecraft::blue_ice, "minecraft:blue_ice"},
        {minecraft::blue_orchid, "minecraft:blue_orchid"},
        {minecraft::blue_shulker_box, "minecraft:blue_shulker_box"},
        {minecraft::blue_stained_glass, "minecraft:blue_stained_glass"},
        {minecraft::blue_stained_glass_pane, "minecraft:blue_stained_glass_pane"},
        {minecraft::blue_terracotta, "minecraft:blue_terracotta"},
        {minecraft::blue_wall_banner, "minecraft:blue_wall_banner"},
        {minecraft::blue_wool, "minecraft:blue_wool"},
        {minecraft::bone_block, "minecraft:bone_block"},
        {minecraft::bookshelf, "minecraft:bookshelf"},
        {minecraft::brain_coral, "minecraft:brain_coral"},
        {minecraft::brain_coral_block, "minecraft:brain_coral_block"},
        {minecraft::brain_coral_fan, "minecraft:brain_coral_fan"},
        {minecraft::brain_coral_wall_fan, "minecraft:brain_coral_wall_fan"},
        {minecraft::brewing_stand, "minecraft:brewing_stand"},
        {minecraft::brick_slab, "minecraft:brick_slab"},
        {minecraft::brick_stairs, "minecraft:brick_stairs"},
        {minecraft::brick_wall, "minecraft:brick_wall"},
        {minecraft::bricks, "minecraft:bricks"},
        {minecraft::brown_banner, "minecraft:brown_banner"},
        {minecraft::brown_bed, "minecraft:brown_bed"},
        {minecraft::brown_carpet, "minecraft:brown_carpet"},
        {minecraft::brown_concrete, "minecraft:brown_concrete"},
        {minecraft::brown_concrete_powder, "minecraft:brown_concrete_powder"},
        {minecraft::brown_glazed_terracotta, "minecraft:brown_glazed_terracotta"},
        {minecraft::brown_mushroom, "minecraft:brown_mushroom"},
        {minecraft::brown_mushroom_block, "minecraft:brown_mushroom_block"},
        {minecraft::brown_shulker_box, "minecraft:brown_shulker_box"},
        {minecraft::brown_stained_glass, "minecraft:brown_stained_glass"},
        {minecraft::brown_stained_glass_pane, "minecraft:brown_stained_glass_pane"},
        {minecraft::brown_terracotta, "minecraft:brown_terracotta"},
        {minecraft::brown_wall_banner, "minecraft:brown_wall_banner"},
        {minecraft::brown_wool, "minecraft:brown_wool"},
        {minecraft::bubble_column, "minecraft:bubble_column"},
        {minecraft::bubble_coral, "minecraft:bubble_coral"},
        {minecraft::bubble_coral_block, "minecraft:bubble_coral_block"},
        {minecraft::bubble_coral_fan, "minecraft:bubble_coral_fan"},
        {minecraft::bubble_coral_wall_fan, "minecraft:bubble_coral_wall_fan"},
        {minecraft::cactus, "minecraft:cactus"},
        {minecraft::cake, "minecraft:cake"},
        {minecraft::campfire, "minecraft:campfire"},
        {minecraft::carrots, "minecraft:carrots"},
        {minecraft::cartography_table, "minecraft:cartography_table"},
        {minecraft::carved_pumpkin, "minecraft:carved_pumpkin"},
        {minecraft::cauldron, "minecraft:cauldron"},
        {minecraft::cave_air, "minecraft:cave_air"},
        {minecraft::chain_command_block, "minecraft:chain_command_block"},
        {minecraft::chest, "minecraft:chest"},
        {minecraft::chipped_anvil, "minecraft:chipped_anvil"},
        {minecraft::chiseled_quartz_block, "minecraft:chiseled_quartz_block"},
        {minecraft::chiseled_red_sandstone, "minecraft:chiseled_red_sandstone"},
        {minecraft::chiseled_sandstone, "minecraft:chiseled_sandstone"},
        {minecraft::chiseled_stone_bricks, "minecraft:chiseled_stone_bricks"},
        {minecraft::chorus_flower, "minecraft:chorus_flower"},
        {minecraft::chorus_plant, "minecraft:chorus_plant"},
        {minecraft::clay, "minecraft:clay"},
        {minecraft::coal_ore, "minecraft:coal_ore"},
        {minecraft::coarse_dirt, "minecraft:coarse_dirt"},
        {minecraft::cobblestone, "minecraft:cobblestone"},
        {minecraft::cobblestone_slab, "minecraft:cobblestone_slab"},
        {minecraft::cobblestone_stairs, "minecraft:cobblestone_stairs"},
        {minecraft::cobblestone_wall, "minecraft:cobblestone_wall"},
        {minecraft::cobweb, "minecraft:cobweb"},
        {minecraft::cocoa, "minecraft:cocoa"},
        {minecraft::command_block, "minecraft:command_block"},
        {minecraft::composter, "minecraft:composter"},
        {minecraft::conduit, "minecraft:conduit"},
        {minecraft::cornflower, "minecraft:cornflower"},
        {minecraft::cracked_stone_bricks, "minecraft:cracked_stone_bricks"},
        {minecraft::crafting_table, "minecraft:crafting_table"},
        {minecraft::creeper_head, "minecraft:creeper_head"},
        {minecraft::creeper_wall_head, "minecraft:creeper_wall_head"},
        {minecraft::cut_red_sandstone, "minecraft:cut_red_sandstone"},
        {minecraft::cut_red_sandstone_slab, "minecraft:cut_red_sandstone_slab"},
        {minecraft::cut_sandstone, "minecraft:cut_sandstone"},
        {minecraft::cut_sandstone_slab, "minecraft:cut_sandstone_slab"},
        {minecraft::cyan_banner, "minecraft:cyan_banner"},
        {minecraft::cyan_bed, "minecraft:cyan_bed"},
        {minecraft::cyan_carpet, "minecraft:cyan_carpet"},
        {minecraft::cyan_concrete, "minecraft:cyan_concrete"},
        {minecraft::cyan_concrete_powder, "minecraft:cyan_concrete_powder"},
        {minecraft::cyan_glazed_terracotta, "minecraft:cyan_glazed_terracotta"},
        {minecraft::cyan_shulker_box, "minecraft:cyan_shulker_box"},
        {minecraft::cyan_stained_glass, "minecraft:cyan_stained_glass"},
        {minecraft::cyan_stained_glass_pane, "minecraft:cyan_stained_glass_pane"},
        {minecraft::cyan_terracotta, "minecraft:cyan_terracotta"},
        {minecraft::cyan_wall_banner, "minecraft:cyan_wall_banner"},
        {minecraft::cyan_wool, "minecraft:cyan_wool"},
        {minecraft::damaged_anvil, "minecraft:damaged_anvil"},
        {minecraft::dandelion, "minecraft:dandelion"},
        {minecraft::dark_oak_button, "minecraft:dark_oak_button"},
        {minecraft::dark_oak_door, "minecraft:dark_oak_door"},
        {minecraft::dark_oak_fence, "minecraft:dark_oak_fence"},
        {minecraft::dark_oak_fence_gate, "minecraft:dark_oak_fence_gate"},
        {minecraft::dark_oak_leaves, "minecraft:dark_oak_leaves"},
        {minecraft::dark_oak_log, "minecraft:dark_oak_log"},
        {minecraft::dark_oak_planks, "minecraft:dark_oak_planks"},
        {minecraft::dark_oak_pressure_plate, "minecraft:dark_oak_pressure_plate"},
        {minecraft::dark_oak_sapling, "minecraft:dark_oak_sapling"},
        {minecraft::dark_oak_sign, "minecraft:dark_oak_sign"},
        {minecraft::dark_oak_slab, "minecraft:dark_oak_slab"},
        {minecraft::dark_oak_stairs, "minecraft:dark_oak_stairs"},
        {minecraft::dark_oak_trapdoor, "minecraft:dark_oak_trapdoor"},
        {minecraft::dark_oak_wall_sign, "minecraft:dark_oak_wall_sign"},
        {minecraft::dark_oak_wood, "minecraft:dark_oak_wood"},
        {minecraft::dark_prismarine, "minecraft:dark_prismarine"},
        {minecraft::dark_prismarine_slab, "minecraft:dark_prismarine_slab"},
        {minecraft::dark_prismarine_stairs, "minecraft:dark_prismarine_stairs"},
        {minecraft::daylight_detector, "minecraft:daylight_detector"},
        {minecraft::dead_brain_coral, "minecraft:dead_brain_coral"},
        {minecraft::dead_brain_coral_block, "minecraft:dead_brain_coral_block"},
        {minecraft::dead_brain_coral_fan, "minecraft:dead_brain_coral_fan"},
        {minecraft::dead_brain_coral_wall_fan, "minecraft:dead_brain_coral_wall_fan"},
        {minecraft::dead_bubble_coral, "minecraft:dead_bubble_coral"},
        {minecraft::dead_bubble_coral_block, "minecraft:dead_bubble_coral_block"},
        {minecraft::dead_bubble_coral_fan, "minecraft:dead_bubble_coral_fan"},
        {minecraft::dead_bubble_coral_wall_fan, "minecraft:dead_bubble_coral_wall_fan"},
        {minecraft::dead_bush, "minecraft:dead_bush"},
        {minecraft::dead_fire_coral, "minecraft:dead_fire_coral"},
        {minecraft::dead_fire_coral_block, "minecraft:dead_fire_coral_block"},
        {minecraft::dead_fire_coral_fan, "minecraft:dead_fire_coral_fan"},
        {minecraft::dead_fire_coral_wall_fan, "minecraft:dead_fire_coral_wall_fan"},
        {minecraft::dead_horn_coral, "minecraft:dead_horn_coral"},
        {minecraft::dead_horn_coral_block, "minecraft:dead_horn_coral_block"},
        {minecraft::dead_horn_coral_fan, "minecraft:dead_horn_coral_fan"},
        {minecraft::dead_horn_coral_wall_fan, "minecraft:dead_horn_coral_wall_fan"},
        {minecraft::dead_tube_coral, "minecraft:dead_tube_coral"},
        {minecraft::dead_tube_coral_block, "minecraft:dead_tube_coral_block"},
        {minecraft::dead_tube_coral_fan, "minecraft:dead_tube_coral_fan"},
        {minecraft::dead_tube_coral_wall_fan, "minecraft:dead_tube_coral_wall_fan"},
        {minecraft::detector_rail, "minecraft:detector_rail"},
        {minecraft::diamond_ore, "minecraft:diamond_ore"},
        {minecraft::diorite, "minecraft:diorite"},
        {minecraft::diorite_slab, "minecraft:diorite_slab"},
        {minecraft::diorite_stairs, "minecraft:diorite_stairs"},
        {minecraft::diorite_wall, "minecraft:diorite_wall"},
        {minecraft::dirt, "minecraft:dirt"},
        {minecraft::dispenser, "minecraft:dispenser"},
        {minecraft::dragon_egg, "minecraft:dragon_egg"},
        {minecraft::dragon_head, "minecraft:dragon_head"},
        {minecraft::dragon_wall_head, "minecraft:dragon_wall_head"},
        {minecraft::dried_kelp_block, "minecraft:dried_kelp_block"},
        {minecraft::dropper, "minecraft:dropper"},
        {minecraft::emerald_ore, "minecraft:emerald_ore"},
        {minecraft::enchanting_table, "minecraft:enchanting_table"},
        {minecraft::end_gateway, "minecraft:end_gateway"},
        {minecraft::end_portal, "minecraft:end_portal"},
        {minecraft::end_portal_frame, "minecraft:end_portal_frame"},
        {minecraft::end_rod, "minecraft:end_rod"},
        {minecraft::end_stone, "minecraft:end_stone"},
        {minecraft::end_stone_brick_slab, "minecraft:end_stone_brick_slab"},
        {minecraft::end_stone_brick_stairs, "minecraft:end_stone_brick_stairs"},
        {minecraft::end_stone_brick_wall, "minecraft:end_stone_brick_wall"},
        {minecraft::end_stone_bricks, "minecraft:end_stone_bricks"},
        {minecraft::ender_chest, "minecraft:ender_chest"},
        {minecraft::farmland, "minecraft:farmland"},
        {minecraft::fern, "minecraft:fern"},
        {minecraft::fire, "minecraft:fire"},
        {minecraft::fire_coral, "minecraft:fire_coral"},
        {minecraft::fire_coral_block, "minecraft:fire_coral_block"},
        {minecraft::fire_coral_fan, "minecraft:fire_coral_fan"},
        {minecraft::fire_coral_wall_fan, "minecraft:fire_coral_wall_fan"},
        {minecraft::fletching_table, "minecraft:fletching_table"},
        {minecraft::flower_pot, "minecraft:flower_pot"},
        {minecraft::frosted_ice, "minecraft:frosted_ice"},
        {minecraft::furnace, "minecraft:furnace"},
        {minecraft::glass, "minecraft:glass"},
        {minecraft::glass_pane, "minecraft:glass_pane"},
        {minecraft::glowstone, "minecraft:glowstone"},
        {minecraft::gold_ore, "minecraft:gold_ore"},
        {minecraft::granite, "minecraft:granite"},
        {minecraft::granite_slab, "minecraft:granite_slab"},
        {minecraft::granite_stairs, "minecraft:granite_stairs"},
        {minecraft::granite_wall, "minecraft:granite_wall"},
        {minecraft::grass, "minecraft:grass"},
        {minecraft::grass_block, "minecraft:grass_block"},
        {minecraft::grass_path, "minecraft:grass_path"},
        {minecraft::gravel, "minecraft:gravel"},
        {minecraft::gray_banner, "minecraft:gray_banner"},
        {minecraft::gray_bed, "minecraft:gray_bed"},
        {minecraft::gray_carpet, "minecraft:gray_carpet"},
        {minecraft::gray_concrete, "minecraft:gray_concrete"},
        {minecraft::gray_concrete_powder, "minecraft:gray_concrete_powder"},
        {minecraft::gray_glazed_terracotta, "minecraft:gray_glazed_terracotta"},
        {minecraft::gray_shulker_box, "minecraft:gray_shulker_box"},
        {minecraft::gray_stained_glass, "minecraft:gray_stained_glass"},
        {minecraft::gray_stained_glass_pane, "minecraft:gray_stained_glass_pane"},
        {minecraft::gray_terracotta, "minecraft:gray_terracotta"},
        {minecraft::gray_wall_banner, "minecraft:gray_wall_banner"},
        {minecraft::gray_wool, "minecraft:gray_wool"},
        {minecraft::green_banner, "minecraft:green_banner"},
        {minecraft::green_bed, "minecraft:green_bed"},
        {minecraft::green_carpet, "minecraft:green_carpet"},
        {minecraft::green_concrete, "minecraft:green_concrete"},
        {minecraft::green_concrete_powder, "minecraft:green_concrete_powder"},
        {minecraft::green_glazed_terracotta, "minecraft:green_glazed_terracotta"},
        {minecraft::green_shulker_box, "minecraft:green_shulker_box"},
        {minecraft::green_stained_glass, "minecraft:green_stained_glass"},
        {minecraft::green_stained_glass_pane, "minecraft:green_stained_glass_pane"},
        {minecraft::green_terracotta, "minecraft:green_terracotta"},
        {minecraft::green_wall_banner, "minecraft:green_wall_banner"},
        {minecraft::green_wool, "minecraft:green_wool"},
        {minecraft::grindstone, "minecraft:grindstone"},
        {minecraft::hay_block, "minecraft:hay_block"},
        {minecraft::heavy_weighted_pressure_plate, "minecraft:heavy_weighted_pressure_plate"},
        {minecraft::hopper, "minecraft:hopper"},
        {minecraft::horn_coral, "minecraft:horn_coral"},
        {minecraft::horn_coral_block, "minecraft:horn_coral_block"},
        {minecraft::horn_coral_fan, "minecraft:horn_coral_fan"},
        {minecraft::horn_coral_wall_fan, "minecraft:horn_coral_wall_fan"},
        {minecraft::ice, "minecraft:ice"},
        {minecraft::infested_chiseled_stone_bricks, "minecraft:infested_chiseled_stone_bricks"},
        {minecraft::infested_cobblestone, "minecraft:infested_cobblestone"},
        {minecraft::infested_cracked_stone_bricks, "minecraft:infested_cracked_stone_bricks"},
        {minecraft::infested_mossy_stone_bricks, "minecraft:infested_mossy_stone_bricks"},
        {minecraft::infested_stone, "minecraft:infested_stone"},
        {minecraft::infested_stone_bricks, "minecraft:infested_stone_bricks"},
        {minecraft::iron_bars, "minecraft:iron_bars"},
        {minecraft::iron_door, "minecraft:iron_door"},
        {minecraft::iron_ore, "minecraft:iron_ore"},
        {minecraft::iron_trapdoor, "minecraft:iron_trapdoor"},
        {minecraft::jack_o_lantern, "minecraft:jack_o_lantern"},
        {minecraft::jigsaw, "minecraft:jigsaw"},
        {minecraft::jukebox, "minecraft:jukebox"},
        {minecraft::jungle_button, "minecraft:jungle_button"},
        {minecraft::jungle_door, "minecraft:jungle_door"},
        {minecraft::jungle_fence, "minecraft:jungle_fence"},
        {minecraft::jungle_fence_gate, "minecraft:jungle_fence_gate"},
        {minecraft::jungle_leaves, "minecraft:jungle_leaves"},
        {minecraft::jungle_log, "minecraft:jungle_log"},
        {minecraft::jungle_planks, "minecraft:jungle_planks"},
        {minecraft::jungle_pressure_plate, "minecraft:jungle_pressure_plate"},
        {minecraft::jungle_sapling, "minecraft:jungle_sapling"},
        {minecraft::jungle_sign, "minecraft:jungle_sign"},
        {minecraft::jungle_slab, "minecraft:jungle_slab"},
        {minecraft::jungle_stairs, "minecraft:jungle_stairs"},
        {minecraft::jungle_trapdoor, "minecraft:jungle_trapdoor"},
        {minecraft::jungle_wall_sign, "minecraft:jungle_wall_sign"},
        {minecraft::jungle_wood, "minecraft:jungle_wood"},
        {minecraft::kelp, "minecraft:kelp"},
        {minecraft::kelp_plant, "minecraft:kelp_plant"},
        {minecraft::ladder, "minecraft:ladder"},
        {minecraft::lantern, "minecraft:lantern"},
        {minecraft::lapis_block, "minecraft:lapis_block"},
        {minecraft::lapis_ore, "minecraft:lapis_ore"},
        {minecraft::large_fern, "minecraft:large_fern"},
        {minecraft::lava, "minecraft:lava"},
        {minecraft::lectern, "minecraft:lectern"},
        {minecraft::lever, "minecraft:lever"},
        {minecraft::light_blue_banner, "minecraft:light_blue_banner"},
        {minecraft::light_blue_bed, "minecraft:light_blue_bed"},
        {minecraft::light_blue_carpet, "minecraft:light_blue_carpet"},
        {minecraft::light_blue_concrete, "minecraft:light_blue_concrete"},
        {minecraft::light_blue_concrete_powder, "minecraft:light_blue_concrete_powder"},
        {minecraft::light_blue_glazed_terracotta, "minecraft:light_blue_glazed_terracotta"},
        {minecraft::light_blue_shulker_box, "minecraft:light_blue_shulker_box"},
        {minecraft::light_blue_stained_glass, "minecraft:light_blue_stained_glass"},
        {minecraft::light_blue_stained_glass_pane, "minecraft:light_blue_stained_glass_pane"},
        {minecraft::light_blue_terracotta, "minecraft:light_blue_terracotta"},
        {minecraft::light_blue_wall_banner, "minecraft:light_blue_wall_banner"},
        {minecraft::light_blue_wool, "minecraft:light_blue_wool"},
        {minecraft::light_gray_banner, "minecraft:light_gray_banner"},
        {minecraft::light_gray_bed, "minecraft:light_gray_bed"},
        {minecraft::light_gray_carpet, "minecraft:light_gray_carpet"},
        {minecraft::light_gray_concrete, "minecraft:light_gray_concrete"},
        {minecraft::light_gray_concrete_powder, "minecraft:light_gray_concrete_powder"},
        {minecraft::light_gray_glazed_terracotta, "minecraft:light_gray_glazed_terracotta"},
        {minecraft::light_gray_shulker_box, "minecraft:light_gray_shulker_box"},
        {minecraft::light_gray_stained_glass, "minecraft:light_gray_stained_glass"},
        {minecraft::light_gray_stained_glass_pane, "minecraft:light_gray_stained_glass_pane"},
        {minecraft::light_gray_terracotta, "minecraft:light_gray_terracotta"},
        {minecraft::light_gray_wall_banner, "minecraft:light_gray_wall_banner"},
        {minecraft::light_gray_wool, "minecraft:light_gray_wool"},
        {minecraft::light_weighted_pressure_plate, "minecraft:light_weighted_pressure_plate"},
        {minecraft::lilac, "minecraft:lilac"},
        {minecraft::lily_of_the_valley, "minecraft:lily_of_the_valley"},
        {minecraft::lily_pad, "minecraft:lily_pad"},
        {minecraft::lime_banner, "minecraft:lime_banner"},
        {minecraft::lime_bed, "minecraft:lime_bed"},
        {minecraft::lime_carpet, "minecraft:lime_carpet"},
        {minecraft::lime_concrete, "minecraft:lime_concrete"},
        {minecraft::lime_concrete_powder, "minecraft:lime_concrete_powder"},
        {minecraft::lime_glazed_terracotta, "minecraft:lime_glazed_terracotta"},
        {minecraft::lime_shulker_box, "minecraft:lime_shulker_box"},
        {minecraft::lime_stained_glass, "minecraft:lime_stained_glass"},
        {minecraft::lime_stained_glass_pane, "minecraft:lime_stained_glass_pane"},
        {minecraft::lime_terracotta, "minecraft:lime_terracotta"},
        {minecraft::lime_wall_banner, "minecraft:lime_wall_banner"},
        {minecraft::lime_wool, "minecraft:lime_wool"},
        {minecraft::loom, "minecraft:loom"},
        {minecraft::magenta_banner, "minecraft:magenta_banner"},
        {minecraft::magenta_bed, "minecraft:magenta_bed"},
        {minecraft::magenta_carpet, "minecraft:magenta_carpet"},
        {minecraft::magenta_concrete, "minecraft:magenta_concrete"},
        {minecraft::magenta_concrete_powder, "minecraft:magenta_concrete_powder"},
        {minecraft::magenta_glazed_terracotta, "minecraft:magenta_glazed_terracotta"},
        {minecraft::magenta_shulker_box, "minecraft:magenta_shulker_box"},
        {minecraft::magenta_stained_glass, "minecraft:magenta_stained_glass"},
        {minecraft::magenta_stained_glass_pane, "minecraft:magenta_stained_glass_pane"},
        {minecraft::magenta_terracotta, "minecraft:magenta_terracotta"},
        {minecraft::magenta_wall_banner, "minecraft:magenta_wall_banner"},
        {minecraft::magenta_wool, "minecraft:magenta_wool"},
        {minecraft::magma_block, "minecraft:magma_block"},
        {minecraft::melon, "minecraft:melon"},
        {minecraft::melon_stem, "minecraft:melon_stem"},
        {minecraft::mossy_cobblestone, "minecraft:mossy_cobblestone"},
        {minecraft::mossy_cobblestone_slab, "minecraft:mossy_cobblestone_slab"},
        {minecraft::mossy_cobblestone_stairs, "minecraft:mossy_cobblestone_stairs"},
        {minecraft::mossy_cobblestone_wall, "minecraft:mossy_cobblestone_wall"},
        {minecraft::mossy_stone_brick_slab, "minecraft:mossy_stone_brick_slab"},
        {minecraft::mossy_stone_brick_stairs, "minecraft:mossy_stone_brick_stairs"},
        {minecraft::mossy_stone_brick_wall, "minecraft:mossy_stone_brick_wall"},
        {minecraft::mossy_stone_bricks, "minecraft:mossy_stone_bricks"},
        {minecraft::moving_piston, "minecraft:moving_piston"},
        {minecraft::mushroom_stem, "minecraft:mushroom_stem"},
        {minecraft::mycelium, "minecraft:mycelium"},
        {minecraft::nether_brick_fence, "minecraft:nether_brick_fence"},
        {minecraft::nether_brick_slab, "minecraft:nether_brick_slab"},
        {minecraft::nether_brick_stairs, "minecraft:nether_brick_stairs"},
        {minecraft::nether_brick_wall, "minecraft:nether_brick_wall"},
        {minecraft::nether_bricks, "minecraft:nether_bricks"},
        {minecraft::nether_portal, "minecraft:nether_portal"},
        {minecraft::nether_quartz_ore, "minecraft:nether_quartz_ore"},
        {minecraft::nether_wart, "minecraft:nether_wart"},
        {minecraft::nether_wart_block, "minecraft:nether_wart_block"},
        {minecraft::netherrack, "minecraft:netherrack"},
        {minecraft::note_block, "minecraft:note_block"},
        {minecraft::oak_button, "minecraft:oak_button"},
        {minecraft::oak_door, "minecraft:oak_door"},
        {minecraft::oak_fence, "minecraft:oak_fence"},
        {minecraft::oak_fence_gate, "minecraft:oak_fence_gate"},
        {minecraft::oak_leaves, "minecraft:oak_leaves"},
        {minecraft::oak_log, "minecraft:oak_log"},
        {minecraft::oak_planks, "minecraft:oak_planks"},
        {minecraft::oak_pressure_plate, "minecraft:oak_pressure_plate"},
        {minecraft::oak_sapling, "minecraft:oak_sapling"},
        {minecraft::oak_sign, "minecraft:oak_sign"},
        {minecraft::oak_slab, "minecraft:oak_slab"},
        {minecraft::oak_stairs, "minecraft:oak_stairs"},
        {minecraft::oak_trapdoor, "minecraft:oak_trapdoor"},
        {minecraft::oak_wall_sign, "minecraft:oak_wall_sign"},
        {minecraft::oak_wood, "minecraft:oak_wood"},
        {minecraft::observer, "minecraft:observer"},
        {minecraft::obsidian, "minecraft:obsidian"},
        {minecraft::orange_banner, "minecraft:orange_banner"},
        {minecraft::orange_bed, "minecraft:orange_bed"},
        {minecraft::orange_carpet, "minecraft:orange_carpet"},
        {minecraft::orange_concrete, "minecraft:orange_concrete"},
        {minecraft::orange_concrete_powder, "minecraft:orange_concrete_powder"},
        {minecraft::orange_glazed_terracotta, "minecraft:orange_glazed_terracotta"},
        {minecraft::orange_shulker_box, "minecraft:orange_shulker_box"},
        {minecraft::orange_stained_glass, "minecraft:orange_stained_glass"},
        {minecraft::orange_stained_glass_pane, "minecraft:orange_stained_glass_pane"},
        {minecraft::orange_terracotta, "minecraft:orange_terracotta"},
        {minecraft::orange_tulip, "minecraft:orange_tulip"},
        {minecraft::orange_wall_banner, "minecraft:orange_wall_banner"},
        {minecraft::orange_wool, "minecraft:orange_wool"},
        {minecraft::oxeye_daisy, "minecraft:oxeye_daisy"},
        {minecraft::packed_ice, "minecraft:packed_ice"},
        {minecraft::peony, "minecraft:peony"},
        {minecraft::petrified_oak_slab, "minecraft:petrified_oak_slab"},
        {minecraft::pink_banner, "minecraft:pink_banner"},
        {minecraft::pink_bed, "minecraft:pink_bed"},
        {minecraft::pink_carpet, "minecraft:pink_carpet"},
        {minecraft::pink_concrete, "minecraft:pink_concrete"},
        {minecraft::pink_concrete_powder, "minecraft:pink_concrete_powder"},
        {minecraft::pink_glazed_terracotta, "minecraft:pink_glazed_terracotta"},
        {minecraft::pink_shulker_box, "minecraft:pink_shulker_box"},
        {minecraft::pink_stained_glass, "minecraft:pink_stained_glass"},
        {minecraft::pink_stained_glass_pane, "minecraft:pink_stained_glass_pane"},
        {minecraft::pink_terracotta, "minecraft:pink_terracotta"},
        {minecraft::pink_tulip, "minecraft:pink_tulip"},
        {minecraft::pink_wall_banner, "minecraft:pink_wall_banner"},
        {minecraft::pink_wool, "minecraft:pink_wool"},
        {minecraft::piston, "minecraft:piston"},
        {minecraft::piston_head, "minecraft:piston_head"},
        {minecraft::player_head, "minecraft:player_head"},
        {minecraft::player_wall_head, "minecraft:player_wall_head"},
        {minecraft::podzol, "minecraft:podzol"},
        {minecraft::polished_andesite, "minecraft:polished_andesite"},
        {minecraft::polished_andesite_slab, "minecraft:polished_andesite_slab"},
        {minecraft::polished_andesite_stairs, "minecraft:polished_andesite_stairs"},
        {minecraft::polished_diorite, "minecraft:polished_diorite"},
        {minecraft::polished_diorite_slab, "minecraft:polished_diorite_slab"},
        {minecraft::polished_diorite_stairs, "minecraft:polished_diorite_stairs"},
        {minecraft::polished_granite, "minecraft:polished_granite"},
        {minecraft::polished_granite_slab, "minecraft:polished_granite_slab"},
        {minecraft::polished_granite_stairs, "minecraft:polished_granite_stairs"},
        {minecraft::poppy, "minecraft:poppy"},
        {minecraft::potatoes, "minecraft:potatoes"},
        {minecraft::potted_acacia_sapling, "minecraft:potted_acacia_sapling"},
        {minecraft::potted_allium, "minecraft:potted_allium"},
        {minecraft::potted_azure_bluet, "minecraft:potted_azure_bluet"},
        {minecraft::potted_bamboo, "minecraft:potted_bamboo"},
        {minecraft::potted_birch_sapling, "minecraft:potted_birch_sapling"},
        {minecraft::potted_blue_orchid, "minecraft:potted_blue_orchid"},
        {minecraft::potted_brown_mushroom, "minecraft:potted_brown_mushroom"},
        {minecraft::potted_cactus, "minecraft:potted_cactus"},
        {minecraft::potted_cornflower, "minecraft:potted_cornflower"},
        {minecraft::potted_dandelion, "minecraft:potted_dandelion"},
        {minecraft::potted_dark_oak_sapling, "minecraft:potted_dark_oak_sapling"},
        {minecraft::potted_dead_bush, "minecraft:potted_dead_bush"},
        {minecraft::potted_fern, "minecraft:potted_fern"},
        {minecraft::potted_jungle_sapling, "minecraft:potted_jungle_sapling"},
        {minecraft::potted_lily_of_the_valley, "minecraft:potted_lily_of_the_valley"},
        {minecraft::potted_oak_sapling, "minecraft:potted_oak_sapling"},
        {minecraft::potted_orange_tulip, "minecraft:potted_orange_tulip"},
        {minecraft::potted_oxeye_daisy, "minecraft:potted_oxeye_daisy"},
        {minecraft::potted_pink_tulip, "minecraft:potted_pink_tulip"},
        {minecraft::potted_poppy, "minecraft:potted_poppy"},
        {minecraft::potted_red_mushroom, "minecraft:potted_red_mushroom"},
        {minecraft::potted_red_tulip, "minecraft:potted_red_tulip"},
        {minecraft::potted_spruce_sapling, "minecraft:potted_spruce_sapling"},
        {minecraft::potted_white_tulip, "minecraft:potted_white_tulip"},
        {minecraft::potted_wither_rose, "minecraft:potted_wither_rose"},
        {minecraft::powered_rail, "minecraft:powered_rail"},
        {minecraft::prismarine, "minecraft:prismarine"},
        {minecraft::prismarine_brick_slab, "minecraft:prismarine_brick_slab"},
        {minecraft::prismarine_brick_stairs, "minecraft:prismarine_brick_stairs"},
        {minecraft::prismarine_bricks, "minecraft:prismarine_bricks"},
        {minecraft::prismarine_slab, "minecraft:prismarine_slab"},
        {minecraft::prismarine_stairs, "minecraft:prismarine_stairs"},
        {minecraft::prismarine_wall, "minecraft:prismarine_wall"},
        {minecraft::pumpkin, "minecraft:pumpkin"},
        {minecraft::pumpkin_stem, "minecraft:pumpkin_stem"},
        {minecraft::purple_banner, "minecraft:purple_banner"},
        {minecraft::purple_bed, "minecraft:purple_bed"},
        {minecraft::purple_carpet, "minecraft:purple_carpet"},
        {minecraft::purple_concrete, "minecraft:purple_concrete"},
        {minecraft::purple_concrete_powder, "minecraft:purple_concrete_powder"},
        {minecraft::purple_glazed_terracotta, "minecraft:purple_glazed_terracotta"},
        {minecraft::purple_shulker_box, "minecraft:purple_shulker_box"},
        {minecraft::purple_stained_glass, "minecraft:purple_stained_glass"},
        {minecraft::purple_stained_glass_pane, "minecraft:purple_stained_glass_pane"},
        {minecraft::purple_terracotta, "minecraft:purple_terracotta"},
        {minecraft::purple_wall_banner, "minecraft:purple_wall_banner"},
        {minecraft::purple_wool, "minecraft:purple_wool"},
        {minecraft::purpur_block, "minecraft:purpur_block"},
        {minecraft::purpur_pillar, "minecraft:purpur_pillar"},
        {minecraft::purpur_slab, "minecraft:purpur_slab"},
        {minecraft::purpur_stairs, "minecraft:purpur_stairs"},
        {minecraft::quartz_pillar, "minecraft:quartz_pillar"},
        {minecraft::quartz_slab, "minecraft:quartz_slab"},
        {minecraft::quartz_stairs, "minecraft:quartz_stairs"},
        {minecraft::rail, "minecraft:rail"},
        {minecraft::red_banner, "minecraft:red_banner"},
        {minecraft::red_bed, "minecraft:red_bed"},
        {minecraft::red_carpet, "minecraft:red_carpet"},
        {minecraft::red_concrete, "minecraft:red_concrete"},
        {minecraft::red_concrete_powder, "minecraft:red_concrete_powder"},
        {minecraft::red_glazed_terracotta, "minecraft:red_glazed_terracotta"},
        {minecraft::red_mushroom, "minecraft:red_mushroom"},
        {minecraft::red_mushroom_block, "minecraft:red_mushroom_block"},
        {minecraft::red_nether_brick_slab, "minecraft:red_nether_brick_slab"},
        {minecraft::red_nether_brick_stairs, "minecraft:red_nether_brick_stairs"},
        {minecraft::red_nether_brick_wall, "minecraft:red_nether_brick_wall"},
        {minecraft::red_nether_bricks, "minecraft:red_nether_bricks"},
        {minecraft::red_sand, "minecraft:red_sand"},
        {minecraft::red_sandstone, "minecraft:red_sandstone"},
        {minecraft::red_sandstone_slab, "minecraft:red_sandstone_slab"},
        {minecraft::red_sandstone_stairs, "minecraft:red_sandstone_stairs"},
        {minecraft::red_sandstone_wall, "minecraft:red_sandstone_wall"},
        {minecraft::red_shulker_box, "minecraft:red_shulker_box"},
        {minecraft::red_stained_glass, "minecraft:red_stained_glass"},
        {minecraft::red_stained_glass_pane, "minecraft:red_stained_glass_pane"},
        {minecraft::red_terracotta, "minecraft:red_terracotta"},
        {minecraft::red_tulip, "minecraft:red_tulip"},
        {minecraft::red_wall_banner, "minecraft:red_wall_banner"},
        {minecraft::red_wool, "minecraft:red_wool"},
        {minecraft::comparator, "minecraft:comparator"},
        {minecraft::redstone_wire, "minecraft:redstone_wire"},
        {minecraft::redstone_lamp, "minecraft:redstone_lamp"},
        {minecraft::redstone_ore, "minecraft:redstone_ore"},
        {minecraft::repeater, "minecraft:repeater"},
        {minecraft::redstone_torch, "minecraft:redstone_torch"},
        {minecraft::redstone_wall_torch, "minecraft:redstone_wall_torch"},
        {minecraft::repeating_command_block, "minecraft:repeating_command_block"},
        {minecraft::rose_bush, "minecraft:rose_bush"},
        {minecraft::sand, "minecraft:sand"},
        {minecraft::sandstone, "minecraft:sandstone"},
        {minecraft::sandstone_slab, "minecraft:sandstone_slab"},
        {minecraft::sandstone_stairs, "minecraft:sandstone_stairs"},
        {minecraft::sandstone_wall, "minecraft:sandstone_wall"},
        {minecraft::scaffolding, "minecraft:scaffolding"},
        {minecraft::sea_lantern, "minecraft:sea_lantern"},
        {minecraft::sea_pickle, "minecraft:sea_pickle"},
        {minecraft::seagrass, "minecraft:seagrass"},
        {minecraft::shulker_box, "minecraft:shulker_box"},
        {minecraft::skeleton_skull, "minecraft:skeleton_skull"},
        {minecraft::skeleton_wall_skull, "minecraft:skeleton_wall_skull"},
        {minecraft::slime_block, "minecraft:slime_block"},
        {minecraft::smithing_table, "minecraft:smithing_table"},
        {minecraft::smoker, "minecraft:smoker"},
        {minecraft::smooth_quartz, "minecraft:smooth_quartz"},
        {minecraft::smooth_quartz_slab, "minecraft:smooth_quartz_slab"},
        {minecraft::smooth_quartz_stairs, "minecraft:smooth_quartz_stairs"},
        {minecraft::smooth_red_sandstone, "minecraft:smooth_red_sandstone"},
        {minecraft::smooth_red_sandstone_slab, "minecraft:smooth_red_sandstone_slab"},
        {minecraft::smooth_red_sandstone_stairs, "minecraft:smooth_red_sandstone_stairs"},
        {minecraft::smooth_sandstone, "minecraft:smooth_sandstone"},
        {minecraft::smooth_sandstone_slab, "minecraft:smooth_sandstone_slab"},
        {minecraft::smooth_sandstone_stairs, "minecraft:smooth_sandstone_stairs"},
        {minecraft::smooth_stone, "minecraft:smooth_stone"},
        {minecraft::smooth_stone_slab, "minecraft:smooth_stone_slab"},
        {minecraft::snow, "minecraft:snow"},
        {minecraft::snow_block, "minecraft:snow_block"},
        {minecraft::soul_sand, "minecraft:soul_sand"},
        {minecraft::spawner, "minecraft:spawner"},
        {minecraft::sponge, "minecraft:sponge"},
        {minecraft::spruce_button, "minecraft:spruce_button"},
        {minecraft::spruce_door, "minecraft:spruce_door"},
        {minecraft::spruce_fence, "minecraft:spruce_fence"},
        {minecraft::spruce_fence_gate, "minecraft:spruce_fence_gate"},
        {minecraft::spruce_leaves, "minecraft:spruce_leaves"},
        {minecraft::spruce_log, "minecraft:spruce_log"},
        {minecraft::spruce_planks, "minecraft:spruce_planks"},
        {minecraft::spruce_pressure_plate, "minecraft:spruce_pressure_plate"},
        {minecraft::spruce_sapling, "minecraft:spruce_sapling"},
        {minecraft::spruce_sign, "minecraft:spruce_sign"},
        {minecraft::spruce_slab, "minecraft:spruce_slab"},
        {minecraft::spruce_stairs, "minecraft:spruce_stairs"},
        {minecraft::spruce_trapdoor, "minecraft:spruce_trapdoor"},
        {minecraft::spruce_wall_sign, "minecraft:spruce_wall_sign"},
        {minecraft::spruce_wood, "minecraft:spruce_wood"},
        {minecraft::sticky_piston, "minecraft:sticky_piston"},
        {minecraft::stone, "minecraft:stone"},
        {minecraft::stone_brick_slab, "minecraft:stone_brick_slab"},
        {minecraft::stone_brick_stairs, "minecraft:stone_brick_stairs"},
        {minecraft::stone_brick_wall, "minecraft:stone_brick_wall"},
        {minecraft::stone_bricks, "minecraft:stone_bricks"},
        {minecraft::stone_button, "minecraft:stone_button"},
        {minecraft::stone_pressure_plate, "minecraft:stone_pressure_plate"},
        {minecraft::stone_slab, "minecraft:stone_slab"},
        {minecraft::stone_stairs, "minecraft:stone_stairs"},
        {minecraft::stonecutter, "minecraft:stonecutter"},
        {minecraft::stripped_acacia_log, "minecraft:stripped_acacia_log"},
        {minecraft::stripped_acacia_wood, "minecraft:stripped_acacia_wood"},
        {minecraft::stripped_birch_log, "minecraft:stripped_birch_log"},
        {minecraft::stripped_birch_wood, "minecraft:stripped_birch_wood"},
        {minecraft::stripped_dark_oak_log, "minecraft:stripped_dark_oak_log"},
        {minecraft::stripped_dark_oak_wood, "minecraft:stripped_dark_oak_wood"},
        {minecraft::stripped_jungle_log, "minecraft:stripped_jungle_log"},
        {minecraft::stripped_jungle_wood, "minecraft:stripped_jungle_wood"},
        {minecraft::stripped_oak_log, "minecraft:stripped_oak_log"},
        {minecraft::stripped_oak_wood, "minecraft:stripped_oak_wood"},
        {minecraft::stripped_spruce_log, "minecraft:stripped_spruce_log"},
        {minecraft::stripped_spruce_wood, "minecraft:stripped_spruce_wood"},
        {minecraft::structure_block, "minecraft:structure_block"},
        {minecraft::structure_void, "minecraft:structure_void"},
        {minecraft::sugar_cane, "minecraft:sugar_cane"},
        {minecraft::sunflower, "minecraft:sunflower"},
        {minecraft::sweet_berry_bush, "minecraft:sweet_berry_bush"},
        {minecraft::tall_grass, "minecraft:tall_grass"},
        {minecraft::tall_seagrass, "minecraft:tall_seagrass"},
        {minecraft::terracotta, "minecraft:terracotta"},
        {minecraft::tnt, "minecraft:tnt"},
        {minecraft::torch, "minecraft:torch"},
        {minecraft::trapped_chest, "minecraft:trapped_chest"},
        {minecraft::tripwire, "minecraft:tripwire"},
        {minecraft::tripwire_hook, "minecraft:tripwire_hook"},
        {minecraft::tube_coral, "minecraft:tube_coral"},
        {minecraft::tube_coral_block, "minecraft:tube_coral_block"},
        {minecraft::tube_coral_fan, "minecraft:tube_coral_fan"},
        {minecraft::tube_coral_wall_fan, "minecraft:tube_coral_wall_fan"},
        {minecraft::turtle_egg, "minecraft:turtle_egg"},
        {minecraft::vine, "minecraft:vine"},
        {minecraft::void_air, "minecraft:void_air"},
        {minecraft::wall_torch, "minecraft:wall_torch"},
        {minecraft::water, "minecraft:water"},
        {minecraft::wet_sponge, "minecraft:wet_sponge"},
        {minecraft::wheat, "minecraft:wheat"},
        {minecraft::white_banner, "minecraft:white_banner"},
        {minecraft::white_bed, "minecraft:white_bed"},
        {minecraft::white_carpet, "minecraft:white_carpet"},
        {minecraft::white_concrete, "minecraft:white_concrete"},
        {minecraft::white_concrete_powder, "minecraft:white_concrete_powder"},
        {minecraft::white_glazed_terracotta, "minecraft:white_glazed_terracotta"},
        {minecraft::white_shulker_box, "minecraft:white_shulker_box"},
        {minecraft::white_stained_glass, "minecraft:white_stained_glass"},
        {minecraft::white_stained_glass_pane, "minecraft:white_stained_glass_pane"},
        {minecraft::white_terracotta, "minecraft:white_terracotta"},
        {minecraft::white_tulip, "minecraft:white_tulip"},
        {minecraft::white_wall_banner, "minecraft:white_wall_banner"},
        {minecraft::white_wool, "minecraft:white_wool"},
        {minecraft::wither_rose, "minecraft:wither_rose"},
        {minecraft::wither_skeleton_skull, "minecraft:wither_skeleton_skull"},
        {minecraft::wither_skeleton_wall_skull, "minecraft:wither_skeleton_wall_skull"},
        {minecraft::yellow_banner, "minecraft:yellow_banner"},
        {minecraft::yellow_bed, "minecraft:yellow_bed"},
        {minecraft::yellow_carpet, "minecraft:yellow_carpet"},
        {minecraft::yellow_concrete, "minecraft:yellow_concrete"},
        {minecraft::yellow_concrete_powder, "minecraft:yellow_concrete_powder"},
        {minecraft::yellow_glazed_terracotta, "minecraft:yellow_glazed_terracotta"},
        {minecraft::yellow_shulker_box, "minecraft:yellow_shulker_box"},
        {minecraft::yellow_stained_glass, "minecraft:yellow_stained_glass"},
        {minecraft::yellow_stained_glass_pane, "minecraft:yellow_stained_glass_pane"},
        {minecraft::yellow_terracotta, "minecraft:yellow_terracotta"},
        {minecraft::yellow_wall_banner, "minecraft:yellow_wall_banner"},
        {minecraft::yellow_wool, "minecraft:yellow_wool"},
        {minecraft::zombie_head, "minecraft:zombie_head"},
        {minecraft::zombie_wall_head, "minecraft:zombie_wall_head"},
    };
    auto mappingIt = mapping.find(id);
    if (mappingIt == mapping.end()) {
        return "";
    }
    return mappingIt->second;
}

} // namespace blocks

class Block {
public:
    explicit Block(std::string const& name, std::map<std::string, std::string> const& properties = std::map<std::string, std::string>())
        : fName(name)
        , fProperties(properties)
    {
    }

    explicit Block(blocks::BlockId id, std::map<std::string, std::string> const& properties = std::map<std::string, std::string>())
        : fName(blocks::Name(id))
        , fProperties(properties)
    {
    }

    bool equals(Block const& other) const {
        if (other.fName != fName) {
            return false;
        }
        if (other.fProperties.size() != fProperties.size()) {
            return false;
        }
        for (auto it : other.fProperties) {
            auto found = fProperties.find(it.first);
            if (found == fProperties.end()) {
                return false;
            }
            if (found->second != it.second) {
                return false;
            }
        }
        return true;
    }

public:
    std::string const fName;
    std::map<std::string, std::string> const fProperties;
};

namespace biomes {

/// @typedef BiomeId
/// @brief BiomeId
using BiomeId = uint16_t;

enum : BiomeId {
    unknown = 0,
};

namespace minecraft {

enum : BiomeId {
    ocean = 1,
    plains,
    desert,
    mountains,
    forest,
    taiga,
    swamp,
    river,
    nether,
    the_end,
    frozen_ocean,
    frozen_river,
    snowy_tundra,
    snowy_mountains,
    mushroom_fields,
    mushroom_field_shore,
    beach,
    desert_hills,
    wooded_hills,
    taiga_hills,
    mountain_edge,
    jungle,
    jungle_hills,
    jungle_edge,
    deep_ocean,
    stone_shore,
    snowy_beach,
    birch_forest,
    birch_forest_hills,
    dark_forest,
    snowy_taiga,
    snowy_taiga_hills,
    giant_tree_taiga,
    giant_tree_taiga_hills,
    wooded_mountains,
    savanna,
    savanna_plateau,
    badlands,
    wooded_badlands_plateau,
    badlands_plateau,
    small_end_islands,
    end_midlands,
    end_highlands,
    end_barrens,
    warm_ocean,
    lukewarm_ocean,
    cold_ocean,
    deep_warm_ocean,
    deep_lukewarm_ocean,
    deep_cold_ocean,
    deep_frozen_ocean,
    the_void,
    sunflower_plains,
    desert_lakes,
    gravelly_mountains,
    flower_forest,
    taiga_mountains,
    swamp_hills,
    ice_spikes,
    modified_jungle,
    modified_jungle_edge,
    tall_birch_forest,
    tall_birch_hills,
    dark_forest_hills,
    snowy_taiga_mountains,
    giant_spruce_taiga,
    giant_spruce_taiga_hills,
    modified_gravelly_mountains,
    shattered_savanna,
    shattered_savanna_plateau,
    eroded_badlands,
    modified_wooded_badlands_plateau,
    modified_badlands_plateau,
    bamboo_jungle,
    bamboo_jungle_hills,

    minecraft_max_biome_id,
};

} // namespace minecraft

static inline BiomeId FromInt(int v) {
    static std::map<int, BiomeId> const mapping = {
        {0, minecraft::ocean},
        {1, minecraft::plains},
        {2, minecraft::desert},
        {3, minecraft::mountains},
        {4, minecraft::forest},
        {5, minecraft::taiga},
        {6, minecraft::swamp},
        {7, minecraft::river},
        {8, minecraft::nether},
        {9, minecraft::the_end},
        {10, minecraft::frozen_ocean},
        {11, minecraft::frozen_river},
        {12, minecraft::snowy_tundra},
        {13, minecraft::snowy_mountains},
        {14, minecraft::mushroom_fields},
        {15, minecraft::mushroom_field_shore},
        {16, minecraft::beach},
        {17, minecraft::desert_hills},
        {18, minecraft::wooded_hills},
        {19, minecraft::taiga_hills},
        {20, minecraft::mountain_edge},
        {21, minecraft::jungle},
        {22, minecraft::jungle_hills},
        {23, minecraft::jungle_edge},
        {24, minecraft::deep_ocean},
        {25, minecraft::stone_shore},
        {26, minecraft::snowy_beach},
        {27, minecraft::birch_forest},
        {28, minecraft::birch_forest_hills},
        {29, minecraft::dark_forest},
        {30, minecraft::snowy_taiga},
        {31, minecraft::snowy_taiga_hills},
        {32, minecraft::giant_tree_taiga},
        {33, minecraft::giant_tree_taiga_hills},
        {34, minecraft::wooded_mountains},
        {35, minecraft::savanna},
        {36, minecraft::savanna_plateau},
        {37, minecraft::badlands},
        {38, minecraft::wooded_badlands_plateau},
        {39, minecraft::badlands_plateau},
        {40, minecraft::small_end_islands},
        {41, minecraft::end_midlands},
        {42, minecraft::end_highlands},
        {43, minecraft::end_barrens},
        {44, minecraft::warm_ocean},
        {45, minecraft::lukewarm_ocean},
        {46, minecraft::cold_ocean},
        {47, minecraft::deep_warm_ocean},
        {48, minecraft::deep_lukewarm_ocean},
        {49, minecraft::deep_cold_ocean},
        {50, minecraft::deep_frozen_ocean},
        {127, minecraft::the_void},
        {129, minecraft::sunflower_plains},
        {130, minecraft::desert_lakes},
        {131, minecraft::gravelly_mountains},
        {132, minecraft::flower_forest},
        {133, minecraft::taiga_mountains},
        {134, minecraft::swamp_hills},
        {140, minecraft::ice_spikes},
        {149, minecraft::modified_jungle},
        {151, minecraft::modified_jungle_edge},
        {155, minecraft::tall_birch_forest},
        {156, minecraft::tall_birch_hills},
        {157, minecraft::dark_forest_hills},
        {158, minecraft::snowy_taiga_mountains},
        {160, minecraft::giant_spruce_taiga},
        {161, minecraft::giant_spruce_taiga_hills},
        {162, minecraft::modified_gravelly_mountains},
        {163, minecraft::shattered_savanna},
        {164, minecraft::shattered_savanna_plateau},
        {165, minecraft::eroded_badlands},
        {166, minecraft::modified_wooded_badlands_plateau},
        {167, minecraft::modified_badlands_plateau},
        {168, minecraft::bamboo_jungle},
        {169, minecraft::bamboo_jungle_hills},
    };
    auto mappingIt = mapping.find(v);
    if (mappingIt == mapping.end()) {
        return unknown;
    }
    return mappingIt->second;
}

} // namespace biomes

class ChunkSection {
public:
    virtual ~ChunkSection() {}
    virtual std::shared_ptr<Block> blockAt(int offsetX, int offsetY, int offsetZ) const = 0;
    virtual uint8_t blockLightAt(int offsetX, int offsetY, int offsetZ) const = 0;
    virtual uint8_t skyLightAt(int offsetX, int offsetY, int offsetZ) const = 0;
    virtual blocks::BlockId blockIdAt(int offsetX, int offsetY, int offsetZ) const = 0;
    virtual bool blockIdWithYOffset(int offsetY, std::function<bool(int offsetX, int offsetZ, blocks::BlockId blockId)> callback) const = 0;
    virtual int y() const = 0;
};

namespace detail {

class ChunkSection_1_12 : public ChunkSection {
public:
    static std::shared_ptr<ChunkSection> MakeChunkSection(nbt::CompoundTag const* section) {
        if (!section) {
            return nullptr;
        }
        auto yTag = section->query("Y")->asByte();
        if (!yTag) {
            return nullptr;
        }

        auto blocksTag = section->query("Blocks")->asByteArray();
        if (!blocksTag) {
            return nullptr;
        }
        std::vector<uint8_t> blocks = blocksTag->value();
        blocks.resize(4096);

        auto dataTag = section->query("Data")->asByteArray();
        if (!dataTag) {
            return nullptr;
        }
        std::vector<uint8_t> data = dataTag->value();
        data.resize(2048);

        auto addTag = section->query("Add")->asByteArray();
        std::vector<uint8_t> add;
        if (addTag) {
            add = addTag->value();
        }
        add.resize(2048);

        std::vector<uint8_t> blockLight;
        auto blockLightTag = section->query("BlockLight")->asByteArray();
        if (blockLightTag) {
            blockLight = blockLightTag->value();
        }

        std::vector<uint8_t> skyLight;
        auto skyLightTag = section->query("SkyLight")->asByteArray();
        if (skyLightTag) {
            skyLight = skyLightTag->value();
        }

        std::vector<std::shared_ptr<Block>> palette;
        std::vector<uint16_t> paletteIndices(16 * 16 * 16);
        for (int y = 0; y < 16; y++) {
            for (int z = 0; z < 16; z++) {
                for (int x = 0; x < 16; x++) {
                    int const index = BlockIndex(x, y, z);
                    uint8_t const idLo = blocks[index];
                    uint8_t const idHi = Nibble4(add, index);
                    uint16_t const id = (uint16_t)idLo + ((uint16_t)idHi << 8);
                    uint8_t const blockData = Nibble4(data, index);
                    std::shared_ptr<Block> block = Flatten(id, blockData);
                    int paletteIndex = -1;
                    for (int i = 0; i < (int)palette.size(); i++) {
                        if (palette[i]->equals(*block)) {
                            paletteIndex = i;
                            break;
                        }
                    }
                    if (paletteIndex < 0) {
                        paletteIndex = (int)palette.size();
                        palette.push_back(block);
                    }
                    paletteIndices[index] = (uint16_t)paletteIndex;
                }
            }
        }

        return std::shared_ptr<ChunkSection>(new ChunkSection_1_12(yTag->fValue, palette, paletteIndices, blockLight, skyLight));
    }

    std::shared_ptr<Block> blockAt(int offsetX, int offsetY, int offsetZ) const override {
        int const index = BlockIndex(offsetX, offsetY, offsetZ);
        if (index < 0) {
            return nullptr;
        }
        int const i = fPaletteIndices[index];
        return fPalette[i];
    }

    uint8_t blockLightAt(int offsetX, int offsetY, int offsetZ) const override {
        int const index = BlockIndex(offsetX, offsetY, offsetZ);
        if (index < 0) {
            return 0;
        }
        return fBlockLight[index];
    }

    uint8_t skyLightAt(int offsetX, int offsetY, int offsetZ) const override {
        int const index = BlockIndex(offsetX, offsetY, offsetZ);
        if (index < 0) {
            return 0;
        }
        return fSkyLight[index];
    }

    blocks::BlockId blockIdAt(int offsetX, int offsetY, int offsetZ) const override {
        int const index = BlockIndex(offsetX, offsetY, offsetZ);
        if (index < 0) {
            return blocks::unknown;
        }
        int const i = fPaletteIndices[index];
        auto const& block = fPalette[i];
        return blocks::FromName(block->fName);
    }

    bool blockIdWithYOffset(int offsetY, std::function<bool(int offsetX, int offsetZ, blocks::BlockId blockId)> callback) const override {
        for (int x = 0; x < 16; x++) {
            for (int z = 0; z < 16; z++) {
                auto id = blockIdAt(x, offsetY, z);
                if (callback(x, z, id)) {
                    return false;
                }
            }
        }
        return true;
    }

    int y() const override {
        return fY;
    }

private:
    ChunkSection_1_12(int y, std::vector<std::shared_ptr<Block>> const& palette, std::vector<uint16_t> const& paletteIndices, std::vector<uint8_t> const& blockLight, std::vector<uint8_t> const& skyLight)
        : fY(y)
        , fPalette(palette)
        , fPaletteIndices(paletteIndices)
        , fBlockLight(blockLight)
        , fSkyLight(skyLight)
    {
    }

    static inline int BlockIndex(int offsetX, int offsetY, int offsetZ) {
        if (offsetX < 0 || 16 <= offsetX || offsetY < 0 || 16 <= offsetY || offsetZ < 0 || 16 <= offsetZ) {
            return -1;
        }
        return offsetY * 16 * 16 + offsetZ * 16 + offsetX;
    }

    static inline uint8_t Nibble4(std::vector<uint8_t> const& arr, int index) {
        return index % 2 == 0 ? arr[index / 2] & 0x0F : (arr[index / 2] >> 4) & 0x0F;
    }

    static inline std::shared_ptr<Block> Flatten(uint16_t blockId, uint8_t data) {
        auto id = blocks::minecraft::air;
        std::map<std::string, std::string> properties;
        switch (blockId) {
            case 0: id = blocks::minecraft::air; break;
            case 1:
                switch (data) {
                    case 1: id = blocks::minecraft::granite; break;
                    case 2: id = blocks::minecraft::polished_granite; break;
                    case 3: id = blocks::minecraft::diorite; break;
                    case 4: id = blocks::minecraft::polished_diorite; break;
                    case 5: id = blocks::minecraft::andesite; break;
                    case 6: id = blocks::minecraft::polished_andesite; break;
                    case 0:
                    default:
                        id = blocks::minecraft::stone;
                        break;
                }
                break;
            case 2: id = blocks::minecraft::grass_block; break;
            case 3:
                switch (data) {
                    case 1: id = blocks::minecraft::coarse_dirt; break;
                    case 2: id = blocks::minecraft::podzol; break;
                    case 0:
                    default:
                        id = blocks::minecraft::dirt;
                        break;
                }
                break;
            case 4: id = blocks::minecraft::cobblestone; break;
            case 5:
                switch (data) {
                    case 1: id = blocks::minecraft::spruce_planks; break;
                    case 2: id = blocks::minecraft::birch_planks; break;
                    case 3: id = blocks::minecraft::jungle_planks; break;
                    case 4: id = blocks::minecraft::acacia_planks; break;
                    case 5: id = blocks::minecraft::dark_oak_planks; break;
                    case 0:
                    default:
                        id = blocks::minecraft::oak_planks;
                        break;
                }
                break;
            case 6:
                switch (data & 0x7) {
                    case 1: id = blocks::minecraft::spruce_sapling; break;
                    case 2: id = blocks::minecraft::birch_sapling; break;
                    case 3: id = blocks::minecraft::jungle_sapling; break;
                    case 4: id = blocks::minecraft::acacia_sapling; break;
                    case 5: id = blocks::minecraft::dark_oak_sapling; break;
                    case 0:
                    default:
                        id = blocks::minecraft::oak_sapling;
                        break;
                }
                break;
            case 7: id = blocks::minecraft::bedrock; break;
            case 8: id = blocks::minecraft::water; break; //TODO: flowing_water
            case 9: id = blocks::minecraft::water; break;
            case 10: id = blocks::minecraft::lava; break; //TODO: flowing_lava
            case 11: id = blocks::minecraft::lava; break;
            case 12:
                switch (data) {
                    case 1: id = blocks::minecraft::red_sand; break;
                    case 0:
                    default:
                        id = blocks::minecraft::sand;
                }
                break;
            case 13: id = blocks::minecraft::gravel; break;
            case 14: id = blocks::minecraft::gold_ore; break;
            case 15: id = blocks::minecraft::iron_ore; break;
            case 16: id = blocks::minecraft::coal_ore; break;
            case 17:
                switch (data & 0x3) {
                    case 1: id = blocks::minecraft::spruce_log; break;
                    case 2: id = blocks::minecraft::birch_log; break;
                    case 3: id = blocks::minecraft::jungle_log; break;
                    case 0:
                    default:
                        id = blocks::minecraft::oak_log;
                        break;
                }
                break;
            case 18:
                switch (data) {
                    case 1:
                    case 5:
                    case 9:
                    case 13:
                        id = blocks::minecraft::spruce_leaves;
                        break;
                    case 2:
                    case 6:
                    case 10:
                    case 14:
                        id = blocks::minecraft::birch_leaves;
                        break;
                    case 3:
                    case 7:
                    case 11:
                    case 15:
                        id = blocks::minecraft::jungle_leaves;
                        break;
                    case 0:
                    case 4:
                    case 8:
                    case 12:
                    default:
                        id = blocks::minecraft::oak_leaves;
                        break;
                }
                break;
            case 19:
                switch (data) {
                    case 1: id = blocks::minecraft::wet_sponge; break;
                    case 0:
                    default:
                        id = blocks::minecraft::sponge;
                        break;
                }
                break;
            case 20: id = blocks::minecraft::glass; break;
            case 21: id = blocks::minecraft::lapis_ore; break;
            case 22: id = blocks::minecraft::lapis_block; break;
            case 23: id = blocks::minecraft::dispenser; break;
            case 24:
                switch (data) {
                    case 1: id = blocks::minecraft::chiseled_sandstone; break;
                    case 2: id = blocks::minecraft::smooth_sandstone; break;
                    case 0:
                    default:
                        id = blocks::minecraft::sandstone;
                        break;
                }
                break;
            case 25: id = blocks::minecraft::note_block; break;
            case 26: id = blocks::minecraft::white_bed; break;
            case 27: id = blocks::minecraft::powered_rail; break;
            case 28: id = blocks::minecraft::detector_rail; break;
            case 29: id = blocks::minecraft::sticky_piston; break;
            case 30: id = blocks::minecraft::cobweb; break;
            case 31: id = blocks::minecraft::tall_grass; break;
            case 32: id = blocks::minecraft::dead_bush; break;
            case 33: id = blocks::minecraft::piston; break;
            case 34: id = blocks::minecraft::piston_head; break;
            case 35:
                switch (data) {
                    case 1: id = blocks::minecraft::orange_wool; break;
                    case 2: id = blocks::minecraft::magenta_wool; break;
                    case 3: id = blocks::minecraft::light_blue_wool; break;
                    case 4: id = blocks::minecraft::yellow_wool; break;
                    case 5: id = blocks::minecraft::lime_wool; break;
                    case 6: id = blocks::minecraft::pink_wool; break;
                    case 7: id = blocks::minecraft::gray_wool; break;
                    case 8: id = blocks::minecraft::light_gray_wool; break;
                    case 9: id = blocks::minecraft::cyan_wool; break;
                    case 10: id = blocks::minecraft::purple_wool; break;
                    case 11: id = blocks::minecraft::blue_wool; break;
                    case 12: id = blocks::minecraft::brown_wool; break;
                    case 13: id = blocks::minecraft::green_wool; break;
                    case 14: id = blocks::minecraft::red_wool; break;
                    case 15: id = blocks::minecraft::black_wool; break;
                    case 0:
                    default:
                        id = blocks::minecraft::white_wool;
                        break;
                }
                break;
            case 37: id = blocks::minecraft::dandelion; break;
            case 38: id = blocks::minecraft::poppy; break;
            case 39: id = blocks::minecraft::brown_mushroom; break;
            case 40: id = blocks::minecraft::red_mushroom; break;
            case 41: id = blocks::minecraft::gold_block; break;
            case 42: id = blocks::minecraft::iron_block; break;
            case 43:
                switch (data) {
                    case 1: id = blocks::minecraft::sandstone_slab; break;
                    case 2: id = blocks::minecraft::oak_slab; break;
                    case 3: id = blocks::minecraft::cobblestone_slab; break;
                    case 4: id = blocks::minecraft::brick_slab; break;
                    case 5: id = blocks::minecraft::stone_brick_slab; break;
                    case 6: id = blocks::minecraft::quartz_slab; break;
                    case 7: id = blocks::minecraft::nether_brick_slab; break;
                    case 0:
                    default:
                        id = blocks::minecraft::stone_slab;
                        break;
                }
                properties["type"] = "double";
                break;
            case 44:
                switch (data) {
                    case 1:
                    case 9:
                        id = blocks::minecraft::sandstone_slab;
                        break;
                    case 2:
                    case 10:
                        id = blocks::minecraft::oak_slab;
                        break;
                    case 3:
                    case 11:
                        id = blocks::minecraft::cobblestone_slab;
                        break;
                    case 4:
                    case 12:
                        id = blocks::minecraft::brick_slab;
                        break;
                    case 5:
                    case 13:
                        id = blocks::minecraft::stone_brick_slab;
                        break;
                    case 6:
                    case 14:
                        id = blocks::minecraft::nether_brick_slab;
                        break;
                    case 7:
                    case 15:
                        id = blocks::minecraft::quartz_slab;
                        break;
                    case 0:
                    case 8:
                    default:
                        id = id = blocks::minecraft::stone_slab;
                        break;
                }
                switch (data) {
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                        properties["type"] = "top";
                        break;
                    default:
                        properties["type"] = "bottom";
                        break;
                }
                break;
            case 45: id = blocks::minecraft::bricks; break;
            case 46: id = blocks::minecraft::tnt; break;
            case 47: id = blocks::minecraft::bookshelf; break;
            case 48: id = blocks::minecraft::mossy_cobblestone; break;
            case 49: id = blocks::minecraft::obsidian; break;
            case 50:
                switch (data) {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                        id = blocks::minecraft::wall_torch;
                        break;
                    case 0:
                    case 5:
                    default:
                        id = blocks::minecraft::torch;
                        break;
                }
                break;
            case 51: id = blocks::minecraft::fire; break;
            case 52: id = blocks::minecraft::spawner; break;
            case 53: id = blocks::minecraft::oak_stairs; break;
            case 54: id = blocks::minecraft::chest; break;
            case 55: id = blocks::minecraft::redstone_wire; break;
            case 56: id = blocks::minecraft::diamond_ore; break;
            case 57: id = blocks::minecraft::diamond_block; break;
            case 58: id = blocks::minecraft::crafting_table; break;
            case 59: id = blocks::minecraft::wheat; break;
            case 60: id = blocks::minecraft::farmland; break;
            case 61:
            case 62: // lit
                id = blocks::minecraft::furnace;
                break;
            case 63: id = blocks::minecraft::oak_sign; break;
            case 64: id = blocks::minecraft::oak_door; break;
            case 65: id = blocks::minecraft::ladder; break;
            case 66: id = blocks::minecraft::rail; break;
            case 67: id = blocks::minecraft::cobblestone_stairs; break;
            case 68: id = blocks::minecraft::oak_wall_sign; break;
            case 69: id = blocks::minecraft::lever; break;
            case 70: id = blocks::minecraft::stone_pressure_plate; break;
            case 71: id = blocks::minecraft::iron_door; break;
            case 72: id = blocks::minecraft::oak_pressure_plate; break;
            case 73: id = blocks::minecraft::redstone_ore; break;
            case 74: id = blocks::minecraft::redstone_ore; break; // glowing_redstone_ore
            case 75: //unlit_redstone_torch
                if (data == 5) {
                    id = blocks::minecraft::redstone_torch;
                } else {
                    id = blocks::minecraft::redstone_wall_torch;
                }
                break;
            case 76: //redstone_torch
                if (data == 5) {
                    id = blocks::minecraft::redstone_torch;
                } else {
                    id = blocks::minecraft::redstone_wall_torch;
                }
                break;
            case 77: id = blocks::minecraft::stone_button; break;
            case 78: id = blocks::minecraft::snow; break;
            case 79: id = blocks::minecraft::ice; break;
            case 80: id = blocks::minecraft::snow_block; break;
            case 81: id = blocks::minecraft::cactus; break;
            case 82: id = blocks::minecraft::clay; break;
            case 83: id = blocks::minecraft::sugar_cane; break;
            case 84: id = blocks::minecraft::jukebox; break;
            case 85: id = blocks::minecraft::oak_fence; break;
            case 86: id = blocks::minecraft::pumpkin; break;
            case 87: id = blocks::minecraft::netherrack; break;
            case 88: id = blocks::minecraft::soul_sand; break;
            case 89: id = blocks::minecraft::glowstone; break;
            case 90: id = blocks::minecraft::nether_portal; break;
            case 91: id = blocks::minecraft::jack_o_lantern; break;
            case 92: id = blocks::minecraft::cake; break;
            case 93: id = blocks::minecraft::repeater; break;
            case 94: id = blocks::minecraft::repeater; break; // powered_repeater
            case 95:
                switch (data) {
                    case 1: id = blocks::minecraft::orange_stained_glass; break;
                    case 2: id = blocks::minecraft::magenta_stained_glass; break;
                    case 3: id = blocks::minecraft::light_blue_stained_glass; break;
                    case 4: id = blocks::minecraft::yellow_stained_glass; break;
                    case 5: id = blocks::minecraft::lime_stained_glass; break;
                    case 6: id = blocks::minecraft::pink_stained_glass; break;
                    case 7: id = blocks::minecraft::gray_stained_glass; break;
                    case 8: id = blocks::minecraft::light_gray_stained_glass; break;
                    case 9: id = blocks::minecraft::cyan_stained_glass; break;
                    case 10: id = blocks::minecraft::purple_stained_glass; break;
                    case 11: id = blocks::minecraft::blue_stained_glass; break;
                    case 12: id = blocks::minecraft::brown_stained_glass; break;
                    case 13: id = blocks::minecraft::green_stained_glass; break;
                    case 14: id = blocks::minecraft::red_stained_glass; break;
                    case 15: id = blocks::minecraft::black_stained_glass; break;
                    case 0:
                    default:
                        id = blocks::minecraft::white_stained_glass;
                        break;
                }
                break;
            case 96: id = blocks::minecraft::oak_trapdoor; break;
            case 97:
                switch (data) {
                    case 1: id = blocks::minecraft::infested_cobblestone; break;
                    case 2: id = blocks::minecraft::infested_stone_bricks; break;
                    case 3: id = blocks::minecraft::infested_mossy_stone_bricks; break;
                    case 4: id = blocks::minecraft::infested_cracked_stone_bricks; break;
                    case 5: id = blocks::minecraft::infested_chiseled_stone_bricks; break;
                    case 0:
                    default:
                        id = blocks::minecraft::infested_stone;
                        break;
                }
                break;
            case 98:
                switch (data) {
                    case 1: id = blocks::minecraft::mossy_stone_bricks; break;
                    case 2: id = blocks::minecraft::cracked_stone_bricks; break;
                    case 3: id = blocks::minecraft::chiseled_stone_bricks; break;
                    case 0:
                    default:
                        id = blocks::minecraft::stone_bricks;
                        break;
                }
                break;
            case 99: id = blocks::minecraft::brown_mushroom_block; break;
            case 100: id = blocks::minecraft::red_mushroom_block; break;
            case 101: id = blocks::minecraft::iron_bars; break;
            case 102: id = blocks::minecraft::glass_pane; break;
            case 103: id = blocks::minecraft::melon; break;
            case 104: id = blocks::minecraft::pumpkin_stem; break;
            case 105: id = blocks::minecraft::melon_stem; break;
            case 106: id = blocks::minecraft::vine; break;
            case 107: id = blocks::minecraft::oak_fence_gate; break;
            case 108: id = blocks::minecraft::brick_stairs; break;
            case 109: id = blocks::minecraft::stone_brick_stairs; break;
            case 110: id = blocks::minecraft::mycelium; break;
            case 111: id = blocks::minecraft::lily_pad; break;
            case 112: id = blocks::minecraft::nether_bricks; break;
            case 113: id = blocks::minecraft::nether_brick_fence; break;
            case 114: id = blocks::minecraft::nether_brick_stairs; break;
            case 115: id = blocks::minecraft::nether_wart; break;
            case 116: id = blocks::minecraft::enchanting_table; break;
            case 117: id = blocks::minecraft::brewing_stand; break;
            case 118: id = blocks::minecraft::cauldron; break;
            case 119: id = blocks::minecraft::end_portal; break;
            case 120: id = blocks::minecraft::end_portal_frame; break;
            case 121: id = blocks::minecraft::end_stone; break;
            case 122: id = blocks::minecraft::dragon_egg; break;
            case 123: id = blocks::minecraft::redstone_lamp; break;
            case 124: id = blocks::minecraft::redstone_lamp; break; // lit_redstone_lamp
            case 125:
                id = blocks::minecraft::oak_slab;
                properties["type"] = "double";
                break;
            case 126:
                id = blocks::minecraft::oak_slab;
                properties["type"] = "bottom";
                break;
            case 127: id = blocks::minecraft::cocoa; break;
            case 128: id = blocks::minecraft::sandstone_stairs; break;
            case 129: id = blocks::minecraft::emerald_ore; break;
            case 130: id = blocks::minecraft::ender_chest; break;
            case 131: id = blocks::minecraft::tripwire_hook; break;
            case 132: id = blocks::minecraft::tripwire; break;
            case 133: id = blocks::minecraft::emerald_block; break;
            case 134: id = blocks::minecraft::spruce_stairs; break;
            case 135: id = blocks::minecraft::birch_stairs; break;
            case 136: id = blocks::minecraft::jungle_stairs; break;
            case 137: id = blocks::minecraft::command_block; break;
            case 138: id = blocks::minecraft::beacon; break;
            case 139: id = blocks::minecraft::cobblestone_wall; break;
            case 140: id = blocks::minecraft::flower_pot; break;
            case 141: id = blocks::minecraft::carrots; break;
            case 142: id = blocks::minecraft::potatoes; break;
            case 143: id = blocks::minecraft::oak_button; break;
            case 144: id = blocks::minecraft::skeleton_skull; break;
            case 145: id = blocks::minecraft::anvil; break;
            case 146: id = blocks::minecraft::trapped_chest; break;
            case 147: id = blocks::minecraft::light_weighted_pressure_plate; break;
            case 148: id = blocks::minecraft::heavy_weighted_pressure_plate; break;
            case 149: id = blocks::minecraft::comparator; break; //unpowered_comparator
            case 150: id = blocks::minecraft::comparator; break;
            case 151: id = blocks::minecraft::daylight_detector; break;
            case 152: id = blocks::minecraft::redstone_block; break;
            case 153: id = blocks::minecraft::nether_quartz_ore; break;
            case 154: id = blocks::minecraft::hopper; break;
            case 155: id = blocks::minecraft::quartz_block; break;
            case 156: id = blocks::minecraft::quartz_stairs; break;
            case 157: id = blocks::minecraft::activator_rail; break;
            case 158: id = blocks::minecraft::dropper; break;
            case 159: id = blocks::minecraft::white_glazed_terracotta; break; //??
            case 160:
                switch (data) {
                    case 1: id = blocks::minecraft::orange_stained_glass_pane; break;
                    case 2: id = blocks::minecraft::magenta_stained_glass_pane; break;
                    case 3: id = blocks::minecraft::light_blue_stained_glass_pane; break;
                    case 4: id = blocks::minecraft::yellow_stained_glass_pane; break;
                    case 5: id = blocks::minecraft::lime_stained_glass_pane; break;
                    case 6: id = blocks::minecraft::pink_stained_glass_pane; break;
                    case 7: id = blocks::minecraft::gray_stained_glass_pane; break;
                    case 8: id = blocks::minecraft::light_gray_stained_glass_pane; break;
                    case 9: id = blocks::minecraft::cyan_stained_glass_pane; break;
                    case 10: id = blocks::minecraft::purple_stained_glass_pane; break;
                    case 11: id = blocks::minecraft::blue_stained_glass_pane; break;
                    case 12: id = blocks::minecraft::brown_stained_glass_pane; break;
                    case 13: id = blocks::minecraft::green_stained_glass_pane; break;
                    case 14: id = blocks::minecraft::red_stained_glass_pane; break;
                    case 15: id = blocks::minecraft::black_stained_glass_pane; break;
                    case 0:
                    default:
                        id = blocks::minecraft::white_stained_glass_pane;
                        break;
                }
                break;
            case 161:
                switch (data) {
                    case 1:
                    case 5:
                    case 9:
                    case 13:
                        id = blocks::minecraft::dark_oak_leaves;
                        break;
                    case 0:
                    case 4:
                    case 8:
                    case 12:
                    default:
                        id = blocks::minecraft::acacia_leaves;
                        break;
                }
                break;
            case 162:
                switch (data & 0x3) {
                    case 1: id = blocks::minecraft::dark_oak_log; break;
                    case 0:
                    default:
                        id = blocks::minecraft::acacia_log;
                        break;
                }
                break;
            case 163: id = blocks::minecraft::acacia_stairs; break;
            case 164: id = blocks::minecraft::dark_oak_stairs; break;
            case 165: id = blocks::minecraft::slime_block; break;
            case 166: id = blocks::minecraft::barrier; break;
            case 167: id = blocks::minecraft::iron_trapdoor; break;
            case 168: id = blocks::minecraft::prismarine; break;
            case 169: id = blocks::minecraft::sea_lantern; break;
            case 170: id = blocks::minecraft::hay_block; break;
            case 171:
                switch (data) {
                    case 1: id = blocks::minecraft::orange_carpet; break;
                    case 2: id = blocks::minecraft::magenta_carpet; break;
                    case 3: id = blocks::minecraft::light_blue_carpet; break;
                    case 4: id = blocks::minecraft::yellow_carpet; break;
                    case 5: id = blocks::minecraft::lime_carpet; break;
                    case 6: id = blocks::minecraft::pink_carpet; break;
                    case 7: id = blocks::minecraft::gray_carpet; break;
                    case 8: id = blocks::minecraft::light_gray_carpet; break;
                    case 9: id = blocks::minecraft::cyan_carpet; break;
                    case 10: id = blocks::minecraft::purple_carpet; break;
                    case 11: id = blocks::minecraft::blue_carpet; break;
                    case 12: id = blocks::minecraft::brown_carpet; break;
                    case 13: id = blocks::minecraft::green_carpet; break;
                    case 14: id = blocks::minecraft::red_carpet; break;
                    case 15: id = blocks::minecraft::black_carpet; break;
                    case 0:
                    default:
                        id = blocks::minecraft::white_carpet;
                        break;
                }
                break;
            case 172:
                switch (data) {
                    case 1: id = blocks::minecraft::orange_terracotta; break;
                    case 2: id = blocks::minecraft::magenta_terracotta; break;
                    case 3: id = blocks::minecraft::light_blue_terracotta; break;
                    case 4: id = blocks::minecraft::yellow_terracotta; break;
                    case 5: id = blocks::minecraft::lime_terracotta; break;
                    case 6: id = blocks::minecraft::pink_terracotta; break;
                    case 7: id = blocks::minecraft::gray_terracotta; break;
                    case 8: id = blocks::minecraft::light_gray_terracotta; break;
                    case 9: id = blocks::minecraft::cyan_terracotta; break;
                    case 10: id = blocks::minecraft::purple_terracotta; break;
                    case 11: id = blocks::minecraft::blue_terracotta; break;
                    case 12: id = blocks::minecraft::brown_terracotta; break;
                    case 13: id = blocks::minecraft::green_terracotta; break;
                    case 14: id = blocks::minecraft::red_terracotta; break;
                    case 15: id = blocks::minecraft::black_terracotta; break;
                    case 0:
                    default:
                        id = blocks::minecraft::white_terracotta;
                        break;
                }
                break;
            case 173: id = blocks::minecraft::coal_block; break;
            case 174: id = blocks::minecraft::packed_ice; break;
            case 175:
                switch (data) {
                    case 1: id = blocks::minecraft::lilac; break;
                    case 2: id = blocks::minecraft::tall_grass; break;
                    case 3: id = blocks::minecraft::large_fern; break;
                    case 4: id = blocks::minecraft::rose_bush; break;
                    case 5: id = blocks::minecraft::peony; break;
                    case 0:
                    default:
                        id = blocks::minecraft::sunflower;
                        break;
                }
                break;
            case 176: id = blocks::minecraft::white_banner; break;
            case 177: id = blocks::minecraft::white_wall_banner; break;
            case 178: id = blocks::minecraft::daylight_detector; break; // inverted_daylight_detector
            case 179: id = blocks::minecraft::red_sandstone; break;
            case 180: id = blocks::minecraft::red_sandstone_stairs; break;
            case 181:
                id = blocks::minecraft::red_sandstone_slab;
                properties["type"] = "double";
                break;
            case 182:
                switch (data) {
                    case 8:
                        id = blocks::minecraft::red_sandstone_slab;
                        properties["type"] = "top";
                        break;
                    case 0:
                    default:
                        id = blocks::minecraft::red_sandstone_slab;
                        properties["type"] = "bottom";
                        break;
                }
                break;
            case 183: id = blocks::minecraft::spruce_fence_gate; break;
            case 184: id = blocks::minecraft::birch_fence_gate; break;
            case 185: id = blocks::minecraft::jungle_fence_gate; break;
            case 186: id = blocks::minecraft::dark_oak_fence_gate; break;
            case 187: id = blocks::minecraft::acacia_fence_gate; break;
            case 188: id = blocks::minecraft::spruce_fence; break;
            case 189: id = blocks::minecraft::birch_fence; break;
            case 190: id = blocks::minecraft::jungle_fence; break;
            case 191: id = blocks::minecraft::dark_oak_fence; break;
            case 192: id = blocks::minecraft::acacia_fence; break;
            case 193: id = blocks::minecraft::spruce_door; break;
            case 194: id = blocks::minecraft::birch_door; break;
            case 195: id = blocks::minecraft::jungle_door; break;
            case 196: id = blocks::minecraft::acacia_door; break;
            case 197: id = blocks::minecraft::dark_oak_door; break;
            case 198: id = blocks::minecraft::end_rod; break;
            case 199: id = blocks::minecraft::chorus_plant; break;
            case 200: id = blocks::minecraft::chorus_flower; break;
            case 201: id = blocks::minecraft::purpur_block; break;
            case 202: id = blocks::minecraft::purpur_pillar; break;
            case 203: id = blocks::minecraft::purpur_stairs; break;
            case 204:
                id = blocks::minecraft::purpur_slab;
                properties["type"] = "double";
                break;
            case 205:
                id = blocks::minecraft::purpur_slab;
                if (data == 8) {
                    properties["type"] = "top";
                } else {
                    properties["type"] = "bottom";
                }
                break;
            case 206: id = blocks::minecraft::end_stone_bricks; break;
            case 207: id = blocks::minecraft::beetroots; break;
            case 208: id = blocks::minecraft::grass_path; break;
            case 209: id = blocks::minecraft::end_gateway; break;
            case 210: id = blocks::minecraft::repeating_command_block; break;
            case 211: id = blocks::minecraft::chain_command_block; break;
            case 212: id = blocks::minecraft::frosted_ice; break;
            case 213: id = blocks::minecraft::magma_block; break;
            case 214: id = blocks::minecraft::nether_wart_block; break;
            case 215: id = blocks::minecraft::red_nether_bricks; break;
            case 216: id = blocks::minecraft::bone_block; break;
            case 217: id = blocks::minecraft::structure_void; break;
            case 218: id = blocks::minecraft::observer; break;
            case 219: id = blocks::minecraft::white_shulker_box; break;
            case 220: id = blocks::minecraft::orange_shulker_box; break;
            case 221: id = blocks::minecraft::magenta_shulker_box; break;
            case 222: id = blocks::minecraft::light_blue_shulker_box; break;
            case 223: id = blocks::minecraft::yellow_shulker_box; break;
            case 224: id = blocks::minecraft::lime_shulker_box; break;
            case 225: id = blocks::minecraft::pink_shulker_box; break;
            case 226: id = blocks::minecraft::gray_shulker_box; break;
            case 227: id = blocks::minecraft::light_gray_shulker_box; break;
            case 228: id = blocks::minecraft::cyan_shulker_box; break;
            case 229: id = blocks::minecraft::purple_shulker_box; break;
            case 230: id = blocks::minecraft::blue_shulker_box; break;
            case 231: id = blocks::minecraft::brown_shulker_box; break;
            case 232: id = blocks::minecraft::green_shulker_box; break;
            case 233: id = blocks::minecraft::red_shulker_box; break;
            case 234: id = blocks::minecraft::black_shulker_box; break;
            case 235: id = blocks::minecraft::white_glazed_terracotta; break;
            case 236: id = blocks::minecraft::orange_glazed_terracotta; break;
            case 237: id = blocks::minecraft::magenta_glazed_terracotta; break;
            case 238: id = blocks::minecraft::light_blue_glazed_terracotta; break;
            case 239: id = blocks::minecraft::yellow_glazed_terracotta; break;
            case 240: id = blocks::minecraft::lime_glazed_terracotta; break;
            case 241: id = blocks::minecraft::pink_glazed_terracotta; break;
            case 242: id = blocks::minecraft::gray_glazed_terracotta; break;
            case 243: id = blocks::minecraft::light_gray_glazed_terracotta; break;
            case 244: id = blocks::minecraft::cyan_glazed_terracotta; break;
            case 245: id = blocks::minecraft::purple_glazed_terracotta; break;
            case 246: id = blocks::minecraft::blue_glazed_terracotta; break;
            case 247: id = blocks::minecraft::brown_glazed_terracotta; break;
            case 248: id = blocks::minecraft::green_glazed_terracotta; break;
            case 249: id = blocks::minecraft::red_glazed_terracotta; break;
            case 250: id = blocks::minecraft::black_glazed_terracotta; break;
            case 251: id = blocks::minecraft::white_concrete; break;
            case 252: id = blocks::minecraft::white_concrete_powder; break;
            case 255: id = blocks::minecraft::structure_block; break;
            default: id = blocks::minecraft::air; break;
        }
        return std::make_shared<Block>(id, properties);
    }

private:
    int const fY;
    std::vector<std::shared_ptr<Block>> fPalette;
    std::vector<uint16_t> fPaletteIndices;
    std::vector<uint8_t> fBlockLight;
    std::vector<uint8_t> fSkyLight;
};

class ChunkSection_1_13 : public ChunkSection {
public:
    std::shared_ptr<Block> blockAt(int offsetX, int offsetY, int offsetZ) const override {
        int const index = paletteIndex(offsetX, offsetY, offsetZ);
        if (index < 0) {
            return nullptr;
        }
        return fPalette[index];
    }

    uint8_t blockLightAt(int offsetX, int offsetY, int offsetZ) const override {
        int const index = BlockIndex(offsetX, offsetY, offsetZ);
        if (index < 0) {
            return 0;
        }
        int const bitIndex = index * 4;
        int const byteIndex = bitIndex / 8;
        if (fBlockLight.size() <= byteIndex) {
            return 0;
        }
        int const bitOffset = bitIndex - 8 * byteIndex;
        uint8_t const v = fBlockLight[byteIndex];
        return (v >> bitOffset) & 0xF;
    }

    uint8_t skyLightAt(int offsetX, int offsetY, int offsetZ) const override {
        int const index = BlockIndex(offsetX, offsetY, offsetZ);
        if (index < 0) {
            return 0;
        }
        int const bitIndex = index * 4;
        int const byteIndex = bitIndex / 8;
        if (fSkyLight.size() <= byteIndex) {
            return 0;
        }
        int const bitOffset = bitIndex - 8 * byteIndex;
        uint8_t const v = fSkyLight[byteIndex];
        return (v >> bitOffset) & 0xF;
    }

    blocks::BlockId blockIdAt(int offsetX, int offsetY, int offsetZ) const override {
        int const index = paletteIndex(offsetX, offsetY, offsetZ);
        if (index < 0) {
            return blocks::unknown;
        }
        return fBlockIdPalette[index];
    }

    bool blockIdWithYOffset(int offsetY, std::function<bool(int offsetX, int offsetZ, blocks::BlockId blockId)> callback) const override {
        if (offsetY < 0 || 16 <= offsetY) {
            return false;
        }
        return eachPaletteIndexWithY(offsetY, [this, callback](int offsetX, int offsetZ, int index) {
            if (index < 0 || fBlockIdPalette.size() <= index) {
                if (!callback(offsetX, offsetZ, blocks::unknown)) {
                    return false;
                }
            }
            return callback(offsetX, offsetZ, fBlockIdPalette[index]);
        });
    }

    int y() const override {
        return fY;
    }

    static std::shared_ptr<ChunkSection> MakeChunkSection(nbt::CompoundTag const* section) {
        if (!section) {
            return nullptr;
        }
        auto yTag = section->query("Y")->asByte();
        if (!yTag) {
            return nullptr;
        }

        auto paletteTag = section->query("Palette")->asList();
        if (!paletteTag) {
            return nullptr;
        }
        std::vector<std::shared_ptr<Block>> palette;
        for (auto entry : paletteTag->fValue) {
            auto tag = entry->asCompound();
            if (!tag) {
                return nullptr;
            }
            auto nameTag = tag->query("Name")->asString();
            if (!nameTag) {
                return nullptr;
            }
            std::map<std::string, std::string> properties;
            auto propertiesTag = tag->query("Properties")->asCompound();
            if (propertiesTag) {
                for (auto p : propertiesTag->fValue) {
                    std::string n = p.first;
                    if (n.empty()) {
                        continue;
                    }
                    auto v = p.second->asString();
                    if (!v) {
                        continue;
                    }
                    properties.insert(std::make_pair(n, v->fValue));
                }
            }
            palette.push_back(std::make_shared<Block>(nameTag->fValue, properties));
        }

        std::vector<blocks::BlockId> blockIdPalette(palette.size());
        for (int i = 0; i < palette.size(); i++) {
            auto block = palette[i];
            blockIdPalette[i] = blocks::FromName(block->fName);
        }

        auto blockStatesTag = section->query("BlockStates")->asLongArray();
        if (!blockStatesTag) {
            return nullptr;
        }

        std::vector<uint8_t> blockLight;
        auto blockLightTag = section->query("BlockLight")->asByteArray();
        if (blockLightTag) {
            blockLight = blockLightTag->value();
        }

        std::vector<uint8_t> skyLight;
        auto skyLightTag = section->query("SkyLight")->asByteArray();
        if (skyLightTag) {
            skyLight = skyLightTag->value();
        }

        return std::shared_ptr<ChunkSection>(new ChunkSection_1_13((int)yTag->fValue,
                                                                   palette,
                                                                   blockIdPalette,
                                                                   blockStatesTag->value(),
                                                                   blockLight,
                                                                   skyLight));
    }

private:
    ChunkSection_1_13(int y, std::vector<std::shared_ptr<Block>> const& palette, std::vector<blocks::BlockId> const& blockIdPalette, std::vector<int64_t> const& blockStates, std::vector<uint8_t> const& blockLight, std::vector<uint8_t> const& skyLight)
        : fY(y)
        , fPalette(palette)
        , fBlockIdPalette(blockIdPalette)
        , fBlockStates(blockStates)
        , fBlockLight(blockLight)
        , fSkyLight(skyLight)
    {
    }

    int paletteIndex(int offsetX, int offsetY, int offsetZ) const {
        if (offsetX < 0 || 16 <= offsetX || offsetY < 0 || 16 <= offsetY || offsetZ < 0 || 16 <= offsetZ) {
            return -1;
        }
        size_t const numBits = fBlockStates.size() * 64;
        if (numBits % 4096 != 0) {
            return -1;
        }
        size_t const bitsPerIndex = numBits >> 12;
        int64_t const mask = std::numeric_limits<uint64_t>::max() >> (64 - bitsPerIndex);

        size_t const index = (size_t)offsetY * 16 * 16 + (size_t)offsetZ * 16 + (size_t)offsetX;
        size_t const bitIndex = index * bitsPerIndex;
        size_t const uint64Index = bitIndex >> 6;

        if (fBlockStates.size() <= uint64Index) {
            return -1;
        }
        uint64_t const v0 = *(uint64_t *)(fBlockStates.data() + uint64Index);
        uint64_t const v1 = (uint64Index + 1) < fBlockStates.size() ? *(uint64_t *)(fBlockStates.data() + uint64Index + 1) : 0;

        int const v0Offset = (int)(bitIndex - uint64Index * 64);
        int const v0Bits = (int)std::min((uint64Index + 1) * 64 - bitIndex, bitsPerIndex);

        uint64_t const paletteIndex = ((v0 >> v0Offset) & mask) | ((v1 << v0Bits) & mask);

        /*
                   uint64Index                         uint64Index + 1
        |-----------------------------------||-----------------------------------|
        |63                                0||63                                0|
        |-------------------^====^----------||-----------------------------------|
                                 |   <--   0
                                 v0Offset


                   uint64Index                         uint64Index + 1
        |-----------------------------------||-----------------------------------|
        |63                                0||63                                0|
        |===^-------------------------------||---------------------------------^=|
            |    <--        <--       <--   0                                    0
            v0Offset
        */

        if (fPalette.size() <= paletteIndex) {
            return -1;
        }

        return (int)paletteIndex;
    }

    bool eachPaletteIndexWithY(int offsetY, std::function<bool(int offsetX, int offsetZ, int paletteIndex)> callback) const {
        size_t const numBits = fBlockStates.size() * 64;
        if (numBits % 4096 != 0) {
            return false;
        }
        size_t const bitsPerIndex = numBits >> 12;
        int64_t const mask = std::numeric_limits<uint64_t>::max() >> (64 - bitsPerIndex);

        size_t index = (size_t)offsetY * 16 * 16;
        size_t bitIndex = index * index * bitsPerIndex;
        size_t uint64Index = bitIndex / 64;
        uint64_t v0 = uint64Index < fBlockStates.size() ? *(uint64_t *)(fBlockStates.data() + uint64Index) : 0;
        uint64_t v1 = (uint64Index + 1) < fBlockStates.size() ? *(uint64_t *)(fBlockStates.data() + uint64Index + 1) : 0;
        size_t v0Remaining = 64;
        size_t v1Remaining = 64;

        for (int offsetZ = 0; offsetZ < 16; offsetZ++) {
            for (int offsetX = 0; offsetX < 16; offsetX++) {
                assert(v0Remaining > 0 && v1Remaining > 0);
                uint64_t paletteIndex = v0 & mask;
                int const v0Bits = (int)std::min(v0Remaining, bitsPerIndex);
                int const v1Bits = (int)(bitsPerIndex - v0Bits);
                v0Remaining -= v0Bits;
                if (v1Bits == 0) {
                    v0 >>= v0Bits;
                } else {
                    assert(v0Remaining == 0);
                    paletteIndex |= (v1 << v0Bits) & mask;
                    v1 >>= v1Bits;
                    v1Remaining -= v1Bits;
                    v0 = v1;
                    v0Remaining = v1Remaining;
                    uint64Index++;
                    v1 = (uint64Index + 1) < fBlockStates.size() ? *(uint64_t *)(fBlockStates.data() + uint64Index + 1) : 0;
                    v1Remaining = 64;
                }
                if (!callback(offsetX, offsetZ, paletteIndex)) {
                    return false;
                }
                index++;
            }
        }

        return true;
    }

    static int BlockIndex(int offsetX, int offsetY, int offsetZ) {
        if (offsetX < 0 || 16 <= offsetX || offsetY < 0 || 16 <= offsetY || offsetZ < 0 || 16 <= offsetZ) {
            return -1;
        }
        return offsetY * 16 * 16 + offsetZ * 16 + offsetX;
    }

public:
    int const fY;
    std::vector<std::shared_ptr<Block>> const fPalette;
    std::vector<blocks::BlockId> const fBlockIdPalette;
    std::vector<int64_t> const fBlockStates;
    std::vector<uint8_t> fBlockLight;
    std::vector<uint8_t> fSkyLight;
};

class ChunkSectionGenerator {
public:
    static std::shared_ptr<ChunkSection> MakeChunkSection(nbt::CompoundTag const* section, int const dataVersion) {
        if (dataVersion <= 1449) {
            //1444 (17w43a)
            //1449 (17w46a)
            return detail::ChunkSection_1_12::MakeChunkSection(section);
        } else {
            //1451 (17w47a)
            //1453 (17w48a)
            //1457 (17w50a)
            //1459 (18w01a)
            return detail::ChunkSection_1_13::MakeChunkSection(section);
        }
    }

private:
    ChunkSectionGenerator() = delete;
};

} // namespace detail

class Chunk {
public:
    std::shared_ptr<Block> blockAt(int x, int y, int z) const {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return nullptr;
        }
        if (y < 0 || 256 <= y) {
            return nullptr;
        }
        int const sectionY = y / 16;
        auto const& section = fSections[sectionY];
        if (!section) {
            return nullptr;
        }
        int const offsetX = x - chunkX * 16;
        int const offsetZ = z - chunkZ * 16;
        int const offsetY = y - sectionY * 16;
        return section->blockAt(offsetX, offsetY, offsetZ);
    }

    blocks::BlockId blockIdAt(int x, int y, int z) const {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return blocks::unknown;
        }
        if (y < 0 || 256 <= y) {
            return blocks::unknown;
        }
        int const sectionY = y / 16;
        auto const& section = fSections[sectionY];
        if (!section) {
            return blocks::unknown;
        }
        int const offsetX = x - chunkX * 16;
        int const offsetZ = z - chunkZ * 16;
        int const offsetY = y - sectionY * 16;
        return section->blockIdAt(offsetX, offsetY, offsetZ);
    }

    int blockLightAt(int x, int y, int z) const {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return 0;
        }
        if (y < 0 || 256 <= y) {
            return -1;
        }
        int const sectionY = y / 16;
        auto const& section = fSections[sectionY];
        if (!section) {
            return -1;
        }
        int const offsetX = x - chunkX * 16;
        int const offsetZ = z - chunkZ * 16;
        int const offsetY = y - sectionY * 16;
        return section->blockLightAt(offsetX, offsetY, offsetZ);
    }

    int skyLightAt(int x, int y, int z) const {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return 0;
        }
        if (y < 0 || 256 <= y) {
            return -1;
        }
        int const sectionY = y / 16;
        auto const& section = fSections[sectionY];
        if (!section) {
            return -1;
        }
        int const offsetX = x - chunkX * 16;
        int const offsetZ = z - chunkZ * 16;
        int const offsetY = y - sectionY * 16;
        return section->skyLightAt(offsetX, offsetY, offsetZ);
    }

    biomes::BiomeId biomeAt(int x, int z) const {
        int const offsetX = x - fChunkX * 16;
        int const offsetZ = z - fChunkZ * 16;

        if (offsetX < 0 || 16 <= offsetX || offsetZ < 0 || 16 <= offsetZ) {
            return biomes::unknown;
        }
        int const index = offsetZ * 16 + offsetX;
        if (index < fBiomes.size()) {
            return fBiomes[index];
        } else {
            return biomes::unknown;
        }
    }

    int minBlockX() const { return fChunkX * 16; }
    int maxBlockX() const { return fChunkX * 16 + 15; }

    int minBlockZ() const { return fChunkZ * 16; }
    int maxBlockZ() const { return fChunkZ * 16 + 15; }

    static std::shared_ptr<Chunk> MakeChunk(int chunkX, int chunkZ, nbt::CompoundTag const& root) {
        int dataVersion = 0;
        auto dataVersionTag = root.query("/DataVersion")->asInt();
        if (dataVersionTag) {
            // *.mca created by Minecraft 1.2.1 does not have /DataVersion tag
            dataVersion = dataVersionTag->fValue;
        }
        auto sectionsTag = root.query("/Level/Sections")->asList();
        if (!sectionsTag) {
            return nullptr;
        }
        std::vector<std::shared_ptr<ChunkSection>> sections;
        for (auto sectionTag : sectionsTag->fValue) {
            auto section = detail::ChunkSectionGenerator::MakeChunkSection(sectionTag->asCompound(), dataVersion);
            if (section) {
                sections.push_back(section);
            }
        }

        std::vector<biomes::BiomeId> biomes;
        auto biomesTag = root.query("/Level/Biomes");
        ParseBiomes(biomesTag, biomes);

        return std::shared_ptr<Chunk>(new Chunk(chunkX, chunkZ, sections, biomes));
    }

private:
    explicit Chunk(int chunkX, int chunkZ, std::vector<std::shared_ptr<ChunkSection>> const& sections, std::vector<biomes::BiomeId> & biomes)
        : fChunkX(chunkX)
        , fChunkZ(chunkZ)
    {
        for (auto section : sections) {
            int const y = section->y();
            if (0 <= y && y < 16) {
                fSections[y] = section;
            }
        }
        fBiomes.swap(biomes);
    }

    static void ParseBiomes(nbt::Tag const* biomesTag, std::vector<biomes::BiomeId> &result) {
        if (!biomesTag) {
            return;
        }
        if (biomesTag->id() == nbt::Tag::TAG_Int_Array) {
            std::vector<int32_t> const& value = biomesTag->asIntArray()->value();
            if (value.size() == 256) {
                result.resize(256);
                for (int i = 0; i < 256; i++) {
                    result[i] = biomes::FromInt(value[i]);
                }
            }
        } else if (biomesTag->id() == nbt::Tag::TAG_Byte_Array) {
            std::vector<uint8_t> const& value = biomesTag->asByteArray()->value();
            if (value.size() == 256) {
                result.resize(256);
                for (int i = 0; i < 256; i++) {
                    result[i] = biomes::FromInt(value[i]);
                }
            }
        }
    }

public:
    int const fChunkX;
    int const fChunkZ;
    std::shared_ptr<ChunkSection> fSections[16];
    std::vector<biomes::BiomeId> fBiomes;
};

namespace detail {

class ChunkDataSource {
public:
    ChunkDataSource(int chunkX, int chunkZ, uint32_t timestamp, long offset, long length)
        : fChunkX(chunkX), fChunkZ(chunkZ), fTimestamp(timestamp), fOffset(offset), fLength(length) {
    }

    bool load(StreamReader &reader, std::function<void(Chunk const &chunk)> callback) const {
        if (!reader.valid()) {
            return false;
        }
        if (!reader.seek(fOffset + sizeof(uint32_t))) {
            return false;
        }
        uint8_t compressionType;
        if (!reader.read(&compressionType)) {
            return false;
        }
        if (compressionType != 2) {
            return false;
        }
        std::vector<uint8_t> buffer(fLength - 1);
        if (!reader.read(buffer)) {
            return false;
        }
        if (!Compression::decompress(buffer)) {
            return false;
        }
        auto root = std::make_shared<nbt::CompoundTag>();
        auto bs = std::make_shared<ByteStream>(buffer);
        auto sr = std::make_shared<StreamReader>(bs);
        root->read(*sr);
        if (!root->valid()) {
            return false;
        }
        auto chunk = Chunk::MakeChunk(fChunkX, fChunkZ, *root);
        if (!chunk) {
            return false;
        }
        callback(*chunk);
        return true;
    }

public:
    int const fChunkX;
    int const fChunkZ;
    uint32_t const fTimestamp;
    long const fOffset;
    long const fLength;
};

} // namespace detail


class Region {
public:
    Region(Region const&) = delete;
    Region& operator = (Region const&) = delete;

    using LoadChunkCallback = std::function<bool(Chunk const&)>;

    bool loadAllChunks(bool& error, LoadChunkCallback callback) const {
        auto fs = std::make_shared<detail::FileStream>(fFilePath);
        detail::StreamReader sr(fs);
        for (int z = 0; z < 32; z++) {
            for (int x = 0; x < 32; x++) {
                if (!loadChunkImpl(x, z, sr, error, callback)) {
                    return false;
                }
            }
        }
        return true;
    }

    bool loadChunk(int localChunkX, int localChunkZ, bool& error, LoadChunkCallback callback) const {
        if (localChunkX < 0 || 32 <= localChunkX || localChunkZ < 0 || 32 <= localChunkZ) {
            return false;
        }
        auto fs = std::make_shared<detail::FileStream>(fFilePath);
        detail::StreamReader sr(fs);
        return loadChunkImpl(localChunkX, localChunkZ, sr, error, callback);
    }

    static std::shared_ptr<Region> MakeRegion(std::string const& filePath, int x, int z) {
        return std::shared_ptr<Region>(new Region(filePath, x, z));
    }

    static std::shared_ptr<Region> MakeRegion(std::string const& filePath) {
        // ../directory/r.5.13.mca

        auto basename = filePath;

        auto pos = filePath.find_last_of('/');
        if (pos == std::string::npos) {
            pos = filePath.find_last_of('\\');
        }
        if (pos != std::string::npos) {
            basename = filePath.substr(pos + 1);
        }

        std::vector<std::string> tokens = detail::String::Split(basename, '.');
        if (tokens.size() != 4) {
            return nullptr;
        }
        if (tokens[0] != "r" || tokens[3] != "mca") {
            return nullptr;
        }

        int x, z;
        try {
            x = std::stoi(tokens[1]);
            z = std::stoi(tokens[2]);
        } catch (...) {
            return nullptr;
        }

        return std::shared_ptr<Region>(new Region(filePath, x, z));
    }

    int minBlockX() const { return fX * 32 * 16; }
    int maxBlockX() const { return (fX + 1) * 32 * 16 - 1; }
    int minBlockZ() const { return fZ * 32 * 16; }
    int maxBlockZ() const { return (fZ + 1) * 32 * 16 - 1; }

    int minChunkX() const { return fX * 32; }
    int maxChunkX() const { return (fX + 1) * 32 - 1; }
    int minChunkZ() const { return fZ * 32; }
    int maxChunkZ() const { return (fZ + 1) * 32 - 1; }

    bool clearChunk(int chunkX, int chunkZ) {
        int const localChunkX = chunkX - fX * 32;
        int const localChunkZ = chunkZ - fZ * 32;
        if (localChunkX < 0 || 32 <= localChunkX) {
            return false;
        }
        if (localChunkZ < 0 || 32 <= localChunkZ) {
            return false;
        }

        FILE *fp = fopen(fFilePath.c_str(), "r+b");
        if (!fp) {
            return false;
        }
        int const index = (localChunkX & 31) + (localChunkZ & 31) * 32;
        if (fseek(fp, 4 * index, SEEK_SET) != 0) {
            fclose(fp);
            return false;
        }
        uint32_t loc;
        if (fread(&loc, sizeof(loc), 1, fp) != 1) {
            fclose(fp);
            return false;
        }
        loc = detail::StreamReader::Int32FromBE(loc);
        if (loc == 0) {
            fclose(fp);
            return true;
        }
        long const sectorOffset = loc >> 8;
        if (fseek(fp, 4 * index, SEEK_SET) != 0) {
            fclose(fp);
            return false;
        }
        loc = 0;
        if (fwrite(&loc, sizeof(loc), 1, fp) != 1) {
            fclose(fp);
            return false;
        }

        if (fseek(fp, kSectorSize + 4 * index, SEEK_SET) != 0) {
            fclose(fp);
            return false;
        }
        uint32_t timestamp = 0;
        if (fwrite(&timestamp, sizeof(timestamp), 1, fp) != 1) {
            fclose(fp);
            return false;
        }
        if (fseek(fp, sectorOffset * kSectorSize, SEEK_SET) != 0) {
            fclose(fp);
            return false;
        }
        uint32_t chunkSize;
        if (fread(&chunkSize, sizeof(chunkSize), 1, fp) != 1) {
            fclose(fp);
            return false;
        }
        chunkSize = detail::StreamReader::Int32FromBE(chunkSize);
        for (uint32_t i = 0; i < chunkSize; i++) {
            uint8_t zero = 0;
            if (fwrite(&zero, sizeof(zero), 1, fp) != 1) {
                fclose(fp);
                return false;
            }
        }

        fclose(fp);
        return true;
    }
    
    bool chunkExists(int chunkX, int chunkZ) const {
        int const localChunkX = chunkX - fX * 32;
        int const localChunkZ = chunkZ - fZ * 32;
        if (localChunkX < 0 || 32 <= localChunkX) {
            return false;
        }
        if (localChunkZ < 0 || 32 <= localChunkZ) {
            return false;
        }
        auto fs = std::make_shared<detail::FileStream>(fFilePath);
        detail::StreamReader sr(fs);
        auto data = dataSource(localChunkX, localChunkZ, sr);
        if (!data) {
            return false;
        }
        return data->fLength > 0;
    }

    static std::string getDefaultChunkNbtFileName(int chunkX, int chunkZ) {
        std::ostringstream s;
        s << "c." << chunkX << "." << chunkZ << ".nbt";
        return s.str();
    }

    static std::string getDefaultCompressedChunkNbtFileName(int chunkX, int chunkZ) {
        std::ostringstream s;
        s << "c." << chunkX << "." << chunkZ << ".nbt.z";
        return s.str();
    }

    bool exportAllToNbt(std::string const& directory, std::function<std::string(int, int)> name = Region::getDefaultChunkNbtFileName) const {
        int const minX = minChunkX();
        int const minZ = minChunkZ();
        int const maxX = maxChunkX();
        int const maxZ = maxChunkZ();
        for (int x = minX; x <= maxX; x++) {
            for (int z = minZ; z <= maxZ; z++) {
                std::string const n = name(x, z);
                std::string const path = directory + "/" + n;
                if (!exportToNbt(x, z, path)) {
                    return false;
                }
            }
        }
        return true;
    }

    bool exportAllToCompressedNbt(std::string const& directory, std::function<std::string(int, int)> name = Region::getDefaultCompressedChunkNbtFileName) const {
        int const minX = minChunkX();
        int const minZ = minChunkZ();
        int const maxX = maxChunkX();
        int const maxZ = maxChunkZ();
        for (int x = minX; x <= maxX; x++) {
            for (int z = minZ; z <= maxZ; z++) {
                std::string const n = name(x, z);
                std::string const path = directory + "/" + n;
                if (!exportToCompressedNbt(x, z, path)) {
                    return false;
                }
            }
        }
        return true;
    }

    bool exportToNbt(int chunkX, int chunkZ, std::string const& filePath) const {
        int const localChunkX = chunkX - fX * 32;
        int const localChunkZ = chunkZ - fZ * 32;
        if (localChunkX < 0 || 32 <= localChunkX) {
            return false;
        }
        if (localChunkZ < 0 || 32 <= localChunkZ) {
            return false;
        }

        FILE *in = fopen(fFilePath.c_str(), "rb");
        if (!in) {
            return false;
        }
        int const index = (localChunkX & 31) + (localChunkZ & 31) * 32;
        if (fseek(in, 4 * index, SEEK_SET) != 0) {
            fclose(in);
            return false;
        }
        uint32_t loc;
        if (fread(&loc, sizeof(loc), 1, in) != 1) {
            fclose(in);
            return false;
        }
        loc = detail::StreamReader::Int32FromBE(loc);
        if (loc == 0) {
            fclose(in);
            return true;
        }
        long const sectorOffset = loc >> 8;
        if (fseek(in, 4 * index, SEEK_SET) != 0) {
            fclose(in);
            return false;
        }
        if (fseek(in, sectorOffset * kSectorSize, SEEK_SET) != 0) {
            fclose(in);
            return false;
        }
        uint32_t chunkSize;
        if (fread(&chunkSize, sizeof(chunkSize), 1, in) != 1) {
            fclose(in);
            return false;
        }
        chunkSize = detail::StreamReader::Int32FromBE(chunkSize) - 1;
        
        uint8_t compressionType;
        if (fread(&compressionType, sizeof(compressionType), 1, in) != 1) {
            fclose(in);
            return false;
        }
        if (compressionType != 2) {
            fclose(in);
            return false;
        }

        FILE* out = fopen(filePath.c_str(), "wb");
        if (!out) {
            fclose(in);
            return false;
        }
        
        std::vector<uint8_t> buffer(chunkSize);
        if (fread(buffer.data(), chunkSize, 1, in) != 1) {
            fclose(in);
            fclose(out);
            return false;
        }
        if (!detail::Compression::decompress(buffer)) {
            fclose(in);
            fclose(out);
            return false;
        }
        if (fwrite(buffer.data(), buffer.size(), 1, out) != 1) {
            fclose(in);
            fclose(out);
            return false;
        }
        
        fclose(in);
        fclose(out);
        
        return true;
    }

    bool exportToCompressedNbt(int chunkX, int chunkZ, std::string const& filePath) const {
        int const localChunkX = chunkX - fX * 32;
        int const localChunkZ = chunkZ - fZ * 32;
        if (localChunkX < 0 || 32 <= localChunkX) {
            return false;
        }
        if (localChunkZ < 0 || 32 <= localChunkZ) {
            return false;
        }

        FILE *in = fopen(fFilePath.c_str(), "rb");
        if (!in) {
            return false;
        }
        int const index = (localChunkX & 31) + (localChunkZ & 31) * 32;
        if (fseek(in, 4 * index, SEEK_SET) != 0) {
            fclose(in);
            return false;
        }
        uint32_t loc;
        if (fread(&loc, sizeof(loc), 1, in) != 1) {
            fclose(in);
            return false;
        }
        loc = detail::StreamReader::Int32FromBE(loc);
        if (loc == 0) {
            fclose(in);
            return true;
        }
        long const sectorOffset = loc >> 8;
        if (fseek(in, 4 * index, SEEK_SET) != 0) {
            fclose(in);
            return false;
        }
        if (fseek(in, sectorOffset * kSectorSize, SEEK_SET) != 0) {
            fclose(in);
            return false;
        }
        uint32_t chunkSize;
        if (fread(&chunkSize, sizeof(chunkSize), 1, in) != 1) {
            fclose(in);
            return false;
        }
        chunkSize = detail::StreamReader::Int32FromBE(chunkSize) - 1;
        
        uint8_t compressionType;
        if (fread(&compressionType, sizeof(compressionType), 1, in) != 1) {
            fclose(in);
            return false;
        }

        FILE* out = fopen(filePath.c_str(), "wb");
        if (!out) {
            fclose(in);
            return false;
        }

        if (!detail::FileStream::copy(in, out, chunkSize)) {
            fclose(in);
            fclose(out);
            return false;
        }
        
        fclose(in);
        fclose(out);
        
        return true;
    }
    
private:
    Region(std::string const& filePath, int x, int z)
        : fX(x)
        , fZ(z)
        , fFilePath(filePath)
    {
    }

    std::shared_ptr<detail::ChunkDataSource> dataSource(int localChunkX, int localChunkZ, detail::StreamReader& sr) const {
        int const index = (localChunkX & 31) + (localChunkZ & 31) * 32;
        if (!sr.valid()) {
            return nullptr;
        }
        if (!sr.seek(4 * index)) {
            return nullptr;
        }

        uint32_t loc;
        if (!sr.read(&loc)) {
            return nullptr;
        }

        long sectorOffset = loc >> 8;
        if (!sr.seek(kSectorSize + 4 * index)) {
            return nullptr;
        }

        uint32_t timestamp;
        if (!sr.read(&timestamp)) {
            return nullptr;
        }

        if (!sr.seek(sectorOffset * kSectorSize)) {
            return nullptr;
        }
        uint32_t chunkSize;
        if (!sr.read(&chunkSize)) {
            return nullptr;
        }

        int const chunkX = this->fX * 32 + localChunkX;
        int const chunkZ = this->fZ * 32 + localChunkZ;
        return std::make_shared<detail::ChunkDataSource>(chunkX, chunkZ, timestamp, sectorOffset * kSectorSize, chunkSize);
    }
    
    bool loadChunkImpl(int regionX, int regionZ, detail::StreamReader& sr, bool& error, LoadChunkCallback callback) const {
        auto data = dataSource(regionX, regionZ, sr);
        if (!data) {
            error = true;
            return true;
        }
        if (data->load(sr, callback)) {
            return true;
        } else {
            error = true;
            return true;
        }
    }

public:
    int const fX;
    int const fZ;

private:
    std::string const fFilePath;

    static long const kSectorSize = 4096;
};

class World {
public:
    explicit World(std::string const& rootDirectory)
        : fRootDirectory(rootDirectory)
    {
    }

    std::shared_ptr<Region> region(int regionX, int regionZ) const {
        std::ostringstream ss;
        ss << fRootDirectory << "/region/r." << regionX << "." << regionZ << ".mca";
        auto fileName = ss.str();
        return Region::MakeRegion(fileName);
    }

    bool eachBlock(int minX, int minZ, int maxX, int maxZ, std::function<bool(int x, int y, int z, std::shared_ptr<Block>)> callback) const {
        if (minX > maxX || minZ > maxZ) {
            return false;
        }
        int const minRegionX = Coordinate::RegionFromBlock(minX);
        int const maxRegionX = Coordinate::RegionFromBlock(maxX);
        int const minRegionZ = Coordinate::RegionFromBlock(minZ);
        int const maxRegionZ = Coordinate::RegionFromBlock(maxZ);
        for (int regionZ = minRegionZ; regionZ <= maxRegionZ; regionZ++) {
            for (int regionX = minRegionX; regionX <= maxRegionX; regionX++) {
                auto region = this->region(regionX, regionZ);
                if (!region) {
                    continue;
                }
                bool error = false;
                return region->loadAllChunks(error, [minX, minZ, maxX, maxZ, callback](Chunk const& chunk) {
                    for (int y = 0; y < 256; y++) {
                        for (int z = std::max(minZ, chunk.minBlockZ()); z <= std::min(maxZ, chunk.maxBlockZ()); z++) {
                            for (int x = std::max(minX, chunk.minBlockX()); x <= std::min(maxX, chunk.maxBlockX()); x++) {
                                auto block = chunk.blockAt(x, y, z);
                                if (!callback(x, y, z, block)) {
                                    return false;
                                }
                            }
                        }
                    }
                    return true;
                });
            }
        }
        return true;
    }

    bool eachRegions(std::function<void(std::shared_ptr<Region> const&)> callback) const {
        auto regionDir = std::filesystem::path(fRootDirectory).append("region");
        std::filesystem::directory_iterator it(regionDir);
        for (auto const& path : it) {
            auto region = Region::MakeRegion(path.path());
            if (!region) {
                continue;
            }
            callback(region);
        }
        return true;
    }
    
public:
    std::string const fRootDirectory;
};

} // namespace mcfile
