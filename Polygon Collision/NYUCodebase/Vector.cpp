//
//  Vector.cpp
//  NYUCodebase
//
//  Created by Shayaan Saiyed on 4/6/17.
//  Copyright © 2017 Ivan Safrin. All rights reserved.
//

#include <math.h>
#include "Vector.hpp"

Vector::Vector(){};

Vector::Vector(float x, float y, float z) : x(x), y(y), z(z) {};

float Vector::length() const {
    return sqrtf(x*x + y*y + z*z);
};

