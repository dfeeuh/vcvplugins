#include "NoteGenerator.hpp"
#include <logger.hpp>
#include <algorithm>

NoteGenerator::NoteGenerator() : 
    noteRange{0x7F}, 
    centreNote{64}, 
    currentKey{NONE},
    keyBase_{CHROMATIC},
    accidental_{NATURAL},
    isMinor_{false}
{
}

static unsigned binarySearch(unsigned *array, unsigned len, unsigned note)
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

// Given the state of keyBase_, accidental_ and isMinor_
// create a new keyMap. 
void NoteGenerator::updateKey()
{
    // Depending on major/minor status, read the correct KEY value.
    if (isMinor_)
    {
        constexpr KEY minorkeys[NUM_BASE_KEYS] = {
            NONE, Fs_MAG, Gs_MAG, A_MAG, B_MAG, Cs_MAG, D_MAG, E_MAG};

        currentKey = minorkeys[keyBase_];
    }
    else
    {
        constexpr KEY majorkeys[NUM_BASE_KEYS] = {
            NONE, A_MAG, B_MAG, C_MAG, D_MAG, E_MAG, F_MAG, G_MAG};

        currentKey = majorkeys[keyBase_];
    }

    // Ignore accidental and return
    if (currentKey == NONE)
        return;

    // Add the accidental as an integer
    currentKey = (KEY)((int)currentKey + (int)accidental_);   

    // generate a new key map 
    // The MIDI pitches of the C-major scale
    const unsigned keyMapBasis[NUM_NOTES_IN_SCALE] = {0, 2, 4, 5, 7, 9, 11};
    unsigned workspace[NUM_NOTES_IN_SCALE];

    for (unsigned i=0; i<NUM_NOTES_IN_SCALE; i++)
    {
        // Key = (C major + new key root note) modulo 12
        workspace[i] = (currentKey + keyMapBasis[i]) % 12;
    }

    // Sort to make all notes in order
    std::sort(workspace, workspace+NUM_NOTES_IN_SCALE);
    
    for (unsigned i=0; i<NUM_NOTES_CHROMATIC; i++)
    {
        // Use a binary search algorithm to fill an array with the nearest value.
        keyMap[i] = binarySearch(workspace, NUM_NOTES_IN_SCALE, i);
    }	

    //DEBUG("Key change - note [%d], accidental [%d], isMinor [%d]. Key [%d]\n", keyBase_, accidental_, isMinor_, (int)currentKey);
}


void NoteGenerator::setNoteOffset(unsigned offset)
{
    centreNote = offset > 127 ? 127 : offset;
}

void NoteGenerator::setNoteRange(unsigned range)
{
    noteRange = range > 127 ? 127 : range;
    if(noteRange == 0) noteRange = 1;
}

// Run a linear feedback shift register
// From https://en.wikipedia.org/wiki/Linear-feedback_shift_register#Galois_LFSRs
unsigned NoteGenerator::generatePitch()
{
    // Generate the number in Qx.1 format to get a half.
    // Then round. Not sure it makes a big difference.
    unsigned noteout = lfsr.generate() & 0xFF;
    noteout = ((noteout + 1) >> 1);

    // map to range
    {
        // Use a signed version when dealing with subtraction
        int i32note = (int)noteout;
        // Calculate upper and lower boundaries within MIDI boundaries
        int upper = (int)centreNote + noteRange/2;
        upper = (upper > 127) ? 127 : upper;

        // Careful with subtraction
        int lower = (int)upper - noteRange;    
        lower = (lower < 0) ? 0 : lower;

        // Wrap input by octave
        while (i32note > upper)
            i32note -= NUM_NOTES_CHROMATIC;

        while (i32note < lower)
            i32note += NUM_NOTES_CHROMATIC;

        // Clamp into min max if we've overshot
        if (i32note > upper) i32note = upper;
        
        noteout = (unsigned)i32note;
    }

    if (currentKey == NONE)
        return noteout;
        
    // snap to a key
    // First get octave and map midiNoteIn to 0 to 11:
    unsigned octave = noteout / NUM_NOTES_CHROMATIC;
    // The remainder is the basis note
    unsigned basisNote  = noteout - (octave * NUM_NOTES_CHROMATIC);

    unsigned note = keyMap[basisNote];

    return (note + octave * NUM_NOTES_CHROMATIC);
} 

// Generate a random value between 0 and 127
unsigned NoteGenerator::generateVelocity()
{
    return lfsr.generate() & 0x7F;
}

void NoteGenerator::updateKey(KEY_BASE note) {
    keyBase_ = note;
    updateKey();
}

void NoteGenerator::updateKey(bool isMinor) {
    isMinor_ = isMinor;
    updateKey();
}

void NoteGenerator::updateKey(ACCIDENTAL accidental) {
    accidental_ = accidental;
    updateKey();
}