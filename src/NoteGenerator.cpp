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

// Given the parameters, converted to a KEY type
// note: -1 = none
// isMinor_: if true, return the minor (major key down three semitones)
// accidental: -1 = flat, 0 = natural; +1 = sharp
void NoteGenerator::updateKey()
{
    constexpr KEY majorkeys[NUM_BASE_KEYS] = {NONE, A_MAG, B_MAG, C_MAG, D_MAG, E_MAG, F_MAG, G_MAG};
    constexpr KEY minorkeys[NUM_BASE_KEYS] = {NONE, Fs_MAG, Gs_MAG, A_MAG, B_MAG, Cs_MAG, D_MAG, E_MAG};

    if (isMinor_)
        currentKey = minorkeys[keyBase_];
    else
        currentKey = majorkeys[keyBase_];

    if (currentKey != NONE)
    {
        currentKey = (KEY)((int)currentKey + (int)accidental_);
        updateNoteMap();
    }

    // Print some logging here
    //DEBUG("Key change - note [%d], accidental [%d], isMinor [%d]. Key [%d]\n", keyBase_, accidental_, isMinor_, (int)currentKey);
}

unsigned NoteGenerator::binarySearch(unsigned *array, unsigned len, unsigned note)
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

void NoteGenerator::setNoteOffset(unsigned offset)
{
    centreNote = offset > 127 ? 127 : offset;
}

void NoteGenerator::setNoteRange(unsigned range)
{
    noteRange = range > 127 ? 127 : range;
    if(noteRange == 0) noteRange = 1;
}

unsigned NoteGenerator::snapToKey(unsigned noteIn)
{
    if (currentKey == NONE)
        return (noteIn);
        
    // First get octave and map midiNoteIn to 0 to 11:
    unsigned octave = noteIn / NUM_NOTES_CHROMATIC;
    unsigned basisNote  = noteIn - (octave * NUM_NOTES_CHROMATIC);

    unsigned note = keyMapChrom[basisNote];

    return (note + octave * NUM_NOTES_CHROMATIC);
}	

void NoteGenerator::mapToRange(unsigned &note)
{
    int noteTmp = (int)note;
    // Calculate upper and lower boundaries within MIDI boundaries
    int upper = (int)centreNote + noteRange/2;
    upper = (upper > 127) ? 127 : upper;

    // Careful with subtraction
    int lower = (int)upper - noteRange;    
    lower = (lower < 0) ? 0 : lower;

    // Wrap input by octave
    while (noteTmp > upper)
        noteTmp -= NUM_NOTES_CHROMATIC;

    while (noteTmp < lower)
        noteTmp += NUM_NOTES_CHROMATIC;

    // Clamp into min max if we've overshot
    if (noteTmp > upper) noteTmp = upper;

    note = noteTmp;
}

// Run a linear feedback shift register
// From https://en.wikipedia.org/wiki/Linear-feedback_shift_register#Galois_LFSRs
unsigned NoteGenerator::generatePitch()
{
    // Generate the number in Qx.1 format to get a half.
    // Then round. Not sure it makes a big difference.
    unsigned noteout = lfsr.generate() & 0xFF;
    noteout = ((noteout + 1) >> 1);

    mapToRange(noteout);

    return snapToKey(noteout);
} 

// Generate a random value between 0 and 127
unsigned NoteGenerator::generateVelocity()
{
    return lfsr.generate() & 0x7F;
}

float NoteGenerator::noteToCv(unsigned note)
{
    return (note - 60.0f) / 12.f;
}

void NoteGenerator::updateNoteMap()
{
    unsigned workspace[NUM_NOTES_IN_SCALE];


    // Everytime a new key is selected, generate a new key map
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
        keyMapChrom[i] = binarySearch(workspace, NUM_NOTES_IN_SCALE, i);
    }		
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