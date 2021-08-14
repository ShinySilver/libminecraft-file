#pragma once

namespace mcfile {
namespace detail {

template <class T>
class Pos2 {
public:
    Pos2(T x, T z) : fX(x), fZ(z) {}

    bool operator==(Pos2<T> const& other) const { return fX == other.fX && fZ == other.fZ; }

    static double DistanceSquare(Pos2<T> const& a, Pos2<T> const& b) {
        double dx = a.fX - b.fX;
        double dz = a.fZ - b.fZ;
        return dx * dx + dz * dz;
    }

public:
    T fX;
    T fZ;
};

template <class T>
class Pos2Hasher {
public:
    size_t operator()(Pos2<T> const& k) const {
        size_t res = 17;
        res = res * 31 + std::hash<T>{}(k.fX);
        res = res * 31 + std::hash<T>{}(k.fZ);
        return res;
    }
};

}

using Pos2i = detail::Pos2<int>;
using Pos2iHasher = detail::Pos2Hasher<int>;

}
