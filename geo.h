#pragma once
//
struct Coordinates {
    double lat;
    double lng;
};

double ComputeDistance(Coordinates from, Coordinates to);
bool Coordinates_equial(const Coordinates& from, const Coordinates& to);
