#pragma once

class Coord2D {
public:
    float x, y; //should be fine because the real coordinates should between 0~2047
    static int32_t dim;

    Coord2D(float x0, float y0) {
        x = x0;
        y = y0;
    }

    Coord2D() { x = y = 0; }
};


class Coord3D {
public:
    float x, y, z; //should be fine because the real coordinates should between 0~2047
    static int32_t dim;

    Coord3D(float x0, float y0, float z0) {
        x = x0;
        y = y0;
        z = z0;
    }

    Coord3D() { x = y = z = 0; }
};
