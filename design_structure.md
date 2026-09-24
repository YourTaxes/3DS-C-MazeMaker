# DESIGN STRUCTURE

## PROJECT LAYOUT
* source/ - main.c and nothing else.
* source/datatypes/ - the structs the whole game shares, and the code that reads and writes them. game_state.h, level_file.h, save_file.h/.c.
* source/utils/ - stateless helpers.
* source/states/<Name>/ - one folder per state: its four functions plus anything only that state uses, eg maze game logic.
* the Makefile lists every one of these folders in SOURCES and source/ in INCLUDES. 
* important, cant't make files that have the same name.

## source/main.c
This file holds only the main loop. it chooses what state's files to run based on if the state variable
* global varibles 
    * Game_State State: init to STATE_MAIN_MENU
    * Raw_Level rawLvl, Built_Level builtLvl: stores all details about the current loaded level
    * GameContext ctx: holds at the two above, plus rebuildLevel (init to true) and curSlot (which save slot rawLvl came from, init to 0, so the maker and the game know where to write back). this is what gets handed to every state.
    * C3D_RenderTarget* top, bottom
    * static const StateFns STATES[STATE_COUNT]: one row per state, holding its init / logic / draw / end functions. 
        * to add a state: write its four functions, add one row here. nothing else in main changes.
        * states with no row yet are all NULL. the main loop refuses to switch to them and logs it.

* rebuildLevel (inside ctx) means the playable level needs to be rebuilt from the raw level. it should only be true in these 3 situations
    * when moving from the level select after selecting a different level to the main menu
    * when moving from the maze maker to the main menu when there were edits made in the maze maker (level number stays the same, but the raw data changed)
    * the game just started (now)

* initalize everything
    * init gfx
    * the debug log, no console on 3ds screen
    * C3D and C2D
    * the romfs
    * the render targets
    * the colors
    * TODO: the font
    * the save file: SaveFile_Ensure() checks sdmc:/3ds/MazeMaker/save.bin. if it is missing, the wrong size, or has the wrong magic or version, a fresh one (every slot empty) is written in its place. (see level_file.h / save_file.h)
    * rawLvl: SaveFile_ReadSlot(ctx.curSlot, &rawLvl) copies just that slot out of the file. if either step failed, rawLvl stays zeroed with empty = true and the game runs anyway.
    * one throwaway hidScanInput, so an input that is already held at boot doesn't count as a press on the first frame
    * STATES[State].init(&ctx) for the starting state. FROM HERE UNTIL THE LOOP EXITS THERE IS ALWAYS EXACTLY ONE LIVE STATE.

* the main loop
    * Input_Read fills a FrameInput (kDown, kHeld, kUp, touch, normalized circle pad). this is the only place the hid is read.
    * debug hotkeys (X input dump, B keyboard) (temporary, not going to stay)
    * next = STATES[State].logic(&in, &ctx). the state returns what should happen: STATE_NONE to stay, STATE_QUIT to leave, or the state to switch to.
    * START always sets next to STATE_QUIT, from any state.
    * start the C3D frame, STATES[State].draw(top, bottom), end the C3D frame.
    * if next is STATE_QUIT: stop looping.
    * else if next is a state: STATES[State].end(), State = next, STATES[State].init(&ctx). the old state got drawn one last time, the new one is fully set up before its first frame.
    * Y prints cpu / gpu / heap stats. printed as integers on purpose, as testing found that newlib's printf mallocs on its first %f and that would move the heap number, which I would rather not do.

* after main loop
    * STATES[State].end() - one state is always live here, whether we left by STATE_QUIT or the HOME menu.
    * TODO: if font exists, then free it
    * delete the render targets, C2D finish, C3D finish, romfsExit, gfxExit



