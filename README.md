# Headphone Station

## TODO soon:
- Implement filtering (mid channel low-pass, side channel high-pass)
- Implement delay on one channel (remember that MS is encoding one side channel --> order of operations is important here)

## Description:
Headphone station is a digital signal processor that includes the following primitive processes:
- 3 band parametric equilizer (low shelf - fixed freq, peaking band - variable Q, freq, and gain, high shelf - fixed freq)
- Mid-Side processor (currently just splits channels, processing to be implemented later)

## Technical Specifications
- 44.1kHz, 16-bit 
- 32-bit floating point DSP
- Supports USB input and line input

## On parameter smoothing:
The problem: analog potentiometer readings are not stable, digital filter updates react poorly to quick and frequency adjustment resulting in clicking and popping noises (also known as zippering)

The fix: There are many known methods to reduce potentiometer noise - I chose to attack this at the parameter update stage. Within the corresponding setCoefficients() methods for the parametric EQ class, I use a one pole digital low pass filter that receives a raw unsteady signal from the potentiometer and outputs a smoothed (slower moving) signal to be used in the coefficient update values. The equation is as follows: 

Standard form one-pole IIR smoother:

$ y[n] = b0 * x[n] - a1 * y[n - 1] $

Which can be rearranged to use only one coefficient:

$ y[n] = y[n - 1] + a * (target - y[n - 1]) $