#pragma once

#include <3ds.h>
#include "datatypes/level_file.h"

/*
* the save file lives on the SD card. this is the only place the SD card is touched.
* a .3dsx has no save archive of its own, so the homebrew convention of a folder
* per app under /3ds/ is used. libctru mounts sdmc: on its own before main.
*/
#define SAVE_DIR  "sdmc:/3ds/MazeMaker"
#define SAVE_PATH SAVE_DIR "/save.bin"

/*
* make sure a valid save file exists. creates a fresh one (every slot empty) if the
* file is missing, the wrong size, has the wrong magic, or the wrong version.
* returns false only if the fresh file could not be written.
*/
bool SaveFile_Ensure(void);

/*
* read the whole save file from disk into *out. false on any failure.
* level select calls this in its _Init into a Save_File inside its state struct.
*/
bool SaveFile_Read(Save_File* out);

/*
* write the whole save file to disk. false if it could not be fully written.
* level select calls this whenever it saves a slot.
*/
bool SaveFile_Write(const Save_File* file);

/*
* copy save slot `slot` (0..LEVEL_SLOT_CNT-1) from disk into *out without loading
* the rest of the file. false on any failure. used at boot to fill rawLvl.
*/
bool SaveFile_ReadSlot(int slot, Raw_Level* out);

/*
* overwrite save slot `slot` on disk with *lvl, leaving the rest of the file alone.
* false on any failure. the maker calls this to save the level it is editing.
*/
bool SaveFile_WriteSlot(int slot, const Raw_Level* lvl);

/*
* overwrite one best time of save slot `slot` on disk, standard or hard, and mark it
* valid. nothing else in the slot is touched. false on any failure.
* the game calls this after a level is completed with a new best.
*/
bool SaveFile_WriteSlotTime(int slot, bool hardMode, double time);

/*
* read the slot the player was last on from disk into *out, without loading the rest
* of the file. false on any failure, including a stored slot outside 0..LEVEL_SLOT_CNT-1,
* in which case *out is left alone and the caller should fall back to its own default.
*/
bool SaveFile_ReadLastSlot(int* out);

/*
* record `slot` (0..LEVEL_SLOT_CNT-1) as the slot the player was last on, leaving the
* rest of the file alone. false on any failure.
*/
bool SaveFile_WriteLastSlot(int slot);
