//
//  Vector.hpp
//  NYUCodebase
//
//  Created by Shayaan Saiyed on 4/6/17.
//  Copyright © 2017 Ivan Safrin. All rights reserved.
//

#ifndef VECTOR
#define VECTOR

class Vector {
public:
    Vector (float x, float y, float z);
    Vector();
    float x; //= 0.0;
    float y; //= 0.0;
    float z; //= 0.0;
    //float a = 1.0;
    float length() const;
};

#endif
