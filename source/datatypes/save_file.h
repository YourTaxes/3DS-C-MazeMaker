#pragma once

#include "datatypes/level_file.h"

//forward definition of Save_File
//typedef struct Save_File Save_File;
//typedef struct Raw_Level Raw_Level;

/*
* The save file lives on the sd card, accessed through sdmc if it is run through the homebrew launcher,
* More likely, if it is run through the cia or cci, then it will have it's own system savedata archive
* */
#define SD_SAVE_DIR  "sdmc:/3ds/MazeMaker"
#define SD_SAVE_PATH SD_SAVE_DIR "/save.bin"
#define ARCHIVE_SAVE_PATH "save:/save.bin"

//sizes of the cia/cci save archive in 512 byte blocks, for FSUSER_FormatSaveData.
//comes from SAVEFILE_SIZE in version.env, and is updated with the makerom
#define SAVE_BLOCKS ((_SAVEFILE_SIZE_ * 1024) / 512)

//DuplicateData keeps 2 copies, which I am doing because my save is so incredibly small,
//so it makes half the archive usable. make sure I will not end up using that much
_Static_assert(sizeof(Save_File) * 2 + 0x2000 <= _SAVEFILE_SIZE_ * 1024, "Save_File no longer fits in the save archive, raise SAVEFILE_SIZE in version.env");

/*
* pick where the save data lives
* gets called once on boot, before anything else with savefile
* cia/cci: mounts the title's save archive as "save",  and formats on the first launch
* 3dsx: saves and reads from the sd card.
* saves will not transfer between versions*/
void SaveFile_Mount(void);

/*
* Unmounts the save archive if it is in use.
* gets called on exit
*/
void SaveFile_Unmount(void);


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
bool SaveFile_WriteSlotTime(int slot, bool hardMode, u32 time);

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
