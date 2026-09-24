#include "arm_math.h"
#include <Audio.h>

#define BLOCK_SIZE 128

class MS : public AudioStream {
public:
    MS();
    void update(void) override;
    // initialize the biquad cascade structure
    void init();
    void setSideGain(float32_t gain);
    void setMidGain(float32_t gain);
    void setMidLPF(float32_t f0);
    void setSideHPF(float32_t f0);
    void toggleSides();
    void toggleMid();

private:
    audio_block_t *inputQueueArray[2];
    // Gain for M/S
    float32_t sideGain;
    float32_t midGain;

    // Mute variables
    bool midMuted;
    bool sideMuted;

    // Gain state variables to handle the muting function
    float32_t midGainState; 
    float32_t sideGainState;

    // use arm bi-quad structure?
    arm_biquad_casd_df1_inst_f32 inst_LPF;
    arm_biquad_casd_df1_inst_f32 inst_HPF;

    // Coefficient arrays
    float32_t coeffs_LPF[5] = {};
    float32_t coeffs_HPF[5] = {};

    // State arrays 
    float32_t state_LPF[4] = {};
    float32_t state_HPF[4] = {};

};
