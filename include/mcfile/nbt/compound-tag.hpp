#pragma once

namespace mcfile::nbt {

class CompoundTag : public Tag {
protected:
    bool readImpl(::mcfile::stream::InputStreamReader &r) override {
        std::map<std::u8string, std::shared_ptr<Tag>> tmp;
        while (r.valid()) {
            uint8_t type;
            if (!r.read(&type)) {
                break;
            }
            if (type == static_cast<uint8_t>(Tag::Type::End)) {
                break;
            }

            std::string key;
            if (!r.read(key)) {
                return false;
            }
            auto name = String::Utf8FromJavaUtf8(key);
            if (!name) {
                return false;
            }

            auto tag = TagFactory::makeTag(type);
            if (!tag) {
                return false;
            }
            if (!tag->readImpl(r)) {
                return false;
            }
            tmp[*name] = tag;
        }
        fValue.swap(tmp);
        return true;
    }

    bool writeImpl(::mcfile::stream::OutputStreamWriter &w) const override {
        for (auto it = fValue.begin(); it != fValue.end(); it++) {
            Tag::Type type = it->second->type();
            if (!w.write(static_cast<uint8_t>(type))) {
                return false;
            }
            auto s = String::JavaUtf8FromUtf8(it->first);
            if (!s) {
                return false;
            }
            if (!w.write(*s)) {
                return false;
            }
            if (!it->second->writeImpl(w)) {
                return false;
            }
        }
        return w.write(static_cast<uint8_t>(Tag::Type::End));
    }

public:
    bool read(::mcfile::stream::InputStreamReader &r) {
        return readImpl(r);
    }

    bool write(::mcfile::stream::OutputStreamWriter &w) const {
        return writeImpl(w);
    }

    [[nodiscard]] static bool Write(CompoundTag const &tag, mcfile::stream::OutputStreamWriter &w) {
        if (!w.write(static_cast<uint8_t>(Tag::Type::Compound))) {
            return false;
        }
        if (!w.write(std::string())) {
            return false;
        }
        return tag.writeImpl(w);
    }

    [[nodiscard]] static bool Write(CompoundTag const &tag, std::filesystem::path const &file, Endian endian) {
        auto stream = std::make_shared<mcfile::stream::FileOutputStream>(file);
        return Write(tag, stream, endian);
    }

    [[nodiscard]] static bool Write(CompoundTag const &tag, std::shared_ptr<mcfile::stream::OutputStream> const &stream, Endian endian) {
        mcfile::stream::OutputStreamWriter w(stream, endian);
        return Write(tag, w);
    }

    static std::optional<std::string> Write(CompoundTag const &tag, Endian endian) {
        auto s = std::make_shared<mcfile::stream::ByteStream>();
        if (!Write(tag, s, endian)) {
            return std::nullopt;
        }
        std::string ret;
        s->drain(ret);
        return ret;
    }

    Tag::Type type() const override { return Tag::Type::Compound; }

