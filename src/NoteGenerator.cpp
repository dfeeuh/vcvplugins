#include "NoteGenerator.hpp"
#include <logger.hpp>
#include <algorithm>
#include <mutex>

// C++11 workaround
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

NoteGenerator::NoteGenerator() : 
    noteRange{0x7F}, 
    centreNote{64}, 
    lastNote_{60},
    currentKey{NONE},
    keyBase_{CHROMATIC},
    accidental_{NATURAL},
    isMinor_{false},
    mode_{MAJOR}
{
    assert(std::atomic<KEY>{}.is_lock_free());
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
    constexpr KEY majorkeys[NUM_BASE_KEYS] = {
        NONE, A_MAG, B_MAG, C_MAG, D_MAG, E_MAG, F_MAG, G_MAG};

    constexpr KEY minorkeys[NUM_BASE_KEYS] = {
        NONE, Fs_MAG, Gs_MAG, A_MAG, B_MAG, Cs_MAG, D_MAG, E_MAG};    

    KEY newKey = NONE;
    // Depending on major/minor status, read the correct KEY value.
    switch(mode_)
    {
    case MAJOR:
        newKey = majorkeys[keyBase_];
        break;

    case MINOR:
        newKey = minorkeys[keyBase_];
        break;

    case PENTATONIC_MAJ:
        // TODO
    case PENTATONIC_MIN:
        // TODO
    default:
        // assert(0);
         break;
    }

    // Ignore accidental and return
    if (newKey == NONE)
    {
        currentKey.store(newKey);
        return;
    }
    
    // Add the accidental as an integer
    newKey = (KEY)((int)newKey + (int)accidental_); 
    currentKey.store(newKey);

    // generate a new key map 
    // The MIDI pitches of the C-major scale
    const unsigned keyMapBasis[NUM_NOTES_IN_SCALE] = {0, 2, 4, 5, 7, 9, 11};
    unsigned workspace[NUM_NOTES_IN_SCALE];

    for (unsigned i=0; i<NUM_NOTES_IN_SCALE; i++)
    {
        // Key = (C major + new key root note) modulo 12
        workspace[i] = (newKey + keyMapBasis[i]) % 12;
    }

    // Sort to make all notes in order
    std::sort(workspace, workspace+NUM_NOTES_IN_SCALE);
    
    // See https://youtu.be/Q0vrQFyAdWI?t=2663 for how the thread safety is working
    {
        auto newKeyMap = make_unique<KEYMAP>();
        std::lock_guard<spin_lock> lock(mutex);
        for (unsigned i=0; i<NUM_NOTES_CHROMATIC; i++)
        {
            // Use a binary search algorithm to fill an array with the nearest value.
            newKeyMap->data[i] = binarySearch(workspace, NUM_NOTES_IN_SCALE, i);
        }
        std::swap(keyMap_, newKeyMap);
        // newKeyMap is deleted once it goes out of scope
    }
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

// This function is called by the audio thread and therefore must be thread safe.
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

    // Try the lock and returns immediately. 
    // From Real-time 101, part 1 https://www.youtube.com/watch?v=Q0vrQFyAdWI
    std::unique_lock<spin_lock> tryLock(mutex, std::try_to_lock);
    if (tryLock.owns_lock())
    {
        lastNote_ = (keyMap_->data[basisNote] + octave * NUM_NOTES_CHROMATIC);
    }

    // If we fail to get a new note, just use the last one. The keyMap_ will be 
    // updated on the next note.    
    return lastNote_;    
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
    //updateKey();

    // TODO - replace this
    updateKey(isMinor_ ? MINOR : MAJOR);
}

void NoteGenerator::updateKey(ACCIDENTAL accidental) {
    accidental_ = accidental;
    updateKey();
}

void NoteGenerator::updateKey(MODE mode)
{
    mode_ = mode;
    updateKey();
}
