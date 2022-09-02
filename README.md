# RTsound_processing

Purpose:
Low-latency pitch-recognition tool

Description:
The code captures incoming audio (namely a musical note) from microphone or other PCM input device. Input can be received from arbitrary number of channels, but the elaborations are performed only on one of them.

Signal is treated with techniques derived from Fourier analysis (i.e. Discrete Fourier Transform, signal power, power spectrum and similar).

The application currently handles only one note at a time.
Note is identified by finding the frequency related to the maximum peak of squared DFT. Some waveforms have a peak at double the fundamental pitch. To reduce the risk of "wrong octave" detection the algorithm compares the peak found with the squared DFT value at half its frequency (it makes use of a parameter called "octave ratio", obtained as squared_DFT(f0)/squared_DFT(2*f0) where f0 is the fundamental pitch of the note in question).
To confirm the note is correct, a check is performed on the signal to verify that power associated with fundamental frequency and its harmonics is above a certain threshold (it makes use of another parameter "harmonic power start")
All parameters have been calculated for each note separately and saved in memory before compile-time. To compute them a synthesizer has been used.

Notes are handled according to the MIDI protocol (keys 0-127, velocity 0-127). When a new note is detected, a new MIDI event is generated and is sent to a sequencer, available for other applications to use.

Dependancies:
The project relies on ALSA libraries for PCM stream and MIDI sequencer.
to install:
> sudo apt-get install libasound-dev

Other libraries may be used for that purpose (for any reason...). In that case only library "audiostream" is to be modified, since it exports the ALSA interface to the main
