#pragma once

#include "svg.h"
//std library
#include <cmath>
#include <algorithm>

namespace catalogue {
    namespace geo {
        struct Coordinates {
            double lat;
            double lng;
            bool operator==(const Coordinates& other) const {
                return lat == other.lat && lng == other.lng;
            }
        };

        inline bool operator!=(const Coordinates& lhs, const Coordinates& rhs) {
            return !(lhs == rhs);
        }

        double ComputeDistance(Coordinates from, Coordinates to);

    } //namespace geo
} //namespace catalogue