## SHARED FILES
each one is named for what it holds. the arrows only point one way: debug <- input <- graphics, game_state <- (input, level_file), and save_file <- (level_file, debug). nothing includes something that includes it back. states include these; these never include a state.
* utils/debug.h/.c - printConsole (stderr to the debugger) and ToBinary. no game knowledge.
* utils/input.h/.c - FrameInput, Input_Read, normalizeCirclePad, printInputs, GetKeyboard. the only place the hid is read.
* utils/format.h/.c - FormatTime (seconds -> mm:ss:mmm, minutes never clamped, negatives treated as 0) and TIME_STR_LEN. converts to whole milliseconds and formats integers only, so no %f ever reaches printf.
* utils/graphics.h/.c - screen size defines, Colors[] / MakeColors, Rect + DrawRect / Rect_Contains / Rect_Tapped, MakeText / DrawTextCentered / DrawTextInRect.
* datatypes/game_state.h - Game_State enum, GameContext, StateFns. header only, this is the contract every state file implements.
* datatypes/level_file.h - tile / level / save file structs and their sizes. header only. inside Raw_Level each best time is kept immediately before its own valid flag, so SaveFile_WriteSlotTime can write the time and the flag in one go. _Static_asserts in save_file.c break the build if that order is disturbed.
* datatypes/save_file.h/.c - the only place the SD card is touched. path is sdmc:/3ds/MazeMaker/save.bin. SaveFile_Ensure (boot check / create), SaveFile_Read / SaveFile_Write (whole file, level select), SaveFile_ReadSlot (boot), SaveFile_WriteSlot (maker save), SaveFile_WriteSlotTime(slot, hardMode, time) (game, new best time: writes that one time and marks it valid). the slot functions seek to just that slot, so the rest of the file is never loaded or touched.
* when the maker palette needs a generic Button (rect + label + action), it should go in a new utils/ui.h/.c on top of graphics.h rather than growing graphics.h.

## FORMAT FOR ALL STATE SPECIFIC FILES 
every state file provides the same four functions, with the same signatures, so main can hold them in the StateFns table (game_state.h). main guarantees the order: init, then logic/draw every frame, then end. exactly once each.

* ONE STRUCT, ONE POINTER. each file defines a struct holding EVERYTHING the state owns while active (rects, C2D_Texts, the text buf handle, counters, whatever) and exactly one static pointer to it:
    ```c
    typedef struct { ... } MainMenuState;
    static MainMenuState* s; // NULL whenever the state is not active
    ```
    * this is how a state costs almost nothing when inactive: 4 bytes for the pointer. when active it is one calloc, so one heap block, so the heap number is the same every visit.
    * do NOT make separate static pointers for each thing. that was the old way and was very unprofesional. use arrays instead and loop through them.
    * data that is fixed at compile time (button labels, target states, layouts) goes in a static const table, not in the struct. it lives in the executable and costs no RAM.
    * rects are plain Rect values inside the struct, filled with compound literals: s->rect = (Rect){ .x = .., .y = .., .width = .., .height = .., .Color = .. };
    * text: the struct holds ONE C2D_TextBuf for the whole state, plus a C2D_Text per line. init does s->textBuf = C2D_TextBufNew(MAX_GLYPHS) then MakeText(str, &s->someText, s->textBuf) for each line. MakeText logs if the buffer is too small, so size MAX_GLYPHS per state and watch the log.

* void state_Init(GameContext* ctx) - called by main right after the switch to this state (or before the loop for the starting state).
    * s = calloc(1, sizeof *s), then fill it: make the text buf, parse the text, set up rects, load whatever data the state needs from ctx.
    * print sizeof the struct once so the number is known.

* Game_State state_Logic(const FrameInput* in, GameContext* ctx) - called every frame right after the inputs are read. does all the thinking for the frame: moving the player, colision, what tile was clicked, etc.
    * returns STATE_NONE to stay in this state, STATE_QUIT to leave the game, or the Game_State to switch to. that return value is the ONLY way a state changes.
    * FrameInput is read only. GameContext may be written (the maker edits rawLvl and sets rebuildLevel, etc).
    * touch: use Rect_Tapped(&rect, in). it is true only on the frame the stylus first lands inside the rect (kDown & KEY_TOUCH), so a touch that was already down when the state started, or slid in from outside, does not count. no per rect touch state to maintain.

* void state_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom) - draws both screens from s. the C3D frame is begun and ended by main.

