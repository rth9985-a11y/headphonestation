#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <usb_audio.h>
#include "CrossTalk.h"
#include "ParametricEQ.h"
#include "MS.h"

// Define all potentiometer pins (all analog inputs)
#define MASTER_VOLUME_POT 26
#define LOW_SHELF_GAIN_POT 39
#define HIGH_SHELF_GAIN_POT 41 
#define PEAKING_EQ_GAIN_POT 40      
#define PEAKING_EQ_FREQ_POT 25 
#define PEAKING_EQ_Q_POT 24        
#define MID_MUTE_BUTTON 0
#define SIDE_MUTE_BUTTON 1
#define MID_GAIN_POT 38
#define SIDE_GAIN_POT 14

// Instantiate Audio Objects 
AudioInputUSB usbIn;
AudioOutputI2S headphoneOut;
AudioControlSGTL5000 codec;
ParametricEQ mainEqL;
ParametricEQ mainEqR;
MS midSide;

AudioConnection patchCord1(usbIn, 0, mainEqL, 0);
AudioConnection patchCord2(usbIn, 1, mainEqR, 0);
AudioConnection patchCord3(mainEqL, 0, midSide, 0);
AudioConnection patchCord4(mainEqR, 0, midSide, 1);
AudioConnection patchCord5(midSide, 0, headphoneOut, 0);
AudioConnection patchCord6(midSide, 1, headphoneOut, 1);

// Represents a single pot, used for reading and mapping a pots raw value
struct eqPot{
  int8_t pin;
  float32_t prevReading;
  // Mapping
  float32_t scale;
  float32_t offset;

  // void (*updateFunction)(int, float32_t, float32_t, float32_t); // function to update
  static constexpr float32_t DEADBAND = 0.30f; // This works, CHANGE TO A DIFFERENT VALUE FOR EVERY POT
};

eqPot EQPots[] = { 
    {.pin = LOW_SHELF_GAIN_POT, .prevReading = 0.0f, .scale = 40.0f/1023.0f, .offset = -20.0f},
    {.pin = HIGH_SHELF_GAIN_POT, .prevReading =  0.0f, .scale = 40.0f/1023.0f, .offset = -20.0f},
    {.pin = PEAKING_EQ_GAIN_POT, .prevReading =  0.0f, .scale = 40.0f/1023.0f, .offset = -20.0f},
    {.pin = PEAKING_EQ_FREQ_POT, .prevReading =  0.0f, .scale = 9750.0f/1023.0f, .offset = 250.0f},
    {.pin = PEAKING_EQ_Q_POT, .prevReading =  0.0f, .scale = 9.0f/1023.0f, .offset = 1.0f},
    {.pin = MASTER_VOLUME_POT, .prevReading =  0.0f, .scale = 1.0f/1023.0f, .offset = 0.0f}
};

// Function definitions at top
float32_t readAndNormalizePot(eqPot& p);
void updateLowShelf(int stageIndex, float32_t f0, float32_t gain, float32_t q);
void updateHighShelf(int stageIndex, float32_t f0, float32_t gain, float32_t q);
void updatePeakingEQ(int stageIndex, float32_t f0, float32_t gain, float32_t q);
float32_t readAndScalePot_f32(int pin);

void setup(){
  Serial.begin(300);
  delay(500);
  Serial.println("BOOT");

  AudioMemory(35); 
  codec.enable();
  pinMode(MASTER_VOLUME_POT, INPUT); 
  codec.volume(0.75);

  mainEqL.parametricEQInit();
  mainEqR.parametricEQInit();
  midSide.init();

  // updatePeakingEQ(0, 500.0f, 0.0f, 0.0f);
 
}
 
/*  
All potentiometer global variables
*/
// EQ pots
float32_t lowshelfGain = 1.0f;
float32_t highshelfGain = 1.0f;
float32_t peakingGain = 1.0f;
float32_t peakingFrequency = 3000.0f;
float32_t peakingQ = 2.0f;
// MS pots
float32_t midGain = 1.0f;
float32_t sideGain = 1.0f;
// Master volume pot
float32_t codecGain = 0.7f;

// Update all EQ parameters and read interval
unsigned long prevMS = 0;
uint16_t updateInterval = 8; 


void loop(){
  unsigned long currentMS = millis();
  
  if (currentMS - prevMS >= updateInterval){

    midGain = readAndScalePot_f32(MID_GAIN_POT);
    sideGain = readAndScalePot_f32(SIDE_GAIN_POT);

    midSide.setMidGain(midGain);
    midSide.setSideGain(sideGain);

  }

  // Update EQ pots if they move, always polling though... is there a way to configure interrupts for this?
  for (auto& p : EQPots){

    float32_t rawReading = readAndNormalizePot(p);

    if (fabsf(rawReading - p.prevReading) >= p.DEADBAND){
      p.prevReading = rawReading;

      // Find a better way to deploy an update function for each pots
      // I don't like passing a pointer to the function, but maybe now that there are global updates it will work?
      if (p.pin == LOW_SHELF_GAIN_POT){
        lowshelfGain = rawReading;
        updateLowShelf(0, 60.0f, lowshelfGain, 0.707);
      }
      if (p.pin == HIGH_SHELF_GAIN_POT){
        highshelfGain = rawReading;
        updateHighShelf(0, 8000.0f, highshelfGain, 0.707);
      }
      if (p.pin == PEAKING_EQ_GAIN_POT){
        peakingGain = rawReading;
        updatePeakingEQ(0, 100.0f, rawReading, peakingQ);
      }
      if (p.pin == PEAKING_EQ_FREQ_POT){
        peakingFrequency = rawReading;
        updatePeakingEQ(0, peakingFrequency, peakingGain, peakingQ);
      }
      if (p.pin == PEAKING_EQ_Q_POT){
        peakingQ = rawReading;
        updatePeakingEQ(0, peakingFrequency, peakingGain, peakingQ);
      }
      if (p.pin == MASTER_VOLUME_POT){
        codecGain = rawReading;
        codec.volume(codecGain);
      }
    }
  }


}

/*
Helper functions
*/
float32_t readAndScalePot_f32(int pin){
  return (float32_t) analogRead(pin) * 0.00488758553f;
}

float32_t readAndNormalizePot(eqPot& p){
  return (float32_t) analogRead(p.pin) * p.scale + p.offset;
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