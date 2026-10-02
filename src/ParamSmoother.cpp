#include "ParamSmoother.h"

void ParamSmoother::setAlpha(float32_t a){
    alpha = a;
}

float32_t ParamSmoother::process(float32_t target){
    state = (alpha * target) + ((1 - alpha) * state);
    return state;
}