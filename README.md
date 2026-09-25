# Headphone Station

## TODO soon:
- Patch pots to analog pis for the mid-side processing
- Get rid of the crazy deadband stuff and keep trying different techniques to reduce fast EQ-coeff updates
    - Coefficient interpolation 
    - less aggressive deadband
    - See REFERENCES.md for EE times article
- Fix gain staging for mid-side --> why does signal pass despite mid and side gain = 0?
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
