#pragma once

#include <algorithm>

#define NUM_NOTES_IN_SCALE 7
#define NUM_NOTES_CHROMATIC 12

struct NoteGenerator
{
    unsigned upper;
    unsigned lower;

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

    NoteGenerator() : upper{0x72}, lower{60}, start_state{0xACE1u}, currentKey{NONE} {
		lfsr = start_state;
    }

	unsigned binarySearch(unsigned *array, unsigned len, unsigned note)
	{
		// Read https://en.wikipedia.org/wiki/Binary_search_algorithm
	
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

    void setLower(unsigned x)
    {
        lower = std::min(x, upper-NUM_NOTES_CHROMATIC);
    }

    void setUpper(unsigned x)
    {
        upper = std::max(x, lower+NUM_NOTES_CHROMATIC);
    }

    unsigned snapToKey(unsigned noteIn)
	{
        if (currentKey == NONE)
            return (noteIn);
			
		// First get octave and map midiNoteIn to 0 to 11:
		unsigned octave = noteIn / NUM_NOTES_CHROMATIC;
		unsigned basisNote  = noteIn - (octave * NUM_NOTES_CHROMATIC);

		unsigned note = keyMapChrom[basisNote];

		return (note + octave * NUM_NOTES_CHROMATIC);
	}	
    
   	// Run a linear feedback shift register
	// From https://en.wikipedia.org/wiki/Linear-feedback_shift_register#Galois_LFSRs
	unsigned generate(void)
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

        // Generate the number in Qx.1 format to get a half.
        // Then round. Not sure it makes a big difference.
        unsigned noteout = lfsr & 0xFF;
        noteout = ((noteout += 1) >> 1);

        while (noteout > upper)
        {
            // Subtract an octave. Upper can never be less than NUM_NOTES_CHROMATIC
            noteout -= NUM_NOTES_CHROMATIC;
        }

        while (noteout < lower)
        {
            // Lower can never be greater than 115
            noteout += NUM_NOTES_CHROMATIC;
        }

		return snapToKey(noteout);
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
			// Use a binary search algorithm to fill an array with the nearest value.
			keyMapChrom[i] = binarySearch(workspace, NUM_NOTES_IN_SCALE, i);
		}		
	}
};
