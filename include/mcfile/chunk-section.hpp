#pragma once

namespace mcfile {

class ChunkSection {
public:
    virtual ~ChunkSection() {}
    virtual std::shared_ptr<Block const> blockAt(int offsetX, int offsetY, int offsetZ) const = 0;
    virtual uint8_t blockLightAt(int offsetX, int offsetY, int offsetZ) const = 0;
    virtual uint8_t skyLightAt(int offsetX, int offsetY, int offsetZ) const = 0;
    virtual blocks::BlockId blockIdAt(int offsetX, int offsetY, int offsetZ) const = 0;
    virtual bool blockIdWithYOffset(int offsetY, std::function<bool(int offsetX, int offsetZ, blocks::BlockId blockId)> callback) const = 0;
    virtual int y() const = 0;
    virtual int rawY() const = 0;
    virtual std::vector<std::shared_ptr<Block const>> const& palette() const = 0;
    virtual std::optional<size_t> paletteIndexAt(int offsetX, int offsetY, int offsetZ) const = 0;

    // optional

    virtual bool setBlockAt(int offsetX, int offsetY, int offsetZ, std::shared_ptr<Block const> const& block) {
        return false;
    }
    
    virtual std::shared_ptr<mcfile::nbt::CompoundTag> toCompoundTag() const {
        return nullptr;
    }
};

} // namespace mcfile
