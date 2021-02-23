#pragma once

#include <algorithm>

#define NUM_NOTES 7

struct NoteGenerator
{
    uint16_t start_state;
	uint16_t lfsr;

	typedef enum keyIds {
		C_MAG=0,
		NUM_KEYS
	} KEY;

	const unsigned keyMapBasis[NUM_NOTES] = {0, 2, 4, 5, 7, 9, 11};
	unsigned keyMap[NUM_NOTES];

    NoteGenerator() : start_state{0xACE1u} {
		lfsr = start_state;
    }
    
   	// Run a linear feedback shift register
	// From https://en.wikipedia.org/wiki/Linear-feedback_shift_register#Galois_LFSRs
	unsigned random(void)
	{
#ifndef LEFT
		unsigned lsb = lfsr & 1u;  /* Get LSB (i.e., the output bit). */
		lfsr >>= 1;                /* Shift register */
		if (lsb)                   /* If the output bit is 1, */
			lfsr ^= 0xB400u;       /*  apply toggle mask. */
#else
		unsigned msb = (int16_t) lfsr < 0;   /* Get MSB (i.e., the output bit). */
		lfsr <<= 1;                          /* Shift register */
		if (msb)                             /* If the output bit is 1, */
			lfsr ^= 0x002Du;                 /*  apply toggle mask. */
#endif
        // Mask into MIDI note range 0..127
		return lfsr & 0x7F;
	} 

    inline float note2cv(unsigned note)
    {
        return (note - 60.0f) / 12.f;
    }
    
    void generateNewKeyIds(KEY newKeyId)
	{
		// Everytime a new key is selected, generate a new key map
		for (unsigned i=0; i<NUM_NOTES; i++)
		{
			// Key = (C major + new key root note) modulo 12
			keyMap[i] = (newKeyId + keyMapBasis[i]) % 12;
		}

		// Sort to make all notes in order
		std::sort(keyMap, keyMap+NUM_NOTES);
	}

    unsigned snapToKey(unsigned midiNoteIn, KEY keyId)
	{
		// First get octave and map midiNoteIn to 0 to 11:
		// unsigned octave = midiNoteIn / 12;
		// unsigned basisNote  = midiNoteIn - (octave * 12);

		// Then use a binary search algorithm to find the nearest value.

		// In order to cope with numbers exactly half, e.g. 10 for C Major 
		// is exactly half way between 9 and 11, the LFSR generates
		// Qx.1 (i.e. with 0 or 0.5). Then round up and using a search algorithm
		// that favours the lower side (10 will snap to 9, 10.5 will snap to 11).
		// Read https://en.wikipedia.org/wiki/Binary_search_algorithm
		// This looks promising: https://thecleverprogrammer.com/2020/11/11/binary-search-in-c/

		// return (snappedNote + octave * 12);
		return midiNoteIn;
	}

};
