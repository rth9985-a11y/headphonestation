#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <usb_audio.h>
#include "CrossTalk.h"
#include "ParametricEQ.h"
#include "MS.h"
#include "teensy_pins.h"

/*
TOGGLE_BUTTON
button_pin: Button pin to read from
toggle_index: The index of the mode
current_ms: The current time to compare to prev (rate limiting)
prev_millis: time of the last button reading    (rate limiting)
prev_button_state: State of the last reading 
*/
typedef struct {
  int button_pin;
  int toggle_index;
  unsigned long current_ms;
  unsigned long prev_millis;
  int prev_button_state;
}TOGGLE_BUTTON;

/*
PICKUP
stored_raw[3]: Hold current values of all modes on one potentiometer
last_raw: last raw reading of current mode
locked: Is current mode locked?
*/
typedef struct {
  int  stored_raw[3];   // current values of each mode 
  int  last_raw;
  bool locked;
} PICKUP;

// Button structs
TOGGLE_BUTTON peaking_eq_toggle_button_struct =   {PEAKING_EQ_TOGGLE_BUTTON, 0, 0, 0, 0};
TOGGLE_BUTTON gain_toggle_button_struct       =   {GAIN_TOGGLE_BUTTON, 0, 0, 0, 0};

// POT LOCKING
PICKUP gain_pot_pickup =  {{955, 1023, 1023}, 0, false};   // master, mid, side
PICKUP eq_pot_pickup   =  {{554, 153, 162}, 0, false};   // gain, freq, Q

/* ---------- CLASS INSTANTIATIONS ----------*/
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
ParamSmoother peaking_gain_smoother;
ParamSmoother peaking_freq_smoother;
ParamSmoother peaking_q_smoother;

// Patching audio objects
AudioConnection patchCord1(usbIn, 0, mainEqL, 0);
AudioConnection patchCord2(usbIn, 1, mainEqR, 0);
AudioConnection patchCord3(mainEqL, 0, midSide, 0);
AudioConnection patchCord4(mainEqR, 0, midSide, 1);
AudioConnection patchCord5(midSide, 0, headphoneOut, 0);
AudioConnection patchCord6(midSide, 1, headphoneOut, 1);

/* ---------- FUNCTION FORWARD DEFINITIONS ---------- */
void updateLowShelf(float32_t f0, float32_t gain, float32_t q);
void updateHighShelf(float32_t f0, float32_t gain, float32_t q);
void updatePeakingEQ(float32_t f0, float32_t gain, float32_t q);
float32_t readAndScale(int pin, float32_t upper, float32_t offset);
void toggle_button_handler(TOGGLE_BUTTON* b);
void gain_toggle_handler(int index);
void peaking_eq_toggle_handler(int index);
bool pickup_update(PICKUP* p, int idx, int raw);

void setup(){
  // SERIAL SETUP
  Serial.begin(300);
  delay(500);
  Serial.println("BOOT");
  delay(500);

  // AUDIO LIBRARY (Init stuff)
  AudioMemory(100); 
  codec.enable();
  codec.volume(0.40f);

  // CONFIGURE I/O DDIR (Arduino HAL)
  pinMode(GAIN_POT, INPUT); 
  pinMode(LOW_SHELF_GAIN_POT, INPUT);
  pinMode(PEAKING_EQ_CONTROL_POT, INPUT);
  pinMode(HIGH_SHELF_GAIN_POT, INPUT);
  pinMode(PEAKING_EQ_TOGGLE_BUTTON, INPUT_PULLDOWN);
  pinMode(GAIN_TOGGLE_BUTTON, INPUT_PULLDOWN);

  // Custom audio object inits
  mainEqL.parametricEQInit();
  mainEqR.parametricEQInit();
  midSide.init();

  // Set smoothing coefficients for gain pot
  master_gain_smoother.setAlpha(SMOOTHING_COEFF);
  side_gain_smoother.setAlpha(SMOOTHING_COEFF);
  mid_gain_smoother.setAlpha(SMOOTHING_COEFF);
  peaking_gain_smoother.setAlpha(SMOOTHING_COEFF);
  peaking_q_smoother.setAlpha(SMOOTHING_COEFF);
  peaking_freq_smoother.setAlpha(SMOOTHING_COEFF);
}
 
/* ---------- GLOBAL VARIABLES ---------- */
// EQ Macros
float32_t lowshelfGain = 0.0f;
float32_t highshelfGain = 0.0f;
float32_t peakingGain = 0.0f;
float32_t peakingFrequency = 3500.0f;
float32_t peakingQ = 2.5f;

// MS toggleable pots
float32_t midGain = 0.75f;
float32_t sideGain = 0.75f;
float32_t codecGain = 0.45f;

// PRINT DEBUG RATE  LIMITING
unsigned long prevMS = 0;
const uint16_t updateInterval = 20;

// EQ UPDATE RATE LIMITING
unsigned long prev_eq_update = millis();
const uint16_t eq_update_interval = 15; // milliseconds

// "right now" time
unsigned long currentMS;

