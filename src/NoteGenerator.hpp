#pragma once

#include <algorithm>

#define NUM_NOTES_IN_SCALE 7
#define NUM_NOTES_CHROMATIC 12

class NoteGenerator
{
public:
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

    typedef enum 
    {
        FLAT=-1,
        NATURAL,
        SHARP
    } ACCIDENTAL;

private:
    unsigned upper;
    unsigned lower;

    uint16_t start_state;
	uint16_t lfsr;

	const unsigned keyMapBasis[NUM_NOTES_IN_SCALE] = {0, 2, 4, 5, 7, 9, 11};
	unsigned keyMapChrom[NUM_NOTES_CHROMATIC];
	KEY currentKey;

public:
    NoteGenerator();

    // Given the parameters, converted to a KEY type
    // note: -1 = none
    // isMinor: if true, return the minor (major key down three semitones)
    // accidental: -1 = flat, 0 = natural; +1 = sharp
    KEY getKey(int note, bool isMinor, ACCIDENTAL accidental);

	unsigned binarySearch(unsigned *array, unsigned len, unsigned note);
    void setLower(unsigned x);
    void setUpper(unsigned x);
    unsigned snapToKey(unsigned noteIn);    
	unsigned generate(void);
    float noteToCv(unsigned note);
    void setKey(KEY newKeyId);
	
};