    Tag const *query(std::u8string const &path) const {
        // path: /Level/Sections
        if (path.empty()) {
            return EndTag::Instance();
        }
        std::u8string p = path;
        Tag const *pivot = this;
        while (!p.empty()) {
            if (p[0] == u8'/') {
                if (fValue.size() != 1) {
                    return EndTag::Instance();
                }
                auto const &child = fValue.begin()->second;
                if (p.size() == 1) {
                    return child.get();
                }
                if (child->type() != Tag::Type::Compound) {
                    return EndTag::Instance();
                }
                pivot = child->asCompound();
                p = p.substr(1);
            } else {
                auto pos = p.find_first_of(u8'/');
                std::u8string name;
                if (pos == std::u8string::npos) {
                    name = p;
                } else {
                    name = p.substr(0, pos);
                }
                if (pivot->type() == Tag::Type::List) {
                    auto list = pivot->asList();
                    if (!list)
                        return EndTag::Instance();
                    int index;
                    try {
                        index = String::Toi<int>(name);
                        if (index < 0 || list->size() <= index) {
                            return EndTag::Instance();
                        }
                        auto const &child = list->at(index);
                        if (pos == std::u8string::npos) {
                            return child.get();
                        }
                        auto id = child->type();
                        if (id != Tag::Type::Compound && id != Tag::Type::List) {
                            return EndTag::Instance();
                        }
                        pivot = child.get();
                        p = p.substr(pos + 1);
                    } catch (...) {
                        return EndTag::Instance();
                    }
                } else if (pivot->type() == Tag::Type::Compound) {
                    auto comp = pivot->asCompound();
                    if (!comp)
                        return EndTag::Instance();

                    auto child = comp->fValue.find(name);
                    if (child == comp->fValue.end()) {
                        return EndTag::Instance();
                    }
                    if (pos == std::u8string::npos) {
                        return child->second.get();
                    }
                    auto id = child->second->type();
                    if (id != Tag::Type::Compound && id != Tag::Type::List) {
                        return EndTag::Instance();
                    }
                    pivot = child->second.get();
                    p = p.substr(pos + 1);
                }
            }
        }
        return EndTag::Instance();
    }

    std::shared_ptr<Tag> &operator[](std::u8string const &name) {
        return fValue[name];
    }

    void insert(std::pair<std::u8string, std::shared_ptr<Tag>> const &item) {
        fValue[item.first] = item.second;
    }

    void insert(std::initializer_list<std::pair<std::u8string const, std::shared_ptr<Tag>>> l) {
        for (auto const &it : l) {
            fValue[it.first] = it.second;
        }
    }

    decltype(auto) find(std::u8string const &name) const { return fValue.find(name); }
    decltype(auto) begin() const { return fValue.begin(); }
    decltype(auto) end() const { return fValue.end(); }
    size_t size() const { return fValue.size(); }
    bool empty() const { return fValue.empty(); }

    void erase(std::u8string const &name) { fValue.erase(name); }

