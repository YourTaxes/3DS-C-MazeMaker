

#define LEVEL_SLOT_CNT 4

//the structure for the level data.
typedef struct {

} Level_Data;

//The structure for the save file.
//this is the the struct that is written to the save file
typedef struct {
    Level_Data Levels[LEVEL_SLOT_CNT];
    
} Save_File;