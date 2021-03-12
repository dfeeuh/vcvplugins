#pragma once

#define NUM_NOTES_IN_SCALE 7
#define NUM_NOTES_CHROMATIC 12

class NoteGenerator
{
public:
    typedef enum keyBase {
        CHROMATIC=0,
        A, B, C, D, E, F, G, NUM_BASE_KEYS
    } KEY_BASE;

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
    unsigned noteRange;
    unsigned centreNote;

    unsigned start_state;
	unsigned lfsr;

	const unsigned keyMapBasis[NUM_NOTES_IN_SCALE] = {0, 2, 4, 5, 7, 9, 11};
	unsigned keyMapChrom[NUM_NOTES_CHROMATIC];
	KEY currentKey;
    KEY_BASE keyBase_;
    ACCIDENTAL accidental_;
    bool isMinor_;

public:
    NoteGenerator();

    void updateKey();
    
    void updateKey(KEY_BASE note);
    void updateKey(bool isMinor);
    void updateKey(ACCIDENTAL accidental);

	unsigned binarySearch(unsigned *array, unsigned len, unsigned note);

    void setNoteOffset(unsigned offset);
    void setNoteRange(unsigned range);
    void mapToRange(unsigned &note);

    unsigned snapToKey(unsigned noteIn);    
	unsigned generate(void);
    float noteToCv(unsigned note);
    void updateNoteMap();
};