    std::shared_ptr<Tag> tag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end()) {
            return nullptr;
        }
        return found->second;
    }

    std::shared_ptr<StringTag> stringTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::String)
            return nullptr;
        return std::dynamic_pointer_cast<StringTag>(found->second);
    }

    std::shared_ptr<ByteTag> byteTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::Byte)
            return nullptr;
        return std::dynamic_pointer_cast<ByteTag>(found->second);
    }

    std::shared_ptr<CompoundTag> compoundTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::Compound)
            return nullptr;
        return std::dynamic_pointer_cast<CompoundTag>(found->second);
    }

    std::shared_ptr<ListTag> listTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::List)
            return nullptr;
        return std::dynamic_pointer_cast<ListTag>(found->second);
    }

    std::shared_ptr<IntTag> intTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::Int)
            return nullptr;
        return std::dynamic_pointer_cast<IntTag>(found->second);
    }

    std::shared_ptr<LongTag> longTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::Long)
            return nullptr;
        return std::dynamic_pointer_cast<LongTag>(found->second);
    }

    std::shared_ptr<ShortTag> shortTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::Short)
            return nullptr;
        return std::dynamic_pointer_cast<ShortTag>(found->second);
    }

    std::shared_ptr<FloatTag> floatTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::Float)
            return nullptr;
        return std::dynamic_pointer_cast<FloatTag>(found->second);
    }

    std::shared_ptr<DoubleTag> doubleTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::Double)
            return nullptr;
        return std::dynamic_pointer_cast<DoubleTag>(found->second);
    }

    std::shared_ptr<IntArrayTag> intArrayTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::IntArray)
            return nullptr;
        return std::dynamic_pointer_cast<IntArrayTag>(found->second);
    }

    std::shared_ptr<ByteArrayTag> byteArrayTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::ByteArray)
            return nullptr;
        return std::dynamic_pointer_cast<ByteArrayTag>(found->second);
    }

    std::shared_ptr<LongArrayTag> longArrayTag(std::u8string const &name) const {
        auto found = fValue.find(name);
        if (found == fValue.end())
            return nullptr;
        if (!found->second)
            return nullptr;
        if (found->second->type() != Tag::Type::LongArray)
            return nullptr;
        return std::dynamic_pointer_cast<LongArrayTag>(found->second);
    }

    template<class Tag>
    std::shared_ptr<Tag> get(std::u8string const &name) const;

    template<class Value>
    std::optional<Value> value(std::u8string const &name) const;

    std::optional<int8_t> byte(std::u8string const &name) const {
        auto v = byteTag(name);
        if (v) {
            return v->fValue;
        } else {
            return std::nullopt;
        }
    }

    std::optional<bool> boolean(std::u8string const &name) const {
        auto v = byteTag(name);
        if (v) {
            return v->fValue != 0;
        } else {
            return std::nullopt;
        }
    }

    std::optional<int16_t> int16(std::u8string const &name) const {
        auto v = shortTag(name);
        if (v) {
            return v->fValue;
        } else {
            return std::nullopt;
        }
    }

    std::optional<int32_t> int32(std::u8string const &name) const {
        auto v = intTag(name);
        if (v) {
            return v->fValue;
        } else {
            return std::nullopt;
        }
    }

    std::optional<int64_t> int64(std::u8string const &name) const {
        auto v = longTag(name);
        if (v) {
            return v->fValue;
        } else {
            return std::nullopt;
        }
    }

    std::optional<std::u8string> string(std::u8string const &name) const {
        auto v = stringTag(name);
        if (v) {
            return v->fValue;
        } else {
            return std::nullopt;
        }
    }

    std::optional<float> float32(std::u8string const &name) const {
        auto v = floatTag(name);
        if (v) {
            return v->fValue;
        } else {
            return std::nullopt;
        }
    }

    std::optional<double> float64(std::u8string const &name) const {
        auto v = doubleTag(name);
        if (v) {
            return v->fValue;
        } else {
            return std::nullopt;
        }
    }

    int8_t byte(std::u8string const &name, int8_t fallback) const {
        auto v = byteTag(name);
        return v ? v->fValue : fallback;
    }

    bool boolean(std::u8string const &name, bool fallback) const {
        auto v = byteTag(name);
        return v ? v->fValue != 0 : fallback;
    }

    int16_t int16(std::u8string const &name, int16_t fallback) const {
        auto v = shortTag(name);
        return v ? v->fValue : fallback;
    }

    int32_t int32(std::u8string const &name, int32_t fallback) const {
        auto v = intTag(name);
        return v ? v->fValue : fallback;
    }

    int64_t int64(std::u8string const &name, int64_t fallback) const {
        auto v = longTag(name);
        return v ? v->fValue : fallback;
    }

    std::u8string string(std::u8string const &name, std::u8string const &fallback) const {
        auto v = stringTag(name);
        return v ? v->fValue : fallback;
    }

    float float32(std::u8string const &name, float fallback) const {
        auto v = floatTag(name);
        return v ? v->fValue : fallback;
    }

    double float64(std::u8string const &name, double fallback) const {
        auto v = doubleTag(name);
        return v ? v->fValue : fallback;
    }

    void set(std::u8string const &name, std::shared_ptr<Tag> const &value) {
        fValue[name] = value;
    }

    void set(std::u8string const &name, std::u8string const &value) {
        fValue[name] = std::make_shared<StringTag>(value);
    }

    std::shared_ptr<Tag> clone() const override {
        return copy();
    }

    std::shared_ptr<CompoundTag> copy() const {
        auto ret = std::make_shared<CompoundTag>();
        for (auto const &it : fValue) {
            ret->set(it.first, it.second->clone());
        }
        return ret;
    }

    bool equals(CompoundTag const &o) const {
        using namespace std;
        if (size() != o.size()) {
            return false;
        }
        for (auto const &it : *this) {
            auto tagO = o.tag(it.first);
            if (!tagO) {
                return false;
            }
            if (!Equals(*tagO, *it.second)) {
                return false;
            }
        }
        return true;
    }

    static bool Equals(Tag const &a, Tag const &b) {
        if (a.type() != b.type()) {
            return false;
        }
        switch (a.type()) {
        case Tag::Type::Byte:
            if (!a.asByte()->equals(*b.asByte())) {
                return false;
            }
            break;
        case Tag::Type::ByteArray:
            if (!a.asByteArray()->equals(*b.asByteArray())) {
                return false;
            }
            break;
        case Tag::Type::Compound:
            if (!a.asCompound()->equals(*b.asCompound())) {
                return false;
            }
            break;
        case Tag::Type::Double:
            if (!a.asDouble()->equals(*b.asDouble())) {
                return false;
            }
            break;
        case Tag::Type::End:
            break;
        case Tag::Type::Float:
            if (!a.asFloat()->equals(*b.asFloat())) {
                return false;
            }
            break;
        case Tag::Type::Int:
            if (!a.asInt()->equals(*b.asInt())) {
                return false;
            }
            break;
        case Tag::Type::IntArray:
            if (!a.asIntArray()->equals(*b.asIntArray())) {
                return false;
            }
            break;
        case Tag::Type::List: {
            ListTag const &av = *a.asList();
            ListTag const &bv = *b.asList();
            if (av.size() != bv.size()) {
                return false;
            }
            if (av.fType != bv.fType) {
                return false;
            }
            for (size_t i = 0; i < av.size(); i++) {
                auto const &avi = av.at(i);
                auto const &bvi = bv.at(i);
                if (avi && bvi) {
                    if (!Equals(*avi, *bvi)) {
                        return false;
                    }
                } else {
                    return false;
                }
            }
            break;
        }
        case Tag::Type::Long:
            if (!a.asLong()->equals(*b.asLong())) {
                return false;
            }
            break;
        case Tag::Type::LongArray:
            if (!a.asLongArray()->equals(*b.asLongArray())) {
                return false;
            }
            break;
        case Tag::Type::Short:
            if (!a.asShort()->equals(*b.asShort())) {
                return false;
            }
            break;
        case Tag::Type::String:
            if (!a.asString()->equals(*b.asString())) {
                return false;
            }
            break;
        default:
            return false;
        }
        return true;
    }

    static void ReadSequential(stream::InputStreamReader &reader, std::function<bool(std::shared_ptr<CompoundTag> const &value)> callback) {
        ReadUntilEosImpl(reader, callback);
    }

    static void ReadSequentialUntilEos(stream::InputStreamReader &reader, std::function<void(std::shared_ptr<CompoundTag> const &value)> callback) {
        ReadUntilEosImpl(reader, [callback](std::shared_ptr<CompoundTag> const &value) {
            callback(value);
            return true;
        });
    }

    static void ReadSequential(std::string const &data, Endian endian, std::function<bool(std::shared_ptr<CompoundTag> const &value)> callback) {
        auto s = std::make_shared<mcfile::stream::ByteInputStream>(data);
        mcfile::stream::InputStreamReader reader(s, endian);
        ReadUntilEosImpl(reader, callback);
    }

    static void ReadSequentialUntilEos(std::string const &data, Endian endian, std::function<void(std::shared_ptr<CompoundTag> const &value)> callback) {
        auto s = std::make_shared<mcfile::stream::ByteInputStream>(data);
        mcfile::stream::InputStreamReader reader(s, endian);
        ReadUntilEosImpl(reader, [callback](std::shared_ptr<CompoundTag> const &value) {
            callback(value);
            return true;
        });
    }

    static std::shared_ptr<CompoundTag> ReadFromFile(std::filesystem::path path, Endian endian) {
        auto s = std::make_shared<mcfile::stream::FileInputStream>(path);
        return Read(s, endian);
    }

    static std::shared_ptr<CompoundTag> Read(std::string const &data, Endian endian) {
        auto s = std::make_shared<mcfile::stream::ByteInputStream>(data);
        return Read(s, endian);
    }

    static std::shared_ptr<CompoundTag> Read(std::vector<uint8_t> const &buffer, Endian endian) {
        std::vector<uint8_t> tmp;
        tmp.reserve(buffer.size());
        std::copy(buffer.begin(), buffer.end(), std::back_inserter(tmp));
        auto s = std::make_shared<mcfile::stream::ByteStream>(tmp);
        return Read(s, endian);
    }

    static std::shared_ptr<CompoundTag> Read(std::shared_ptr<stream::InputStream> const &stream, Endian endian) {
        mcfile::stream::InputStreamReader isr(stream, endian);
        return Read(isr);
    }

    static std::shared_ptr<CompoundTag> Read(mcfile::stream::InputStreamReader &isr) {
        uint8_t type;
        if (!isr.read(&type)) {
            return nullptr;
        }
        if (type != static_cast<uint8_t>(Tag::Type::Compound)) {
            return nullptr;
        }
        std::string n;
        if (!isr.read(n)) {
            return nullptr;
        }
        assert(n.empty());
        auto tag = std::make_shared<CompoundTag>();
        if (!tag->readImpl(isr)) {
            return nullptr;
        }
        return tag;
    }

    static std::shared_ptr<CompoundTag> ReadCompressedFromFile(std::filesystem::path p, Endian endian) {
        auto s = std::make_shared<mcfile::stream::FileInputStream>(p);
        return ReadDeflateCompressed(*s, endian);
    }

    static std::shared_ptr<CompoundTag> ReadDeflateCompressed(std::string const &data, Endian endian) {
        std::vector<uint8_t> buffer;
        buffer.reserve(data.size());
        std::copy(data.begin(), data.end(), std::back_inserter(buffer));
        return ReadDeflateCompressed(buffer, endian);
    }

    static std::shared_ptr<CompoundTag> ReadDeflateCompressed(mcfile::stream::InputStream &stream, Endian endian) {
        std::vector<uint8_t> buffer;
        mcfile::stream::InputStream::ReadUntilEos(stream, buffer);
        return ReadDeflateCompressed(buffer, endian);
    }

    static std::shared_ptr<CompoundTag> ReadDeflateCompressed(std::vector<uint8_t> const &buffer, Endian endian) {
        std::vector<uint8_t> tmp;
        tmp.reserve(buffer.size());
        std::copy(buffer.begin(), buffer.end(), std::back_inserter(tmp));
        if (!Compression::DecompressZlib(tmp)) {
            return nullptr;
        }
        return Read(tmp, endian);
    }

    static std::shared_ptr<CompoundTag> ReadLz4Compressed(std::string const &data, Endian endian) {
        std::vector<uint8_t> buffer;
        buffer.reserve(data.size());
        std::copy(data.begin(), data.end(), std::back_inserter(buffer));
        if (!Compression::DecompressLz4(buffer)) {
            return nullptr;
        }
        return Read(buffer, endian);
    }

    static std::shared_ptr<CompoundTag> ReadLz4Compressed(mcfile::stream::InputStream &stream, Endian endian) {
        std::vector<uint8_t> buffer;
        mcfile::stream::InputStream::ReadUntilEos(stream, buffer);
        if (!Compression::DecompressLz4(buffer)) {
            return nullptr;
        }
        return Read(buffer, endian);
    }

    static std::shared_ptr<CompoundTag> ReadLz4Compressed(std::vector<uint8_t> const &buffer, Endian endian) {
        std::vector<uint8_t> tmp;
        tmp.reserve(buffer.size());
        std::copy(buffer.begin(), buffer.end(), std::back_inserter(tmp));
        if (!Compression::DecompressLz4(tmp)) {
            return nullptr;
        }
        return Read(tmp, endian);
    }

    static bool WriteDeflateCompressed(CompoundTag const &tag, std::filesystem::path file, Endian endian) {
        auto o = std::make_shared<stream::FileOutputStream>(file);
        return WriteDeflateCompressed(tag, *o, endian);
    }

    static std::optional<std::string> WriteDeflateCompressed(CompoundTag const &tag, Endian endian) {
        auto s = std::make_shared<stream::ByteStream>();
        if (!WriteDeflateCompressed(tag, *s, endian)) {
            return std::nullopt;
        }
        std::string ret;
        s->drain(ret);
        return ret;
    }

    static bool WriteDeflateCompressed(CompoundTag const &tag, mcfile::stream::OutputStream &stream, Endian endian) {
        auto s = std::make_shared<stream::ByteStream>();
        stream::OutputStreamWriter osr(s, endian);
        if (!Write(tag, osr)) {
            return false;
        }
        std::vector<uint8_t> buffer;
        s->drain(buffer);
        if (!Compression::CompressZlib(buffer)) {
            return false;
        }
        return stream.write(buffer.data(), buffer.size());
    }

