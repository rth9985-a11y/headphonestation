#include "ParametricEQ.h"
#include <numbers>
#include <cmath>

ParametricEQ::ParametricEQ() : AudioStream(1, inputQueueArray) {}

void ParametricEQ::update() {

  // These arrays will decay to pointers naturally so no need to declare them as pointers
  float32_t float32_input[BLOCK_SIZE];
  float32_t float32_output[BLOCK_SIZE];

  // Get audio samples into the system, check if buffer fills
  audio_block_t* block = receiveWritable(0);
  if (!block) return;

  // Convert to 32 bit floating point
  arm_q15_to_float(block->data, float32_input, BLOCK_SIZE);

  // Process the audio using ARM DSP library
  arm_biquad_cascade_df1_f32(&inst, float32_input, float32_output, BLOCK_SIZE);
  
  // Convert back to int16_t types
  arm_float_to_q15(float32_output, block->data, BLOCK_SIZE);

  // Give the processed block back to Audio Library's output driver and the DAC
  transmit(block, 0);
  release(block);

}

void ParametricEQ::parametricEQInit(){
  arm_biquad_cascade_df1_init_f32(&inst, NUM_STAGES, coeffs, states);
}


/*
@brief  Calculate and update IIR filter coefficients for  high-shelf filter
        filter coefficients are updated in the private coeffs[] member array 
        this is standard for use with the ARM CMSIS-DSP "cascaded filter" function 

@param  stage_index:  All coefficients for all filter stages belong in coeffs[] 
                      enter the stage number of this biquad section
@param  gainDb:       gain at f0 (center/cutoff frequency)
@param  f0:           center frequency or filter cutoff depending on the filter type
@param  q:            quality factor or steepness of filter approaching f0
*/
void ParametricEQ::setHighShelf(int stage_index, float32_t gainDb, float32_t f0, float32_t q) {

  float32_t A = powf(10.f, gainDb / 40.0f); 
  float32_t w0 = 2 * PI * (f0 / 44100);
  float32_t alpha = sin(w0) / (2 * q);

  // Compute once per function call cos(w0), sin(w0), and sqrt(w0)
  float32_t cos_w0 = arm_cos_f32(w0);
  float32_t sqrt_A;
  arm_status status;
  status = arm_sqrt_f32(A, &sqrt_A); // Using the CMSIS-DSP library for more overhead

  //Coefficient calculations according to the RBJ cookbook formula analog prototypes
  float32_t b0 =      A * ((A + 1) + (A - 1) * cos_w0 + 2 * sqrt_A * alpha);
  float32_t b1 = -2 * A * ((A - 1) + (A + 1) * cos_w0);
  float32_t b2 =      A * ((A + 1) + (A - 1) * cos_w0 - 2 * sqrt_A * alpha);
  float32_t a0 =          (A +1 ) - (A - 1) * cos_w0 + 2 * sqrt_A * alpha;
  float32_t a1 =      2 * ((A - 1) - (A + 1) * cos_w0);
  float32_t a2 =          (A + 1) - (A - 1) * cos_w0 - 2 * sqrt_A * alpha;

  // Normalize and negate
  float32_t c0 = b0 / a0;
  float32_t c1 = b1 / a0;
  float32_t c2 = b2 / a0;
  float32_t c3 = - a1 / a0;
  float32_t c4 = - a2 / a0;

  __disable_irq();
  coeffs[5 * stage_index + 0] = c0;
  coeffs[5 * stage_index + 1] = c1;
  coeffs[5 * stage_index + 2] = c2;
  coeffs[5 * stage_index + 3] = c3;
  coeffs[5 * stage_index + 4] = c4;
  __enable_irq();

}

void ParametricEQ::setLowShelf(int stage_index, float32_t gainDb, float32_t f0, float32_t q){
  float32_t A = powf(10.f, gainDb / 40.0f);
  float32_t w0 = 2 * PI * (f0 / 44100);
  float32_t alpha = sin(w0) / (2 * q);

  // Compute once per function call cos(w0), sin(w0), and sqrt(w0)
  float32_t cos_w0 = arm_cos_f32(w0);
  float32_t sqrt_A;
  arm_status status;
  status = arm_sqrt_f32(A, &sqrt_A); // Using the CMSIS-DSP library for more overhead

  //Coefficient calculations according to the RBJ cookbook formula analog prototypes
  float32_t b0 =     A * ((A + 1) - (A - 1) * cos_w0 + 2 * sqrt_A * alpha);
  float32_t b1 = 2 * A * ((A - 1) - (A + 1) * cos_w0);
  float32_t b2 =     A * ((A + 1) - (A - 1) * cos_w0 - 2 * sqrt_A * alpha);
  float32_t a0 =          (A + 1) + (A - 1) * cos_w0 + 2 * sqrt_A * alpha;
  float32_t a1 =   - 2 * ((A - 1) + (A + 1) * cos_w0);
  float32_t a2 =          (A + 1) + (A - 1) * cos_w0 - 2 * sqrt_A * alpha;
  
  // Normalize and negate
  float32_t c0 = b0 / a0;
  float32_t c1 = b1 / a0;
  float32_t c2 = b2 / a0;
  float32_t c3 = - a1 / a0;
  float32_t c4 = - a2 / a0;

  __disable_irq();
  coeffs[5 * stage_index + 0] = c0;
  coeffs[5 * stage_index + 1] = c1;
  coeffs[5 * stage_index + 2] = c2;
  coeffs[5 * stage_index + 3] = c3;
  coeffs[5 * stage_index + 4] = c4;

  __enable_irq();
}

void ParametricEQ::setPeaking(int stage_index, float32_t gainDb, float32_t f0, float32_t q){
  float32_t A = powf(10.0f, gainDb / 40.0f);
  float32_t w0 = 2 * PI * (f0 / 44100);
  float32_t alpha = arm_sin_f32(w0) / (2 * q);

  float32_t cos_w0 = arm_cos_f32(w0);

  float32_t b0 =     1 + alpha * A;
  float32_t b1 =    -2 * cos_w0;
  float32_t b2 =     1 - alpha * A;
  float32_t a0 =     1 + alpha / A;
  float32_t a1 =    -2 * cos_w0;
  float32_t a2 =     1 - alpha / A;

  float32_t c0 = b0 / a0;
  float32_t c1 = b1 / a0;
  float32_t c2 = b2 / a0;
  float32_t c3 = - a1 / a0;
  float32_t c4 = - a2 / a0;

  __disable_irq();
  coeffs[5 * stage_index + 0] = c0;
  coeffs[5 * stage_index + 1] = c1;
  coeffs[5 * stage_index + 2] = c2;
  coeffs[5 * stage_index + 3] = c3;
  coeffs[5 * stage_index + 4] = c4;
  __enable_irq();
  
}