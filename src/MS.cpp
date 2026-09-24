#include "MS.h"

MS::MS() : AudioStream(2, inputQueueArray) {}

void MS::update(){
    // Get stereo inputs
    audio_block_t* block_L = receiveWritable(0); 
    audio_block_t* block_R = receiveWritable(1); 

    // Init intermediate arrays to store converted floats
    float32_t input_L[BLOCK_SIZE];
    float32_t input_R[BLOCK_SIZE];

    // Init output arrays to store processed data
    // See below for ideas to make this more space and time efficient
    float32_t output_L[BLOCK_SIZE];
    float32_t output_R[BLOCK_SIZE];

    if (!block_L || !block_R) return;

    arm_q15_to_float(block_L->data, input_L, BLOCK_SIZE);
    arm_q15_to_float(block_R->data, input_R, BLOCK_SIZE);

    for (int i = 0; i < BLOCK_SIZE; i++){
        float32_t mid = (input_L[i] + input_R[i]) / 2;  
        float32_t side = (input_L[i] - input_R[i]) / 2;

        // Scale mid and side channels
        side *= sideGain;
        mid *= midGain;

        // Try to optimize this more later by doing the conversion + transmission 
        // in one statement, okay for now but fix ASAP, need overhead
        output_L[i] = mid + side;
        output_R[i] = mid - side;
    }

    arm_float_to_q15(output_L, block_L->data, BLOCK_SIZE);
    arm_float_to_q15(output_R, block_R->data, BLOCK_SIZE);

    transmit(block_L, 0);
    transmit(block_R, 1);

    release(block_L);
    release(block_R);
}

void MS::setMidGain(float32_t gain){
    gain > 5 ? midGain = 5 : midGain = gain;
}

void MS::setSideGain(float32_t gain){
    gain > 5 ? sideGain = 5 : sideGain = gain;
}

void MS::setMidLPF(float32_t f0){
    // Might just want to hard code this into the private variable?
}

void MS::setSideHPF(float32_t f0){

}

void  MS::toggleSides(){
    if (!sideMuted){
        sideGainState = sideGain;
        sideGain = 0;
    }
    else {
        sideGain = sideGainState;
    }
}

void MS::toggleMid(){
    if (!midMuted){
        midGainState = midGain;
        midGain = 0;
    }
    else {
        midGain = midGainState;
    }
}

void MS::init(){
    // Initialize them separately, the channels are going through different filters so
    // There needs to be two filters not one, cascaded biquad
    arm_biquad_cascade_df1_init_f32(&inst_LPF, 1, coeffs_LPF, state_LPF);
    arm_biquad_cascade_df1_init_f32(&inst_HPF, 1, coeffs_HPF, state_HPF);

    // Mid side volume init
    midGain = 1.0f;
    sideGain = 1.0f;
    midGainState = midGain;
    sideGainState = sideGain;
    midMuted = false;
    sideMuted = false;
}
