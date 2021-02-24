/*
* My design followed a lot of test-driven development, so there 
* are quite a few unit tests. Lots of refactoring, etc.
*
* To run, use >./unit_test
*/ 

#include <random>

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
};


// Sanity check our mechanism for setting up the effects chain
TEST_F (NoteGeneratorTest, testNoneDoesNothing)
{
    // Given a default NoteGenerator, any number that goes in should come
    // back unaffected.
    for (unsigned i=0; i<12; i++)
        EXPECT_EQ(i, noteGen.snapToKey(i));
}

TEST_F (NoteGeneratorTest, testNewKeyId)
{
    noteGen.setKey(noteGen.A_MAG);
    unsigned result[NUM_NOTES] = {1, 2, 4, 6, 8, 9, 11};

    for (unsigned i=0; i<NUM_NOTES; i++)
    {
        EXPECT_EQ(noteGen.keyMap[i], result[i]);
    }
}

TEST_F (NoteGeneratorTest, testSnapToKey_noteInKey)
{
    noteGen.setKey(noteGen.A_MAG);
    // Snap to 1, 2, 4, 6, 8, 9, 11
    EXPECT_EQ(noteGen.binarySearch(1), 1);
    EXPECT_EQ(noteGen.binarySearch(2), 2);
    EXPECT_EQ(noteGen.binarySearch(4), 4);
    EXPECT_EQ(noteGen.binarySearch(6), 6);
    EXPECT_EQ(noteGen.binarySearch(8), 8);
    EXPECT_EQ(noteGen.binarySearch(9), 9);
    EXPECT_EQ(noteGen.binarySearch(11), 11);
}

TEST_F (NoteGeneratorTest, testSnapToKey_noteNotInKey)
{
    noteGen.setKey(noteGen.A_MAG);
    // Snap to 1, 2, 4, 6, 8, 9, 11
    EXPECT_EQ(noteGen.binarySearch(0), 11);
    EXPECT_EQ(noteGen.binarySearch(3), 2);
    EXPECT_EQ(noteGen.binarySearch(5), 4);
    EXPECT_EQ(noteGen.binarySearch(7), 6);
    EXPECT_EQ(noteGen.binarySearch(10), 9);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}