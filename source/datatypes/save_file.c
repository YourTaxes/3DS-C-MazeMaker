#include "datatypes/save_file.h"
#include "utils/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>   // offsetof
#include <sys/stat.h> // mkdir

/*
* a best time and its valid flag, in the order Raw_Level keeps them. writing
* exactly TIME_PAIR_BYTES sends the double and the bool and stops, so the byte
* after the flag (`empty`, for the standard pair) is left alone. sizeof(TimePair)
* would be 16 with trailing padding and would clobber it.
*/
typedef struct { double time; bool valid; } TimePair;
#define TIME_PAIR_BYTES (offsetof(TimePair, valid) + sizeof(bool))

//Raw_Level keeps each time next to its own flag on purpose, so one write does both.
//if someone reorders those fields, this stops the build instead of corrupting saves.
_Static_assert(offsetof(Raw_Level, standardTimeValid) - offsetof(Raw_Level, bestTimeStandard) == offsetof(TimePair, valid),
               "bestTimeStandard and standardTimeValid must stay adjacent");
_Static_assert(offsetof(Raw_Level, hardTimeValid) - offsetof(Raw_Level, bestTimeHard) == offsetof(TimePair, valid),
               "bestTimeHard and hardTimeValid must stay adjacent");

/*
* byte offset of Levels[slot] inside the file, or -1 if slot is out of range.
* every per slot function goes through this so the offset math is written once.
*/
static long slotOffset(int slot)
{
    if (slot < 0 || slot >= LEVEL_SLOT_CNT){
        printConsole("save slot %d is out of range (0..%d)", slot, LEVEL_SLOT_CNT - 1);
        return -1;
    }
    return (long)(offsetof(Save_File, Levels) + (size_t)slot * sizeof(Raw_Level));
}

/*
* open the save file, seek to `offset`, and read or write `size` bytes there.
* "r+b" opens the existing file for update, never truncating the rest of it.
* returns true only if every byte was transferred.
*/
static bool transferAt(long offset, void* buf, size_t size, bool write)
{
    FILE* save_file = fopen(SAVE_PATH, write ? "r+b" : "rb");
    if (save_file == NULL){
        printConsole("could not open %s for %s", SAVE_PATH, write ? "writing" : "reading");
        return false;
    }

    bool ok = fseek(save_file, offset, SEEK_SET) == 0;
    if (ok){
        size_t current = write ? fwrite(buf, 1, size, save_file) : fread(buf, 1, size, save_file);
        ok = (current == size);
    }
    fclose(save_file);

    if (!ok) printConsole("%s %u bytes at offset %ld of %s failed",
                          write ? "writing" : "reading", (unsigned)size, offset, SAVE_PATH);
    return ok;
}

/*
* is the file on disk one this build can use? logs the first reason it isn't.
* checks size first (catches a truncated file), then magic, then version.
*/
static bool isValid(void)
{
    FILE* save_file = fopen(SAVE_PATH, "rb");
    if (save_file == NULL){
        printConsole("save file %s not found", SAVE_PATH);
        return false;
    }

    bool ok = false;
    u32 header[2]; // magic, version

    fseek(save_file, 0, SEEK_END);
    long size = ftell(save_file);
    rewind(save_file);

    if (size != (long)sizeof(Save_File)){
        printConsole("save file is %ld bytes, expected %u", size, (unsigned)sizeof(Save_File));
    } else if (fread(header, sizeof header, 1, save_file) != 1){
        printConsole("could not read the save file header");
    } else if (header[0] != SAVE_MAGIC){
        printConsole("save file magic is 0x%08lX, expected 0x%08lX", (unsigned long)header[0], (unsigned long)SAVE_MAGIC);
    } else if (header[1] != SAVE_VERSION){
        //only version 1 exists so far. when SAVE_VERSION is bumped, this is where
        //an old file gets migrated instead of thrown away.
        printConsole("save file is version %lu, this build wants %u", (unsigned long)header[1], SAVE_VERSION);
    } else {
        ok = true;
    }

    fclose(save_file);
    return ok;
}

bool SaveFile_Ensure(void)
{
    if (isValid()){
        printConsole("save file found at %s", SAVE_PATH);
        return true;
    }

    printConsole("creating a fresh save file, %u bytes", (unsigned)sizeof(Save_File));

    //zeroed memory already means every tile is EMPTY, both times 0 and invalid, and levelName "".
    //the buffer is freed before returning so the heap is back where it started.
    Save_File* fresh = calloc(1, sizeof(Save_File));
    if (fresh == NULL){
        printConsole("out of memory building the fresh save file");
        return false;
    }

    fresh->magic = SAVE_MAGIC;
    fresh->version = SAVE_VERSION;
    for (int i = 0; i < LEVEL_SLOT_CNT; i++) fresh->Levels[i].empty = true;

    bool ok = SaveFile_Write(fresh);
    free(fresh);
    return ok;
}

bool SaveFile_Read(Save_File* out)
{
    return transferAt(0, out, sizeof(Save_File), false);
}

bool SaveFile_Write(const Save_File* file)
{
    //the folders are made here rather than in _Ensure so a save still works if
    //they were deleted while the game was running. EEXIST is the normal case.
    mkdir("sdmc:/3ds", 0777);
    mkdir(SAVE_DIR, 0777);

    FILE* save_file = fopen(SAVE_PATH, "wb");
    if (save_file == NULL){
        printConsole("could not create %s", SAVE_PATH);
        return false;
    }

    bool ok = fwrite(file, sizeof(Save_File), 1, save_file) == 1;
    fclose(save_file);

    if (!ok) printConsole("writing %s failed", SAVE_PATH);
    return ok;
}

bool SaveFile_ReadSlot(int slot, Raw_Level* out)
{
    long offset = slotOffset(slot);
    if (offset < 0) return false;
    return transferAt(offset, out, sizeof(Raw_Level), false);
}

bool SaveFile_WriteSlot(int slot, const Raw_Level* lvl)
{
    long offset = slotOffset(slot);
    if (offset < 0) return false;
    return transferAt(offset, (void*)lvl, sizeof(Raw_Level), true);
}

bool SaveFile_WriteSlotTime(int slot, bool hardMode, double time)
{
    long offset = slotOffset(slot);
    if (offset < 0) return false;

    size_t fieldOff = hardMode ? offsetof(Raw_Level, bestTimeHard)
                               : offsetof(Raw_Level, bestTimeStandard);

    //recording a time is what makes it valid, so the flag goes out with it
    TimePair pair = { .time = time, .valid = true };

    //no %f in the log (see debug.h), so the time is reported in whole milliseconds
    printConsole("slot %d best %s time -> %ld ms", slot, hardMode ? "hard" : "standard",
                 (long)(time * 1000.0));
    return transferAt(offset + (long)fieldOff, &pair, TIME_PAIR_BYTES, true);
}
