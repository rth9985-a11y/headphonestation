// #include "CrossTalk.h"

// CrossTalk::CrossTalk() : AudioStream(2, inputQueueArray) {}

// void CrossTalk::update() {

//   int32_t smoothedGain = 0;

//   smoothedGain += (bleedGain - smoothedGain) >> 10;

//   audio_block_t *blockL = receiveWritable(0);
//   audio_block_t *blockR = receiveWritable(1);

//   if (!blockL)
//     return;
//   if (!blockR)
//     return;

//   for (int i = 0; i < 128; i++) {
//     int16_t xL = (int16_t)blockL->data[i];
//     int16_t xR = (int16_t)blockR->data[i];

//     // Write a sample into the circular buffer on each bleed channe
//     delayBufferL2R[writeIdx] = xL;
//     delayBufferR2L[writeIdx] = xR;

//     // Advance and loopback for write pointer
//     writeIdx++;
//     if (writeIdx >= BUFFER_SIZE)
//       writeIdx = 0;

//     // Advance and loopback for read pointer
//     readIdx = (writeIdx - delaySamples + BUFFER_SIZE);
//     if (readIdx >= BUFFER_SIZE)
//       readIdx = 0;

//     // Update the output value (blend the circular buffer with the raw LR data)
//     blockL->data[i] = xL + ((delayBufferR2L[readIdx] * bleedGain) >> 15);
//     blockR->data[i] = xR + ((delayBufferL2R[readIdx] * bleedGain) >> 15);
//   }

//   transmit(blockL, 0);
//   transmit(blockR, 1);
//   release(blockL);
//   release(blockR);
// }

// void CrossTalk::CrossTalkInit() { delayIndex = 0; }

// void CrossTalk::setDelayMs(int16_t samples) {
//   if (samples >= MAX_DELAY)
//     samples = MAX_DELAY;
//   if (samples <= 0)
//     samples = 0;
//   delaySamples = samples;
// }

// void CrossTalk::setBleedGain(int16_t gain) {
//   if (gain < 0)
//     gain = 0;
//   if (gain > 32767)
//     gain = 32767;
//   bleedGain = gain;
// }

// void CrossTalk::setLPFCutoff(float32_t fc) {}