private:
    static void ReadUntilEosImpl(stream::InputStreamReader &reader, std::function<bool(std::shared_ptr<CompoundTag> const &value)> callback) {
        while (reader.valid()) {
            uint8_t type;
            if (!reader.read(&type)) {
                break;
            }
            std::string name;
            if (!reader.read(name)) {
                break;
            }
            auto tag = std::make_shared<CompoundTag>();
            if (!tag->readImpl(reader)) {
                break;
            }
            if (!callback(tag)) {
                break;
            }
        }
    }

public:
    std::map<std::u8string, std::shared_ptr<Tag>> fValue;
};

template<>
inline std::shared_ptr<ByteTag> CompoundTag::get(std::u8string const &name) const {
    return byteTag(name);
}

template<>
inline std::shared_ptr<ShortTag> CompoundTag::get(std::u8string const &name) const {
    return shortTag(name);
}

template<>
inline std::shared_ptr<IntTag> CompoundTag::get(std::u8string const &name) const {
    return intTag(name);
}

template<>
inline std::shared_ptr<LongTag> CompoundTag::get(std::u8string const &name) const {
    return longTag(name);
}

template<>
inline std::shared_ptr<FloatTag> CompoundTag::get(std::u8string const &name) const {
    return floatTag(name);
}

template<>
inline std::shared_ptr<DoubleTag> CompoundTag::get(std::u8string const &name) const {
    return doubleTag(name);
}

