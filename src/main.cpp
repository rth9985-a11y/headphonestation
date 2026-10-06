#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <usb_audio.h>
#include "CrossTalk.h"
#include "ParametricEQ.h"
#include "MS.h"

// POT DEFINES
#define GAIN_POT 38
#define LOW_SHELF_GAIN_POT 39
#define PEAKING_EQ_CONTROL_POT 40
#define HIGH_SHELF_GAIN_POT 41

// BUTTON DEFINES
#define PEAKING_EQ_TOGGLE_BUTTON 29
#define GAIN_TOGGLE_BUTTON 33


/* ---------- CLASS INSTANTIATIONS ----------*/
// Instantiate Audio Objects (this is where all of the custom DSP is too)
AudioInputUSB usbIn;
AudioOutputI2S headphoneOut;
AudioControlSGTL5000 codec;
ParametricEQ mainEqL;
ParametricEQ mainEqR;
MS midSide;

// Gain smoother object instantiations for gains
ParamSmoother master_gain_smoother;
ParamSmoother side_gain_smoother;
ParamSmoother mid_gain_smoother;

// Patching audio objects
AudioConnection patchCord1(usbIn, 0, mainEqL, 0);
AudioConnection patchCord2(usbIn, 1, mainEqR, 0);
AudioConnection patchCord3(mainEqL, 0, midSide, 0);
AudioConnection patchCord4(mainEqR, 0, midSide, 1);
AudioConnection patchCord5(midSide, 0, headphoneOut, 0);
AudioConnection patchCord6(midSide, 1, headphoneOut, 1);

/* ---------- FUNCTION FORWARD DEFINITIONS ---------- */
void updateLowShelf(int stageIndex, float32_t f0, float32_t gain, float32_t q);
void updateHighShelf(int stageIndex, float32_t f0, float32_t gain, float32_t q);
void updatePeakingEQ(int stageIndex, float32_t f0, float32_t gain, float32_t q);
float32_t readAndScalePot_f32(int pin);
float32_t readAndScale(int pin, float32_t upper);
void toggle_button_handler(int button, int *gain_toggle_index, unsigned long main_millis, unsigned long* prev_millis, int* gain_toggle_prev);
void gain_toggle_handler(int index);

void setup(){
  // SERIAL SETUP
  Serial.begin(300);
  delay(500);
  Serial.println("BOOT");

  // AUDIO LIBRARY (Init stuff)
  AudioMemory(100); 
  codec.enable();
  codec.volume(0.75);

  // CONFIGURE I/O DDIR (Arduino HAL)
  pinMode(GAIN_POT, INPUT); 
  pinMode(LOW_SHELF_GAIN_POT, INPUT);
  pinMode(PEAKING_EQ_CONTROL_POT, INPUT);
  pinMode(HIGH_SHELF_GAIN_POT, INPUT);

  // Custom audio object inits
  mainEqL.parametricEQInit();
  mainEqR.parametricEQInit();
  midSide.init();

  // Set smoothing coefficients for gain pot
  master_gain_smoother.setAlpha(0.5f);
  side_gain_smoother.setAlpha(0.5f);
  mid_gain_smoother.setAlpha(0.5f);
}
 
/* ---------- GLOBAL VARIABLES ---------- */
// EQ Macros
float32_t lowshelfGain = 1.0f;
float32_t highshelfGain = 1.0f;
float32_t peakingGain = 1.0f;
float32_t peakingFrequency = 3000.0f;
float32_t peakingQ = 2.0f;

// MS toggleable pots
float32_t midGain = 1.0f;
float32_t sideGain = 1.0f;
float32_t codecGain = 0.7f;

// PEAKING EQ TOGGLE (states and index)

// GAIN TOGGLE (states and index)
int gain_toggle_idx = 0;
int gain_toggle_prev_button = 0;
unsigned long gain_toggle_prev_millis = millis();
unsigned long currentMS = millis();

// PRINT TIMING VARS
unsigned long prevMS = 0;
uint16_t updateInterval = 20;

void loop(){
  currentMS = millis();
  
  // GAIN TOGGLE HANDLING AND UPDATE
  toggle_button_handler(GAIN_TOGGLE_BUTTON, &gain_toggle_idx, currentMS, &gain_toggle_prev_millis, &gain_toggle_prev_button);
  gain_toggle_handler(gain_toggle_idx);
  midSide.setMidGain(midGain);
  midSide.setSideGain(sideGain);
  codec.volume(codecGain);

  // PEAKING EQ TOGGLE HANDLING AND UPDATE -------------------------------- TODO


  // DEBUG PRINTING
  if (currentMS - prevMS >= updateInterval){
    Serial.printf("Gain IDX: %d, Mid Gain: %.2f, Side Gain: %.2f, Master Volume: %.2f\n", 
                  midGain, sideGain, codecGain);
    prevMS = currentMS;
  }
}



// ---------- HELPER FUNCTIONS ----------

// Scaling methods
float32_t readAndScalePot_f32(int pin){
  return (float32_t) analogRead(pin) * 0.0009775171 - 0.25f;
}

float32_t readAndScale(int pin, float32_t upper){
  return (float32_t) analogRead(pin) * (upper/1023.0f);
}

void updateLowShelf(int stageIndex, float32_t f0, float32_t gain, float32_t q){
  mainEqL.setLowShelf(0, gain, f0, q);
  mainEqR.setLowShelf(0, gain, f0, q);
}

void updateHighShelf(int stageIndex, float32_t f0, float32_t gain, float32_t q){
  mainEqL.setHighShelf(2, gain, f0, q);
  mainEqR.setHighShelf(2, gain, f0, q);
}

void updatePeakingEQ(int stageIndex, float32_t f0, float32_t gain, float32_t q){
  mainEqL.setPeaking(1, gain, f0, q);
  mainEqR.setPeaking(1, gain, f0, q);
}

// Analog Reading methods
void toggle_button_handler(int button, int *gain_toggle_index, unsigned long main_millis, 
                            unsigned long* prev_millis, int* gain_toggle_prev)
  {
  int reading = digitalRead(button);
  if (main_millis - *prev_millis >= 20){
    if (reading == 1 && gain_toggle_prev == 0) {
      *gain_toggle_index = ((*gain_toggle_index) + 1) % 3;
    }
    *prev_millis = main_millis;
    *gain_toggle_prev = 1;
  }
}

void gain_toggle_handler(int index){
  if (index == 0){
    codecGain = master_gain_smoother.process(readAndScale(GAIN_POT, 0.75));
  }
  if (index == 1){
    midGain = mid_gain_smoother.process(readAndScale(GAIN_POT, 0.75f));
  }
  if (index == 2){
    sideGain = side_gain_smoother.process(readAndScale(GAIN_POT, 0.75f));
  }
} 