void loop(){
  currentMS = millis();
  
  // GAIN TOGGLE HANDLING AND UPDATE
  gain_toggle_button_struct.current_ms = currentMS;
  toggle_button_handler(&gain_toggle_button_struct);

  // PEAKING EQ TOGGLE HANDLING AND UPDATE  
  peaking_eq_toggle_button_struct.current_ms = currentMS;
  toggle_button_handler(&peaking_eq_toggle_button_struct);

	// LOCK POTS WHEN THEIR MODE CHANGES
  	static int prev_gain_idx = 0;
  	static int prev_eq_idx   = 0;

  	if (gain_toggle_button_struct.toggle_index != prev_gain_idx) {
    	gain_pot_pickup.locked = true;
    	prev_gain_idx = gain_toggle_button_struct.toggle_index;
  	}
  	if (peaking_eq_toggle_button_struct.toggle_index != prev_eq_idx) {
    	eq_pot_pickup.locked = true;
    	prev_eq_idx = peaking_eq_toggle_button_struct.toggle_index;
  	}

  // UPDATE GAINS AND EQ (rate limited to 150Hz)
  if (currentMS - prev_eq_update >= eq_update_interval){
    // Update what the pots do based on the 
    gain_toggle_handler(gain_toggle_button_struct.toggle_index);
    peaking_eq_toggle_handler(peaking_eq_toggle_button_struct.toggle_index);

    	midSide.setMidGain(midGain);
    	midSide.setSideGain(sideGain);
   	codec.volume(codecGain);

		updatePeakingEQ(peakingFrequency, peakingGain, peakingQ);
    	updateHighShelf(HIGH_SHELF_F0, readAndScale(HIGH_SHELF_GAIN_POT, 24.0f, -12.0f), SHELVING_Q);
    	updateLowShelf(LOW_SHELF_F0, readAndScale(LOW_SHELF_GAIN_POT, 24.0f, -12.0f), SHELVING_Q);

    prev_eq_update = currentMS;
  }

  // DEBUG PRINTING
  if (currentMS - prevMS >= updateInterval){
    Serial.printf("Mid Gain: %.2f, Side Gain: %.2f, Master Volume: %.2f, EQ Gain %.2f, Freq %.2f, Q %.2f, EQ Index %d, Gain Index %d\n", 
                  midGain, sideGain, codecGain, peakingGain, peakingFrequency, peakingQ, peaking_eq_toggle_button_struct.toggle_index, gain_toggle_button_struct.toggle_index);
    prevMS = currentMS;
  }
}



// ---------- HELPER FUNCTIONS ----------

float32_t readAndScale(int pin, float32_t upper, float32_t offset){
  return (float32_t) analogRead(pin) * (upper/1023.0f) + offset;
}

void updateLowShelf(float32_t f0, float32_t gain, float32_t q){
  mainEqL.setLowShelf(0, gain, f0, q);
  mainEqR.setLowShelf(0, gain, f0, q);
}

void updateHighShelf(float32_t f0, float32_t gain, float32_t q){
  mainEqL.setHighShelf(2, gain, f0, q);
  mainEqR.setHighShelf(2, gain, f0, q);
}

void updatePeakingEQ(float32_t f0, float32_t gain, float32_t q){
  mainEqL.setPeaking(1, gain, f0, q);
  mainEqR.setPeaking(1, gain, f0, q);
}

// Analog Reading methods
void toggle_button_handler(TOGGLE_BUTTON* b)
  {
  int reading = digitalRead(b->button_pin);
  if (b->current_ms - b->prev_millis >= 20){
    if (reading == 1 && b->prev_button_state == 0) {
      b->toggle_index = (b->toggle_index + 1) % 3;
    }
    b->prev_millis = b->current_ms;
    b->prev_button_state = reading;
  }
}

// Index handlers
void gain_toggle_handler(int index){
	int raw = analogRead(GAIN_POT);
	if (!pickup_update(&gain_pot_pickup, index, raw)) return;
  	if (index == 0) codecGain = master_gain_smoother.process(readAndScale(GAIN_POT, 1.00, -0.25f));
  	if (index == 1) midGain = mid_gain_smoother.process(readAndScale(GAIN_POT, 1.00f, -0.25f));
  	if (index == 2) sideGain = side_gain_smoother.process(readAndScale(GAIN_POT, 1.00f, -0.25f));
  
} 

void peaking_eq_toggle_handler(int index){
	int raw = analogRead(PEAKING_EQ_CONTROL_POT);
	if (!pickup_update(&eq_pot_pickup, index, raw)) return;

  	if (index == 0) peakingGain = peaking_gain_smoother.process(readAndScale(PEAKING_EQ_CONTROL_POT, 24.0f, -12.0f));
  	if (index == 1) peakingFrequency = peaking_freq_smoother.process(readAndScale(PEAKING_EQ_CONTROL_POT, 19980.0f, 20.0f));
  	if (index == 2) peakingQ = peaking_q_smoother.process(readAndScale(PEAKING_EQ_CONTROL_POT, 9.5f, 0.5f));
}


bool pickup_update(PICKUP* p, int idx, int raw){
  // Check if the pickup object is locked
  if (p->locked) {
    int target = p->stored_raw[idx];                          // Target is number that we want match to begin updating

    // Distance from target {|raw reading - target|} within the range {window size/2}
    bool inside  = abs(raw - target) <= (WINDOW >> 1);        // Inside is the range surrounding target 
    bool crossed = (long)(p->last_raw - target) * (raw - target) <= 0; // ??
    if (inside || crossed) p->locked = false;                 // Unlock the pot if the crossed or inside threshold
  }
  p->last_raw = raw;
  if (p->locked) return false;

  p->stored_raw[idx] = raw;   // follow the pot while in control
  return true;
}