template<>
inline std::shared_ptr<ByteArrayTag> CompoundTag::get(std::u8string const &name) const {
    return byteArrayTag(name);
}

template<>
inline std::shared_ptr<StringTag> CompoundTag::get(std::u8string const &name) const {
    return stringTag(name);
}

template<>
inline std::shared_ptr<ListTag> CompoundTag::get(std::u8string const &name) const {
    return listTag(name);
}

template<>
inline std::shared_ptr<CompoundTag> CompoundTag::get(std::u8string const &name) const {
    return compoundTag(name);
}

template<>
inline std::shared_ptr<IntArrayTag> CompoundTag::get(std::u8string const &name) const {
    return intArrayTag(name);
}

template<>
inline std::shared_ptr<LongArrayTag> CompoundTag::get(std::u8string const &name) const {
    return longArrayTag(name);
}

template<>
inline std::optional<int8_t> CompoundTag::value(std::u8string const &name) const {
    return byte(name);
}

template<>
inline std::optional<int16_t> CompoundTag::value(std::u8string const &name) const {
    return int16(name);
}

template<>
inline std::optional<int32_t> CompoundTag::value(std::u8string const &name) const {
    return int32(name);
}

template<>
inline std::optional<int64_t> CompoundTag::value(std::u8string const &name) const {
    return int64(name);
}

template<>
inline std::optional<float> CompoundTag::value(std::u8string const &name) const {
    return float32(name);
}

template<>
inline std::optional<double> CompoundTag::value(std::u8string const &name) const {
    return float64(name);
}

template<>
inline std::optional<std::u8string> CompoundTag::value(std::u8string const &name) const {
    return string(name);
}

template<>
inline std::optional<bool> CompoundTag::value(std::u8string const &name) const {
    return boolean(name);
}

} // namespace mcfile::nbt
