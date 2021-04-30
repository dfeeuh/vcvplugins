#pragma once
#include <chrono>
#include <cstdint>
#include <atomic>
#include <memory>


#define NUM_NOTES_IN_SCALE 7
#define NUM_NOTES_CHROMATIC 12

// From https://youtu.be/Q0vrQFyAdWI?t=2663
// A bit crude but probably fine for this simple plugin...
class spin_lock {
public:
    void lock() noexcept     { while (flag.test_and_set()); }
    void unlock() noexcept   { flag.clear(); }
    bool try_lock() noexcept { return ! flag.test_and_set(); }
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

// a Galois linear feedback shift register initialised from the clock time
class LFSR {
private:
    uint16_t lfsr;

public:
    LFSR() {
        // Seed the LFSR with the current clock
        auto t = (std::chrono::system_clock::now()).time_since_epoch();
        lfsr = (uint16_t)(t.count() & 0xFFFF);
    }

    // From https://en.wikipedia.org/wiki/Linear-feedback_shift_register#Galois_LFSRs
    uint16_t generate(void)
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
        return lfsr;
    }
};

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

    typedef enum
    {
        MAJOR=0,
        MINOR,
        PENTATONIC_MAJ,
        PENTATONIC_MIN
    } MODE;

private:
	LFSR lfsr;
    unsigned noteRange;
    unsigned centreNote;
    unsigned lastNote_;

    // DANGER! these are used in the audio thread and updated in the GUI
    // by updateKey. See https://youtu.be/Q0vrQFyAdWI?t=2663
    spin_lock mutex;
    struct KEYMAP {
	    unsigned data[NUM_NOTES_CHROMATIC];
    };
    std::unique_ptr<KEYMAP> keyMap_;
	std::atomic<KEY> currentKey;

    // Local to GUI thread only
    KEY_BASE keyBase_;
    ACCIDENTAL accidental_;
    bool isMinor_;
    MODE mode_;

public:
    NoteGenerator();

    void updateKey();
    void updateKey(KEY_BASE note);
    void updateKey(bool isMinor);
    void updateKey(ACCIDENTAL accidental);
    void updateKey(MODE mode);

    void setNoteOffset(unsigned offset);
    void setNoteRange(unsigned range);

	unsigned generatePitch();
    unsigned generateVelocity();
};
