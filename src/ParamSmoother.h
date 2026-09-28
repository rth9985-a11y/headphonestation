
#pragma once
#include "arm_math.h"
class ParamSmoother {
public:
    void setAlpha(float32_t a); 
    float32_t process(float32_t target);

private:
    float32_t alpha = 0.1f;
    float32_t state = 0.0f;

};