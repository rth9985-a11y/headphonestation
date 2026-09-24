#pragma once
#include <Audio.h>
#include "arm_math.h"
#include <cstdlib>

#define NUM_STAGES 3
#define BLOCK_SIZE 128

class ParametricEQ : public AudioStream {
public:
  ParametricEQ();
  void update(void) override;

  //Init (set CMSIS DSP biqaud parameters)
  void parametricEQInit();

  void setHighShelf(int stage_index, float32_t gainDb, float32_t f0, float32_t q);
  void setPeaking(int stage_index, float32_t gainDb, float32_t f0, float32_t q);
  void setLowShelf(int stage_index, float32_t gainDb, float32_t f0, float32_t q);

private:
  audio_block_t *inputQueueArray[1];
  arm_biquad_casd_df1_inst_f32 inst;

  // Coefficients are read left to right in the difference equation and are entered as such in the "coeffs" array 
  // Ensure that post normalization by a0, the final two (a1, a2) are NEGATED, CMSIS-DSP wants this because that just
  //    how MAC works!
  float32_t coeffs[5 * NUM_STAGES] = {};
  float32_t states[4 * NUM_STAGES] = {}; 
};