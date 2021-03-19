#pragma once

namespace mcfile::chunksection {

class BlockStatesParser113 {
public:
    static void PaletteIndicesFromBlockStates(std::vector<int64_t> const& blockStates, std::vector<uint16_t> &paletteIndices) {
        paletteIndices.resize(16 * 16 * 16);
        size_t const numBits = blockStates.size() * 64;
        if (numBits % 4096 != 0) {
            return;
        }
        size_t const bitsPerIndex = numBits >> 12;
        if (64 % bitsPerIndex != 0) {
            int a =0 ;
        }
        size_t u64Index = 0;
        size_t bitIndex = 0;
        for (int i = 0; i < 4096; i++) {
            size_t bitIndex = bitsPerIndex * i;
            size_t u64Index = bitIndex / 64;
            size_t v0Bits = std::min(bitsPerIndex, (u64Index + 1) * 64 - bitIndex);
            size_t v0BitOffset = bitIndex - 64 * u64Index;
            size_t v1Bits = bitsPerIndex - v0Bits;
            uint64_t v0 = *(uint64_t*)(blockStates.data() + u64Index);
            uint64_t paletteIndex = (v0 >> v0BitOffset) & ((1 << v0Bits) - 1);
            if (v1Bits > 0) {
                uint64_t v1 = *(uint64_t*)(blockStates.data() + (u64Index + 1));
                uint64_t t = v1 & ((1 << v1Bits) - 1);
                paletteIndex = (t << v0Bits) | paletteIndex;
            }
            paletteIndices[i] = (uint16_t)(paletteIndex & 0xffff);
        }
    }

    static void BlockStatesFromPaletteIndices(size_t numPaletteEntries, std::vector<uint16_t> const& paletteIndices, std::vector<int64_t> &blockStates) {
        using namespace std;
        blockStates.clear();

        uint8_t bitsPerBlock;
        int blocksPerLong;
        if (numPaletteEntries <= 16) {
            bitsPerBlock = 4;
            blocksPerLong = 16;
        } else if (numPaletteEntries <= 32) {
            bitsPerBlock = 5;
            blocksPerLong = 12;
        } else if (numPaletteEntries <= 64) {
            bitsPerBlock = 6;
            blocksPerLong = 10;
        } else if (numPaletteEntries <= 128) {
            bitsPerBlock = 7;
            blocksPerLong = 9;
        } else if (numPaletteEntries <= 256) {
            bitsPerBlock = 8;
            blocksPerLong = 8;
        } else {
            bitsPerBlock = 16;
            blocksPerLong = 4;
        }
        vector<bool> bitset;
        bitset.reserve(bitsPerBlock * 4096);
        assert(paletteIndices.size() == 4096);
        for (int i = 0; i < 4096; i++) {
            uint16_t v = paletteIndices[i];
            for (int j = 0; j < bitsPerBlock; j++) {
                bitset.push_back((v & 1) == 1);
                v >>= 1;
            }
        }
        for (int i = 0; i < bitset.size(); i += 64) {
            uint64_t v = 0;
            for (int j = i; j < i + 64 && j < bitset.size(); j++) {
                uint64_t t = ((uint64_t)(bitset[j] ? 1 : 0)) << (j - i);
                v |= t;
            }
            blockStates.push_back(*(int64_t*)&v);
        }
    }

private:
    BlockStatesParser113() = delete;
};

}
