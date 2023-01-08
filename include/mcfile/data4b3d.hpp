#pragma once

namespace mcfile {

class Data4b3dView {
public:
    static std::optional<Data4b3dView> Make(Pos3i const &origin, uint32_t widthX, uint32_t height, uint32_t widthZ, std::vector<uint8_t> *ref) {
        using namespace std;
        if (!ref) {
            return nullopt;
        }
        auto size = widthX * height * widthZ;
        size = size / 2 + size % 2;
        if (ref->size() != size) {
            return nullopt;
        }
        Data4b3dView ret(origin, widthX, height, widthZ, ref);
        return ret;
    }

    uint8_t getUnchecked(Pos3i const &p) const {
        assert(fOrigin.fX <= p.fX && p.fX < fOrigin.fX + fWidthX);
        assert(fOrigin.fY <= p.fY && p.fY < fOrigin.fY + fHeight);
        assert(fOrigin.fZ <= p.fZ && p.fZ < fOrigin.fZ + fWidthZ);
        uint32_t dx = p.fX - fOrigin.fX;
        uint32_t dy = p.fY - fOrigin.fY;
        uint32_t dz = p.fZ - fOrigin.fZ;
        uint32_t index = (dy * fWidthZ + dz) * fWidthX + dx;
        uint32_t i = index / 2;
        uint32_t shift = (index - i * 2) * 4;
        return 0xf & ((*fRef)[i] >> shift);
    }

    void setUnchecked(Pos3i const &p, uint8_t v) {
        assert(fOrigin.fX <= p.fX && p.fX < fOrigin.fX + fWidthX);
        assert(fOrigin.fY <= p.fY && p.fY < fOrigin.fY + fHeight);
        assert(fOrigin.fZ <= p.fZ && p.fZ < fOrigin.fZ + fWidthZ);
        uint32_t dx = p.fX - fOrigin.fX;
        uint32_t dy = p.fY - fOrigin.fY;
        uint32_t dz = p.fZ - fOrigin.fZ;
        uint32_t index = (dy * fWidthZ + dz) * fWidthX + dx;
        uint32_t i = index / 2;
        uint32_t shift = (index - i * 2) * 4;
        uint8_t current = (*fRef)[i];
        (*fRef)[i] = (current & (uint8_t(0xf) << (4 - shift)) | ((v & 0xf) << shift));
    }

    template <class Container>
    void copy(Container &out) const {
        out.clear();
        out.reserve(fRef->size());
        std::copy(fRef->begin(), fRef->end(), std::back_inserter(out));
    }

    template <class Container>
    bool reset(Container const &data) {
        auto size = std::distance(data.begin(), data.end());
        if (size != fRef->size()) {
            return false;
        }
        std::copy(data.begin(), data.end(), fRef->begin());
        return true;
    }

protected:
    Data4b3dView(Pos3i origin, uint32_t widthX, uint32_t height, uint32_t widthZ, std::vector<uint8_t> *ref)
        : fOrigin(origin)
        , fWidthX(widthX)
        , fHeight(height)
        , fWidthZ(widthZ)
        , fRef(ref) {
    }

protected:
    Pos3i const fOrigin;
    uint32_t const fWidthX;
    uint32_t const fHeight;
    uint32_t const fWidthZ;
    std::vector<uint8_t> *fRef;
};

class Data4b3d : public Data4b3dView {
public:
    Data4b3d(Pos3i origin, uint32_t widthX, uint32_t height, uint32_t widthZ)
        : Data4b3dView(origin, widthX, height, widthZ, nullptr) {
        auto size = widthX * height * widthZ;
        size = size / 2 + size % 2;
        fStorage.resize(size);
        fRef = &fStorage;
    }

private:
    std::vector<uint8_t> fStorage;
};

} // namespace mcfile
