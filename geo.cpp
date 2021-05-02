#define _USE_MATH_DEFINES

#include "geo.h"

#include <cstdlib>
#include <cmath>

bool Coordinates_equial(const Coordinates& from, const Coordinates& to) {
    static constexpr double eps = 1E-10;
    return (std::abs(from.lat - to.lat) < eps) && (abs(from.lng - to.lng) < eps);
}

double ComputeDistance(Coordinates from, Coordinates to) {
    using namespace std;
    static constexpr double dr = M_PI / 180.0;
    if (Coordinates_equial(from, to)) {
        return 0.0;
    }
    return acos(sin(from.lat * dr) * sin(to.lat * dr)
        + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
        * 6371000;
}

