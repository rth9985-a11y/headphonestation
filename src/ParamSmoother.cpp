#include "ParamSmoother.h"

void ParamSmoother::setAlpha(float32_t a){
    alpha = a;
}

float32_t ParamSmoother::process(float32_t target){
    return state + 0.5 * (target - state);
}