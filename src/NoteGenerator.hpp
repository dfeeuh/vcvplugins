#pragma once

#include <algorithm>

#define NUM_NOTES_IN_SCALE 7
#define NUM_NOTES_CHROMATIC 12

struct NoteGenerator
{
    uint16_t start_state;
	uint16_t lfsr;

	typedef enum keyIds {
        NONE=-1,
		C_MAG=0,
		Cs_MAG,
		D_MAG,
		Ds_MAG,
		E_MAG,
		F_MAG,
		Fs_MAG,
		G_MAG,
		Gs_MAG,
		A_MAG,
		As_MAG,
		B_MAG,
		NUM_KEYS
	} KEY;

	const unsigned keyMapBasis[NUM_NOTES_IN_SCALE] = {0, 2, 4, 5, 7, 9, 11};
	unsigned keyMapChrom[NUM_NOTES_CHROMATIC];
	KEY currentKey;

    NoteGenerator() : start_state{0xACE1u}, currentKey{NONE} {
		lfsr = start_state;
    }

	unsigned binarySearch(unsigned *array, unsigned len, unsigned note)
	{
		// Set as the wraparound case, in case no matches are found
		unsigned closest = array[len-1];
		int s = 0;
		int e = NUM_NOTES_IN_SCALE;
		while (s <= e)
		{
			int mid = (s + e) / 2;
			if (note == array[mid])
			{
				return array[mid];
			}
			else if (array[mid] > note)
			{
				e = mid - 1;
			}
			else
			{
				// Favour rounding down
				if (note-array[mid] == 1)
					closest = array[mid];

				s = mid + 1;
			}
		}
		
		return closest;
	}

    unsigned snapToKey(unsigned noteIn)
	{
        if (currentKey == NONE)
            return (noteIn);
			
		// First get octave and map midiNoteIn to 0 to 11:
		unsigned octave = noteIn / NUM_NOTES_CHROMATIC;
		unsigned basisNote  = noteIn - (octave * NUM_NOTES_CHROMATIC);

		unsigned note = keyMapChrom[basisNote];

		return (note + octave * 12);
	}	
    
   	// Run a linear feedback shift register
	// From https://en.wikipedia.org/wiki/Linear-feedback_shift_register#Galois_LFSRs
	unsigned generate(void)
	{
		// Generate the number in Qx.1 format to get a half.

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
		return snapToKey(lfsr & 0x7F);
	} 

    float noteToCv(unsigned note)
    {
        return (note - 60.0f) / 12.f;
    }
    
    void setKey(KEY newKeyId)
	{
      	unsigned workspace[NUM_NOTES_IN_SCALE];

		currentKey = newKeyId;
		if (newKeyId == NONE)
			return; 

		// Everytime a new key is selected, generate a new key map
		for (unsigned i=0; i<NUM_NOTES_IN_SCALE; i++)
		{
			// Key = (C major + new key root note) modulo 12
			workspace[i] = (newKeyId + keyMapBasis[i]) % 12;
		}

		// Sort to make all notes in order
		std::sort(workspace, workspace+NUM_NOTES_IN_SCALE);
		
		for (unsigned i=0; i<NUM_NOTES_CHROMATIC; i++)
		{
			// Then use a binary search algorithm to find the nearest value.

			// In order to cope with numbers exactly half, e.g. 10 for C Major 
			// is exactly half way between 9 and 11, the LFSR generates
			// Qx.1 (i.e. with 0 or 0.5). Then round up and using a search algorithm
			// that favours the lower side (10 will snap to 9, 10.5 will snap to 11).
			// Read https://en.wikipedia.org/wiki/Binary_search_algorithm
			// This looks promising: https://thecleverprogrammer.com/2020/11/11/binary-search-in-c/
			keyMapChrom[i] = binarySearch(workspace, NUM_NOTES_IN_SCALE, i);
		}		
	}
};
