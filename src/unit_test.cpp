/*
* My design followed a lot of test-driven development, so there 
* are quite a few unit tests. Lots of refactoring, etc.
*
* To run, use >./unit_test
*/ 

#include <random>
#include <iostream>

#define nDEBUG
#if DEBUG
# include <iostream>
#endif

#include "gtest/gtest.h"
#include "NoteGenerator.hpp"

class NoteGeneratorTest : public ::testing::Test
{
public:
    NoteGenerator noteGen;

    void SetUp() {
        noteGen.setKey(noteGen.NONE);
    }

    void TearDown() {
    }

    void printArray(unsigned *a, unsigned n) { 
        for (unsigned i=0; i<n; i++)
            std::cout << " " << a[i];
        std::cout << std::endl;
    }
};


// Sanity check our mechanism for setting up the effects chain
TEST_F (NoteGeneratorTest, testNoneDoesNothing)
{
    // Given a default NoteGenerator, any number that goes in should come
    // back unaffected.
    for (unsigned i=0; i<NUM_NOTES_CHROMATIC; i++)
        EXPECT_EQ(i, noteGen.snapToKey(i));
}

TEST_F (NoteGeneratorTest, testSnapToKey_noteInKey)
{
    noteGen.setKey(noteGen.A_MAG);

    printArray(noteGen.keyMapChrom, NUM_NOTES_CHROMATIC);

    // Snap to 1, 2, 4, 6, 8, 9, 11
    EXPECT_EQ(noteGen.snapToKey(1), 1);
    EXPECT_EQ(noteGen.snapToKey(2), 2);
    EXPECT_EQ(noteGen.snapToKey(4), 4);
    EXPECT_EQ(noteGen.snapToKey(6), 6);
    EXPECT_EQ(noteGen.snapToKey(8), 8);
    EXPECT_EQ(noteGen.snapToKey(9), 9);
    EXPECT_EQ(noteGen.snapToKey(11), 11);
}

TEST_F (NoteGeneratorTest, testSnapToKey_noteNotInKey)
{
    noteGen.setKey(noteGen.A_MAG);
    // Snap to 1, 2, 4, 6, 8, 9, 11
    EXPECT_EQ(noteGen.snapToKey(3), 2);
    EXPECT_EQ(noteGen.snapToKey(5), 4);
    EXPECT_EQ(noteGen.snapToKey(7), 6);
    EXPECT_EQ(noteGen.snapToKey(10), 9);
}

TEST_F (NoteGeneratorTest, testSnapToKey_wrapAround_low)
{
    noteGen.setKey(noteGen.A_MAG);
    EXPECT_EQ(noteGen.snapToKey(0), 11);
}

TEST_F (NoteGeneratorTest, testSnapToKey_wrapAround_high)
{
    noteGen.setKey(noteGen.Cs_MAG);
    EXPECT_EQ(noteGen.snapToKey(11), 10);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}