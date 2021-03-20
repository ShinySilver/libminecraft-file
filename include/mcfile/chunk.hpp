#pragma once

namespace mcfile {

class Chunk {
public:
    std::shared_ptr<Block const> blockAt(int x, int y, int z) const {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return nullptr;
        }
        auto const& section = sectionAtBlock(y);
        if (!section) {
            return nullptr;
        }
        int const offsetX = x - chunkX * 16;
        int const offsetZ = z - chunkZ * 16;
        int const offsetY = y - section->y() * 16;
        return section->blockAt(offsetX, offsetY, offsetZ);
    }

    blocks::BlockId blockIdAt(int x, int y, int z) const {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return blocks::unknown;
        }
        auto const& section = sectionAtBlock(y);
        if (!section) {
            return blocks::unknown;
        }
        int const offsetX = x - chunkX * 16;
        int const offsetZ = z - chunkZ * 16;
        int const offsetY = y - section->y() * 16;
        return section->blockIdAt(offsetX, offsetY, offsetZ);
    }

    bool setBlockAt(int x, int y, int z, std::shared_ptr<Block const> const& block, SetBlockOptions options = SetBlockOptions()) {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return false;
        }
        auto const& section = sectionAtBlock(y);
        if (!section) {
            return false;
        }
        int const offsetX = x - chunkX * 16;
        int const offsetZ = z - chunkZ * 16;
        int const offsetY = y - section->y() * 16;
        if (!section->setBlockAt(offsetX, offsetY, offsetZ, block)) {
            return false;
        }
        if (options.fRemoveTileEntity) {
            return removeTileEntityAt(x, y, z);
        } else {
            return true;
        }
    }

    bool removeTileEntityAt(int x, int y, int z) {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return false;
        }
        for (size_t i = 0; i < fTileEntities.size(); i++) {
            auto const& te = fTileEntities[i];
            auto tx = te->int32("x");
            auto ty = te->int32("y");
            auto tz = te->int32("z");
            if (!tx || !ty || !tz) {
                continue;
            }
            if (*tx == x && *ty == y && *tz == z) {
                fTileEntities.erase(fTileEntities.begin() + i);
                break;
            }
        }
        return true;
    }
    
    int blockLightAt(int x, int y, int z) const {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return 0;
        }
        auto const& section = sectionAtBlock(y);
        if (!section) {
            return -1;
        }
        int const offsetX = x - chunkX * 16;
        int const offsetZ = z - chunkZ * 16;
        int const offsetY = y - section->y() * 16;
        return section->blockLightAt(offsetX, offsetY, offsetZ);
    }

    int skyLightAt(int x, int y, int z) const {
        int const chunkX = Coordinate::ChunkFromBlock(x);
        int const chunkZ = Coordinate::ChunkFromBlock(z);
        if (chunkX != fChunkX || chunkZ != fChunkZ) {
            return 0;
        }
        auto const& section = sectionAtBlock(y);
        if (!section) {
            return -1;
        }
        int const offsetX = x - chunkX * 16;
        int const offsetZ = z - chunkZ * 16;
        int const offsetY = y - section->y() * 16;
        return section->skyLightAt(offsetX, offsetY, offsetZ);
    }

    biomes::BiomeId biomeAt(int x, int y, int z) const {
        if (fDataVersion >= 2203) { // 19w36a
            int const offsetX = (x - fChunkX * 16) / 4;
            int const offsetY = (y - fMinChunkSectionY * 16) / 4;
            int const offsetZ = (z - fChunkZ * 16) / 4;
            if (offsetX < 0 || 4 <= offsetX || offsetZ < 0 || 4 <= offsetZ || offsetY < 0) {
                return biomes::unknown;
            }
            int const index = offsetX + offsetZ * 4 + offsetY * 16;
            if (index < fBiomes.size()) {
                return fBiomes[index];
            } else {
                return biomes::unknown;
            }
        } else {
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
    }

    biomes::BiomeId biomeAt(int x, int z) const {
        return biomeAt(x, 0, z);
    }

    int minBlockX() const { return fChunkX * 16; }
    int maxBlockX() const { return fChunkX * 16 + 15; }

    int minBlockY() const { return fMinChunkSectionY * 16; }
    int maxBlockY() const { return fMinChunkSectionY < 0 ? 319 : 255; }

    int minBlockZ() const { return fChunkZ * 16; }
    int maxBlockZ() const { return fChunkZ * 16 + 15; }

    enum class Status {
        UNKNOWN,
        FULL,
    };

    Status status() const {
        // Java Edition 1.15.2 2230 "full"
        // Java Edition 1.14.1 1957 "full"
        // Java Edition 19w02a 1921 "full"
        // Java Edition 18w47a 1912 "full"
        // Java Edition 18w46a 1910 "fullchunk"
        // Java Edition 18w45a 1908 "fullchunk"
        // Java Edition 18w44a 1907 "fullchunk"
        // Java Edition 18w43a 1901 "fullchunk"
        // Java Edition 1.13.2 1631 "postprocessed" | "fullchunk"
        // Java Edition 1.13.2-pre2 1630 "postprocessed"
        // Java Edition 1.13.2-pre1 1629 "postprocessed"
        // Java Edition 1.13.1-pre1 1626 "postprocessed"
        // ...
        // Java Edition 18w30a 1620 "postprocessed"
        // ...
        // Java Edition 1.13-pre1 1501 "postprocessed"
        // ...
        // Java Edition 18w15a 1482 "finalized"
        // Java Edition 18w14b 1481 "postprocessed"
        // Java Edition 18w14a 1479 "postprocessed"
        // Java Edition 18w11a 1478 "postprocessed"
        // Java Edition 18w08a 1470 "postprocessed"
        // ...
        // Java Edition 18w07a 1467 "postprocessed"
        // Java Edition 18w06a 1466 "postprocessed"
        // Java Edition 18w05a 1464 TerrianPopulated 1
        // ...
        // Java Edition 18w01a 1459 TerrianPopulated 1
        // ...
        // Java Edition 17w47a 1451 TerrianPopulated 1
        if (fDataVersion >= 1912) {
            if (fStatus == "full") {
                return Status::FULL;
            } else {
                return Status::UNKNOWN;
            }
        } else if (fDataVersion >= 1466) {
            if (fStatus == "postprocessed" || fStatus == "finalized" || fStatus == "fullchunk") {
                return Status::FULL;
            } else {
                return Status::UNKNOWN;
            }
        } else {
            if (fTerrianPopulated && *fTerrianPopulated) {
                return Status::FULL;
            } else {
                return Status::UNKNOWN;
            }
        }
    }

    std::shared_ptr<nbt::CompoundTag> toCompoundTag() const {
        using namespace std;
        using namespace mcfile::nbt;
        auto root = make_shared<CompoundTag>();
        root->set("DataVersion", make_shared<IntTag>(fDataVersion));

        auto level = make_shared<CompoundTag>();

        level->set("xPos", make_shared<IntTag>(fChunkX));
        level->set("zPos", make_shared<IntTag>(fChunkZ));

        vector<shared_ptr<ChunkSection>> sections;
        for (auto const& section : fSections) {
            if (section) {
                sections.push_back(section);
            }
        }
        if (!sections.empty()) {
            sort(sections.begin(), sections.end(), [](auto const& a, auto const& b) {
                return a->rawY() < b->rawY();
            });
            auto sectionsList = make_shared<ListTag>();
            sectionsList->fType = Tag::TAG_Compound;
            for (auto const& section : sections) {
                auto s = section->toCompoundTag();
                if (!s) {
                    return nullptr;
                }
                sectionsList->push_back(s);
            }
            level->set("Sections", sectionsList);
        }
        
        std::vector<int32_t> biomes;
        for (biomes::BiomeId biome : fBiomes) {
            biomes.push_back(static_cast<int32_t>(biome));
        }
        level->set("Biomes", make_shared<IntArrayTag>(biomes));
        
        auto entities = make_shared<ListTag>();
        entities->fType = Tag::TAG_Compound;
        for (auto const& entity : fEntities) {
            entities->push_back(entity->clone());
        }
        level->set("Entities", entities);
        
        auto tileEntities = make_shared<ListTag>();
        tileEntities->fType = Tag::TAG_Compound;
        for (auto const& tileEntity : fTileEntities) {
            tileEntities->push_back(tileEntity->clone());
        }
        level->set("TileEntities", tileEntities);

        if (fStructures) {
            level->set("Structures", fStructures->clone());
        }
        
        level->set("Status", make_shared<StringTag>(fStatus));

        static set<string> const whitelist = {
            "DataVersion",
            "xPos",
            "zPos",
            "Sections",
            "Biomes",
            "Entities",
            "TileEntities",
            "Structures",
            "Status",
        };
        CompoundTag const* existingLevel = fRoot->query("/Level")->asCompound();
            if (existingLevel) {
            for (auto it : *existingLevel) {
                if (whitelist.find(it.first) != whitelist.end()) {
                    continue;
                }
                level->set(it.first, it.second->clone());
            }
        }

        root->set("Level", level);
        return root;
    }
    
    bool write(stream::OutputStream &s) const {
        using namespace std;
        using namespace mcfile;
        auto compound = toCompoundTag();
        if (!compound) {
            return false;
        }
        auto stream = make_shared<stream::ByteStream>();
        auto writer = make_shared<stream::OutputStreamWriter>(stream, stream::WriteOption{.fLittleEndian = false});
        auto root = make_shared<nbt::CompoundTag>();
        root->set("", compound);
        root->write(*writer);
        std::vector<uint8_t> buffer;
        stream->drain(buffer);
        detail::Compression::compress(buffer);
        s.write(buffer.data(), buffer.size());
        return true;
    }
    
    static std::shared_ptr<Chunk> MakeChunk(int chunkX, int chunkZ, std::shared_ptr<nbt::CompoundTag> const& root) {
        using namespace std;

        auto level = root->query("/Level")->asCompound();
        if (!level) {
            return nullptr;
        }

        int dataVersion = 0;
        auto dataVersionTag = root->query("/DataVersion")->asInt();
        if (dataVersionTag) {
            // *.mca created by Minecraft 1.2.1 does not have /DataVersion tag
            dataVersion = dataVersionTag->fValue;
        }

        vector<shared_ptr<nbt::CompoundTag>> tileEntities;
        auto tileEntitiesList = level->listTag("TileEntities");
        if (tileEntitiesList && tileEntitiesList->fType == nbt::Tag::TAG_Compound) {
            for (auto it : *tileEntitiesList) {
                auto comp = it->asCompound();
                if (!comp) {
                    continue;
                }
                auto copy = make_shared<nbt::CompoundTag>();
                copy->fValue = comp->fValue;
                tileEntities.push_back(copy);
            }
        }

        auto sectionsTag = level->listTag("Sections");
        if (!sectionsTag) {
            return nullptr;
        }
        vector<shared_ptr<ChunkSection>> sections;
        chunksection::ChunkSectionGenerator::MakeChunkSections(sectionsTag, dataVersion, chunkX, chunkZ, tileEntities, sections);

        vector<biomes::BiomeId> biomes;
        auto biomesTag = level->query("Biomes");
        ParseBiomes(biomesTag, biomes);

        vector<shared_ptr<nbt:: CompoundTag>> entities;
        auto entitiesTag = level->listTag("Entities");
        if (entitiesTag && entitiesTag->fType == nbt::Tag::TAG_Compound) {
            auto entitiesList = entitiesTag->asList();
            for (auto it : *entitiesList) {
                auto comp = it->asCompound();
                if (!comp) {
                    continue;
                }
                auto copy = make_shared<nbt::CompoundTag>();
                copy->fValue = comp->fValue;
                entities.push_back(copy);
            }
        }

        string s;
        auto status = level->stringTag("Status");
        if (status) {
            s = status->fValue;
        }
        std::optional<bool> terrianPopulated = level->boolean("TerrianPopulated");

        auto structures = level->compoundTag("Structures");

        return std::shared_ptr<Chunk>(new Chunk(root, chunkX, chunkZ, sections, dataVersion, biomes, entities, tileEntities, structures, s, terrianPopulated));
    }

    static std::shared_ptr<Chunk> LoadFromCompressedChunkNbtFile(std::string const& filePath, int chunkX, int chunkZ) {
        auto stream = std::make_shared<mcfile::stream::FileInputStream>(filePath);
        if (!stream->valid()) {
            return nullptr;
        }
        std::vector<uint8_t> buffer(stream->length());
        if (!stream->read(buffer.data(), 1, buffer.size())) {
            return nullptr;
        }
        if (!detail::Compression::decompress(buffer)) {
            return nullptr;
        }
        auto root = std::make_shared<mcfile::nbt::CompoundTag>();
        auto bs = std::make_shared<mcfile::stream::ByteStream>(buffer);
        stream::InputStreamReader reader(bs);
        root->read(reader);
        if (!root->valid()) {
            return nullptr;
        }
        return MakeChunk(chunkX, chunkZ, root);
    }

private:
    explicit Chunk(std::shared_ptr<nbt::CompoundTag> const& root,
                   int chunkX, int chunkZ,
                   std::vector<std::shared_ptr<ChunkSection>> const& sections,
                   int dataVersion,
                   std::vector<biomes::BiomeId> & biomes,
                   std::vector<std::shared_ptr<nbt::CompoundTag>> &entities,
                   std::vector<std::shared_ptr<nbt::CompoundTag>>& tileEntities,
                   std::shared_ptr<nbt::CompoundTag> const& structures,
                   std::string const& status,
                   std::optional<bool> terrianPopulated)
        : fRoot(root)
        , fChunkX(chunkX)
        , fChunkZ(chunkZ)
        , fDataVersion(dataVersion)
        , fStructures(structures)
        , fStatus(status)
        , fTerrianPopulated(terrianPopulated)
    {
        if (sections.empty()) {
            fMinChunkSectionY = 0;
        } else {
            std::optional<int> minChunkSectionY;
            std::optional<int> maxChunkSectionY;
            for (auto const& section : sections) {
                int const y = section->y();
                if (minChunkSectionY) {
                    minChunkSectionY = std::min(*minChunkSectionY, y);
                } else {
                    minChunkSectionY = y;
                }
                if (maxChunkSectionY) {
                    maxChunkSectionY = std::max(*maxChunkSectionY, y);
                } else {
                    maxChunkSectionY = y;
                }
            }
            fMinChunkSectionY = *minChunkSectionY;
            fSections.resize(*maxChunkSectionY - *minChunkSectionY + 1);
            for (auto const& section : sections) {
                int const y = section->y();
                int const idx = y - fMinChunkSectionY;
                fSections[idx] = section;
            }
        }
        fBiomes.swap(biomes);
        fEntities.swap(entities);
        fTileEntities.swap(tileEntities);
    }

    std::shared_ptr<ChunkSection> sectionAtBlock(int y) const {
        int const sectionIndex = y / 16;
        int const idx = sectionIndex - fMinChunkSectionY;
        if (idx < 0 || fSections.size() <= idx) {
            return nullptr;
        } else {
            return fSections[idx];
        }
    }

    static void ParseBiomes(nbt::Tag const* biomesTag, std::vector<biomes::BiomeId> &result) {
        if (!biomesTag) {
            return;
        }
        if (biomesTag->id() == nbt::Tag::TAG_Int_Array) {
            std::vector<int32_t> const& value = biomesTag->asIntArray()->value();
            size_t const size = value.size();
            if (size == 256 || size == 1024 || size == 1536) {
                result.resize(size);
                for (int i = 0; i < size; i++) {
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
    std::shared_ptr<mcfile::nbt::CompoundTag> fRoot;
    int const fChunkX;
    int const fChunkZ;
    std::vector<std::shared_ptr<ChunkSection>> fSections;
    std::vector<biomes::BiomeId> fBiomes;
    int const fDataVersion;
    std::vector<std::shared_ptr<nbt::CompoundTag>> fEntities;
    std::vector<std::shared_ptr<nbt::CompoundTag>> fTileEntities;
    std::shared_ptr<mcfile::nbt::CompoundTag> fStructures;

private:
    std::string const fStatus;
    std::optional<bool> fTerrianPopulated;
    int fMinChunkSectionY;
};

} // namespace mcfile
