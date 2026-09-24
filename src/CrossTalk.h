// #pragma once
// #include <Audio.h>
// #include <arm_math.h>
// #include <cstdint>

// class CrossTalk : public AudioStream {
// public:
//   CrossTalk();
//   void CrossTalkInit();
//   void update(void);
//   void setDelayMs(int16_t samples);
//   void setBleedGain(int16_t gain);
//   void setLPFCutoff(float32_t fc);

// private:
//   audio_block_t *inputQueueArray[2];

//   // Buffers and buffer indicies
//   static const int BUFFER_SIZE = 39690; // 39,690 is ~1 foot of sound travel
//   int16_t delayBufferL2R[MAX_DELAY];
//   int16_t delayBufferR2L[MAX_DELAY];
//   int16_t writeIdx;
//   int16_t readIdx = 0;

//   // Amount of bleed between "speakers"
//   int16_t bleedGain;
// };