* void state_End(void) - called by main once, after the state's last draw, when switching away or when the game exits.
    * C2D_TextBufDelete(s->textBuf); free any other big buffers the state made; free(s); s = NULL;
    * that's it. if the state only allocates through s and its text buf, End can't get this wrong.
    


## states/MainMenu/MainMenu.c/h
this file will load the currently selected save slot, and allow for switching to all other states (except for egg room and debug) 
* the buttons are a static const table MENU_ITEMS[] of { label, target state }, in top to bottom order. init, logic and draw all loop over it, so adding a button is one line. STATE_QUIT is just another target.
* layout is a handful of #defines (BUTTON_X, BUTTON_Y0, BUTTON_SPACING, BUTTON_W, BUTTON_H, HIGHLIGHT_PAD). button i is at y = BUTTON_Y0 + i * BUTTON_SPACING. all positions are hardcoded, just hardcoded once.
* the slot info block in the middle of the top screen has its own layout defines (INFO_Y0, INFO_LINE_SPACING, INFO_GROUP_GAP, INFO_TEXT_SCALE). line i is at y = INFO_Y0 + i * INFO_LINE_SPACING, plus INFO_GROUP_GAP once the time pair starts, so the two pairs read as two groups.
* MainMenuState: textBuf, the title and byline text for the top screen, the four slot info texts plus drawHardTime, a Rect and a C2D_Text per button, and cur (index of the highlighted button).
* MAIN_MENU_MAX_GLYPHS is 256. every character a state parses takes a slot (citro2d only treats '\n' specially, spaces included), and the worst case here is 179, mostly the 32 char level name and the two time lines. if a line is added, recount and round up to the next power of two.

* MainMenu_Init(GameContext* ctx):
    * calloc s, make the text buf, parse all text, fill the button rects from the layout defines, cur = 0
    * build the four slot info lines from ctx->curSlot and ctx->rawLvl into local char buffers, then MakeText each one. the buffers are locals because C2D_TextParse copies the glyphs into textBuf, so nothing has to outlive Init. this is also why the block refreshes for free: main runs Init again on every entry to the state, so coming back from the save menu shows the new slot.
        * line 1, always: "Slot N", with N as curSlot + 1 so the player sees 1-4, not 0-3.
        * line 2: "Name: Empty" on an empty slot, otherwise "Name: " and the level name. the name is printed with a precision of LEVEL_NAME_MAX_LEN, since levelName is a fixed char[32] that a full length rename leaves unterminated.
        * lines 3 and 4: "Best Standard Time: " and "Best Hard Time: ", each followed by FormatTime of the matching time, or "No time yet" when its valid flag is false.
        * on an empty slot, line 3 is "Empty level, no best times" instead and drawHardTime is false, so line 4 is not drawn at all. (drawing is skipped rather than parsing an empty string, which would trip a MakeText log line.)
    * if ctx->rebuildLevel is true, then rebuild the Built_Level from the Raw_Level and clear the flag.
        * have not determined the logic for this yet, but it should be done here.

* MainMenu_Logic(const FrameInput* in, GameContext* ctx):
    * L goes to the debug state (THIS IS DEBUG AND WILL BE CHANGED LATER)
    * up/down on the d pad or circle pad move cur, wrapping at both ends.
    * at most one button acts per frame: first check Rect_Tapped on each button in order and return that button's target; otherwise if A was pressed return MENU_ITEMS[cur].target. a tap wins over A if both happen at once.
    * otherwise return STATE_NONE.

* MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom):
    * top screen is the game title, the current slot's info block centered under it, and my name at the bottom. the info lines are held in a small array and looped over, drawing 3 of them or 4 depending on drawHardTime.
    * bottom screen: the highlight (the selected button's rect grown by HIGHLIGHT_PAD, drawn first so it sits underneath), then each button rect with its label centered in it (DrawTextInRect).
    * (the earlier plan had 3 buttons plus a "press start to exit" message. there are now 4 buttons including Quit; START still exits from anywhere.)

* MainMenu_End(void): delete the text buf, free s, s = NULL.

## MEMORY LEAK TEST. 
* temporarily set main menu to go to the game screen always, not just on A press.
* set the game logic to be to always switch to the game screen
* leave running and see if memory usage increases.

## Level Select - states/LevelSelect/
* LevelSelectState holds a Save_File (the whole file, loaded with SaveFile_Read in _Init, so the file is only in RAM while this state is active). saving a slot / rename / copy / delete edit that struct, then SaveFile_Write flushes it to disk. main and the other states never hold the file, only rawLvl. loading a slot sets ctx->curSlot.
* UPON THIS STATE BEGINING, THE PLAYER WILL BE ASKED IF THEY WANT TO SAVE THEIR SLOT BEFORE CONTINUING. 

* when the player hovers over a level with the cursor, then on the top screen, show the full layout (all 9 screens) of the level through a custom texture. 

* The currently active save slot should have it's color changed from the rest.


* at any time during this screen, the player can press X to save their game to the slot they were on. 
* vars for this state are enum action and enum "are you sure". 
* action has 3 values LOAD, RENAME, COPY, DELETE
* are you sure has values NONE, AYS1, AYS2, AYS3
* this will consist of 4 buttons in the middle of the screen, with text below them showing their names. 
* STRETCH GOAL - as part of compilation, create a static png of the entire level(all screens), and have that png be shown on the button.
* in the bottom left corner of the screen, there should be 2 small buttons, 
"rename" "copy" and "delete"
* the 3 buttons for rename copy and delete are replaced by 1 button called cancel when action is not LOAD. 
* when rename is pressed, the Action is set to rename. this puts the state in rename mode, where interacting with the save slot buttons will instead bring up a keyboard and allow the player to rename the save.
* when delete is pressed, it sets action to delete. when action is DELETE, the when the player presses the save, they get an are you sure, then another are you sure, then another. (keep track of these with the enum AYS). pressing B or no at any time during the are you sures will result in AYS being set to NONE, and the delete is cancled. if the AYS goes through, (yes pressed while AYS = AYS3) then the file is wiped, and the save's empty bool is set to true.
* if the player tries to delete a level that is already empty, nothing happens.
* when the player presses the copy button, action gets set to copy, and if the player clicks on an empty save, then the currently loaded level is copied to the empty slot. if the selected slot is not empty, then the AYS stuff happens from delete.
* when the player presses the clear times button, it is nearly the same as the delete button, but it only deletes the best times for the level


* WHEN YOU LOAD A LEVEL, THEN SET REBUILD LEVEL TO TRUE.


* when the player presses B in an are you sure section, then the are you sure is set to no and ends then the action is set to load, if they were already in the load action, then they go back to the main menu.


* there should still be dpad option for this menu. the buttons should be stored in a matrix, arranged as
[level1, level2],
[level3, level4],
[MainMenu, rename, copy, clear_times, delete],
and the cursor starts on the level 1 spot, and when left and right are pressed, then the current button moves to the left or right, and loops around if it goes off an edge, and when up and down are pressed, the current button moves up or down and loops around like the main menu, but without the enum, and only keeping track of max hight.






## Maze Game - states/Maze/
* IMPORTANT IDEA FROM KELIN - ONLY CHECK COLISION WITH TILES WITHIN 1 TILE UNIT OF THE PLAYER.
    * if player is in [1, 1], check colision with [0,0], [1,0], [2, 0], [0, 1], [2, 1], [0, 2], [1, 2], [2, 1]. 
    * keep track of what cordanate grid space the player is in by dividing their cordinates by the length of a tile. 
    * using this strategy, it may not be needed to compile the levels in the way specified before, as all of the blocks could then be drawn regularly, as the 3ds can handle a lot of blocks. 
    * the compilation would still need to define some specific things though, such as the start location, and the locations of the portals. 

* automatically save the times if they are new bests when the player reaches the victory screen.









## Maze Maker - states/Maker/
* When in the actual maze maker, DO NOT DRAW THE ENTIRE TOP SCREEN EVERY FRAME, to save preformance. instead, in the struct of state vars, keep a boolean for all 9 rooms for if they need to be drawn this frame. in init initalize all of these to true.
    * in Draw, check each of these individually, and if one is true, clear and 

* Automatically save the raw level when the player leaves the Maker