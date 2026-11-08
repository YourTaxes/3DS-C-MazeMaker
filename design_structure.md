# DESIGN STRUCTURE

## PROJECT LAYOUT
* source/ - main.c and nothing else.
* source/datatypes/ - the structs the whole game shares, and the code that reads and writes them. game_state.h, level_file.h, save_file.h/.c.
* source/utils/ - stateless helpers.
* source/states/<Name>/ - one folder per state: its four functions plus anything only that state uses, eg maze game logic.
* source/menu_navigation/ - all of the logic for the buttons in the menus.
* source/game_logic/ - all of the special logic for the act of actually playing the maze game.
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

* datatypes/save_file.h/.c - The only place the SD card is touched. path is sdmc:/3ds/MazeMaker/save.bin. SaveFile_Ensure (boot check / create), SaveFile_Read / SaveFile_Write (whole file, level select), SaveFile_ReadSlot (boot), SaveFile_WriteSlot (maker save), SaveFile_WriteSlotTime(slot, hardMode, time) (game, new best time: writes that one time and marks it valid). the slot functions seek to just that slot, so the rest of the file is never loaded or touched.

* utils/popup.h/c - Reusable instance of the are you sure popup. used in level select before destructive actions. planned to be used in the same way later in the maker.

* menu_navigation/menu_nav.h/c - the (far too object oriented) system for determining player input in relation to the menus. supports touchscreen and keys in tandem, callbacks for what to do when buttons are interacted with, a draggable mode where the highlight follows a held stylus, and handing context to a neighbouring menu through the MENU_EXIT_* link sentinels. a menu with no neighbours linked behaves exactly as it always did, so one menu on a screen needs none of that.

* menu_navigation/menu_group.h/c - several menus on one screen, with exactly one holding keyboard context and touch routed to whichever one the stylus landed in. it owns the menus it builds, so "every menu has to exist before linking" cannot be gotten wrong. one group at a time. a single menu does not need this.


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
    * calloc s, make the text buf, parse all text, fill the button rects from the layout defines, initialize the menu_nav struct.
    * build the four slot info lines from ctx->curSlot and ctx->rawLvl into local char buffers, then MakeText each one. the buffers are locals because C2D_TextParse copies the glyphs into textBuf, so nothing has to outlive Init. this is also why the block refreshes for free: main runs Init again on every entry to the state, so coming back from the save menu shows the new slot.
        * line 1, always: "Slot N", with N as curSlot + 1 so the player sees 1-4, not 0-3.
        * line 2: "Name: Empty" on an empty slot, otherwise "Name: " and the level name. the name is printed with a precision of LEVEL_NAME_MAX_LEN, since levelName is a fixed char[32] that a full length rename leaves unterminated.
        * lines 3 and 4: "Best Standard Time: " and "Best Hard Time: ", each followed by FormatTime of the matching time, or "No time yet" when its valid flag is false.
        * on an empty slot, line 3 is "Empty level, no best times" instead and drawHardTime is false, so line 4 is not drawn at all. (drawing is skipped rather than parsing an empty string, which would trip a MakeText log line.)
    * if the current slot is empty, then grey out the maze game button, and return here.
    * if ctx->rebuildLevel is true, then rebuild the Built_Level from the Raw_Level and clear the flag.
        * have not determined the logic for this yet, but it should be done here.

* MainMenu_Logic(const FrameInput* in, GameContext* ctx):
    * L goes to the debug state (THIS IS DEBUG AND WILL BE CHANGED LATER)
    * R goes somewhere special.
    * do the call the menu_nav logic here, return what it gave, 
    * otherwise return STATE_NONE.

* MainMenu_Draw(C3D_RenderTarget* top, C3D_RenderTarget* bottom):
    * top screen is the game title, the current slot's info block centered under it, and my name at the bottom. the info lines are held in a small array and looped over, drawing 3 of them or 4 depending on drawHardTime.
    * bottom screen: the highlight (the selected button's rect grown by HIGHLIGHT_PAD, drawn first so it sits underneath), then each button rect with its label centered in it (DrawTextInRect).
    * (the earlier plan had 3 buttons plus a "press start to exit" message. there are now 4 buttons including Quit; START still exits from anywhere.)

* MainMenu_End(void): delete the text buf, free menu_nav, free s, s = NULL.

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
* on load, create a static png of the entire level(all screens), and have that png be shown when the button is highlighted.
* in the bottom left corner of the screen, there should be small buttons, 
"rename" "copy" and "delete"
* when rename is pressed, the Action is set to rename. this puts the state in rename mode, where interacting with the save slot buttons will instead bring up a keyboard and allow the player to rename the save.
* when delete is pressed, it sets action to delete. when action is DELETE, the when the player presses the save, they get an are you sure, then another are you sure, then another. (keep track of these with the enum AYS). pressing B or no at any time during the are you sures will result in AYS being set to NONE, and the delete is cancled. if the AYS goes through, (yes pressed while AYS = AYS3) then the file is wiped, and the save's empty bool is set to true.
* if the player tries to delete a level that is already empty, nothing happens.
* when the player presses the copy button, action gets set to copy, and if the player clicks on an empty save, then the currently loaded level is copied to the empty slot. if the selected slot is not empty, then the AYS stuff happens from delete.
* when the player presses the clear times button, it is nearly the same as the delete button, but it only deletes the best times for the level


* WHEN YOU LOAD A LEVEL, THEN SET REBUILD LEVEL TO TRUE.


* when the player presses B in an are you sure section, then the are you sure is set to no and ends then the action is set to load, if they were already in the load action, then they go back to the main menu.





## Maze Game - states/Maze/
* IMPORTANT IDEA FROM KELIN - ONLY CHECK COLISION WITH TILES WITHIN 1 TILE UNIT OF THE PLAYER. (or more if the player is bigger than 1 tile)
    * if player is in [1, 1], check colision with [0,0], [1,0], [2, 0], [0, 1], [2, 1], [0, 2], [1, 2], [2, 1]. 
    * keep track of what cordanate grid space the player is in by dividing their cordinates by the length of a tile. 
    * using this strategy, it may not be needed to compile the levels in the way specified before, as all of the blocks could then be drawn regularly, as the 3ds can handle a lot of blocks. 
    * the compilation would still need to define some specific things though, such as the start location, and the locations of the portals.

* automatically save the times if they are new bests when the player reaches the victory screen.

* there should be 2 portal types, the blue and orange ones.
    * if more than 2 are placed, then the one closer in the linear list to the front should be the main, which all others go to, and the 
    * if there is only one portal in a set, then it doesn't do anything.

* when a key has been collected for a door, then do not pass that door's rect into the SURROUNDING list for moving the player.

* NEW IDEA - Platformer mode 
    * have the player affected by gravity.
    * have this be a toggle in the save file
    * only possible with standard tile grid, check if colliding with any of the 3 bottom tiles.
    * every frame, decrease the downward velocity by a set amount.
    * if on ground, then set ground velocity to 0
    * if press A on ground, set ground velocity to a large number. 







## Maze Maker - states/Maker/
* When in the actual maze maker, DO NOT REBUILD THE ENTIRE TOP SCREEN EVERY FRAME, to save preformance.

* When the player presses Y in the maker, save the slot.s

* press x in the maker, and the the keyboard comes up and the player can enter the name for their level. it is only written to the level in memory, not in storage, so it needs to be saved.

* player presses b, they go to the main menu, presses L, go to level select

* if the player presses ZL/ZR, then they cycle through the pallete.

* if the player tries to leave to the level select or the main menu, but has not saved, then bring up a popup asking if they want to save.

* there needs to be a system to make sure that the player does not place more than 2 of each type of portal.






## Forgotten 
* when the player enters this state, the player spawns on the bottom screen, and is rendered as the same 2d sprite as always. 
* on the top screen is the 3d model of the tree, split into the trunk and it's top. the top sways from left to right, and has a texture of the tree top.
* the player is expected to move to the top of the bottom screen, and go accross the barrier between screens.
* when the player is on the top screen, they are rendered as a 3d cube. 
* the boarder of the top screen (asside from the seam between top and bottom) should have invisible walls, preventing the player from leaving. the bounding walls should extend to cover both screens, and there should be walls where the bottom screen is. 
* the actual code for moving the player should be the same on both screens, but just have the player sprite be replaced by a 3d cube.
* have the tree be layered above the player.
* if the player is FULLY off screen on either screen, then don't draw them on the respective screen. if they are between, then render them on both. 
* have an invisible hitbox behind the tree, and if the player touches it, then they lose controll of the player, and a text box appears, and follows the same logic as the Are you sure screen, but on the top screen and there is no exiting, only continuing. 
* when the text finishes, then the screen should fade to CLR_CLEAR, then switch to the main menu


* look into the stensil buffer to get the effect of the tree being outlined red.

* I need to make a cover of the egg room theme, and play it here


* IMPORTANT NEW INFO - I can use the .obj file type, which has the array of vertexes in it somewhere aparently.


* EXTREMELY IMPORTANT - NEED TO NORMALIZE THE DPAD INPUT








## Menu Navigation
the multi menu update is IMPLEMENTED. menu_nav now supports several menus on one screen,
and menu_group is the thing a state talks to when it has more than one.

* MENU_NONE is now a legal value for `selected`, meaning "this menu does not have context".
  a menu in that state ignores the keys entirely, still reads the touchscreen, and draws
  nothing. that one change is what makes everything else possible, and it is why
  select_index is the only place `selected` is ever written.

* exiting a menu with the keys: the MENU_EXIT_LEFT / RIGHT / UP / DOWN sentinels go in a
  MenuDef's link slots, the same place a button index goes. the SENTINEL names the
  neighbour to hand off to, not the slot it sits in, so `.left = MENU_EXIT_UP` legally
  means "a left press hands context to the menu above me".
    * they are derived from MenuDir (`MENU_NONE - 1 - dir`), NOT the -2/-3/-4/-5 order the
      old plan listed, so sentinel <-> direction is arithmetic instead of another table
      that has to agree with DIR_KEYS and link[]. pinned with a _Static_assert like the rest.
    * step() hands a sentinel straight back to the caller rather than following it, because
      cursor_ok would index buttons[-4]. menu_nav_logic then BREAKS out of the direction
      loop, because a second pass would feed that sentinel into step() as `from`. that break
      is the whole crash, and it is the one line to not lose.

* the handoff releases context BEFORE asking the neighbour, and restores it if the
  neighbour refuses, so a refused exit moves nothing at all and a degenerate self link is
  harmless. a refusal happens when there is no neighbour that way, when the neighbour's
  enter[side] is MENU_NONE (that side is sealed), or when its entry button is disabled and
  one step onwards fails. one hop only, always, or two menus pointing at each other bounce
  context forever.

* the menu owns the handoff, not the group, and it reports it as MENU_EV_EXIT with
  MenuEvent.menu set to whoever holds context now. that is deliberate: menu_nav_link is
  public, so two menus can be linked with no group at all, and if the group owned the
  handoff then linking would silently do nothing without one. the group only watches for
  MENU_EV_EXIT to keep its own index in step.

* enter[MENU_DIR_COUNT] on the config is button INDEXES, not the MenuDef pointers the old
  plan called for. a pointer cannot be turned back into an index without arithmetic on the
  table base, and the index is what pairs a button with rects[i] and what every callback is
  handed, so a pointer would break the one thing the whole file is built on.

* `draggable` on the config swaps kDown for kHeld in the hit test, so the highlight follows
  a held stylus. on_click still fires ONLY on the frame the stylus went down, because
  otherwise a held stylus fires the button once per frame, which in MainMenu shaped code is
  a state change per frame. on_highlight is already contractually idempotent, so that is
  the per frame hook for anything tracking a drag.

* menu_nav_hit_test is public and is the ONE hit test. menu_nav_logic uses it and so does
  the group, so the two can never drift apart about which menu owns a tap. if they did, a
  tap would be lost AND context would move to a menu the user did not hit. Rect_Tapped is
  not used here because it hardcodes kDown, which draggable needs to override.

* MenuConfig is now ZERO INITIALIZED by the caller (`MenuConfig cfg = {0};`) instead of
  "all fields required". it grows over time, and both consumers were filling it field by
  field from an uninitialized local, so adding `draggable` to it would have read whatever
  was on the stack. a zeroed config enters on button 0 from every side, which is a fine
  default, and MENU_NONE is how a side gets sealed.

### menu_navigation/menu_group.h/c
the handler for several menus on one screen, and the thing a state with more than one menu
actually calls. one group at a time; a second menu_group_init fails and logs rather than
replacing the first, because the caller is still holding that handle (unlike popup_init).

* the group OWNS its menus. it takes an array of MenuConfigs and a MenuLink table, builds
  every menu, wires them, and frees them. that is the point: linking needs every menu to
  exist first, and if the group does the building then that ordering cannot be gotten wrong.
  a state must not free a grouped menu itself.
* MenuLink is the same union trick as MenuDef, for the same reason, with the same assert:
  the table is written with .left/.right/.up/.down and read as side[MenuDir].
* one frame: a touch that BEGINS this frame moves context to the menu it landed in, then
  only the context menu runs its logic, which handles that same touch normally. so there is
  one highlight, one set of callbacks, and no second touch code path.
    * routing is gated on kDown and never kHeld, even for draggable menus, so a drag that
      slides off a draggable menu's edge keeps the gesture on the menu it started on.
    * a tap has no direction, so there is no enter{side} for it. the tapped menu has no
      selection, and its own logic lands the cursor on the button the stylus actually hit,
      which beats any guess enter[] could make.
    * menus are scanned in array order and the first hit wins, so menus should not overlap,
      and where they do, array order is the priority order.
* menu_group_focus is the escape hatch for a state that wants to drive focus itself.

### still to do
* a test state with three menus (one top screen keyboard only, one plain list, one
  draggable) to exercise the handoff, the refusals, and the drag firing rule.
* restoring the previous selection on re entry, as a MENU_ENTER_LAST sentinel plus an
  int lastSelected updated in select_index. the old plan called this `last_used`; it was
  dropped because the group tracks context and the menus do not need to. purely additive.
* note that live[] flags are OR'd on top of the table's, so a runtime flag can only ever
  ADD a restriction: set_enabled(.., true, ..) cannot re-enable a button the static MenuDef
  table disabled.




## STRETCH GOAL - STREETPASS

### Explination of funciton
* My vision for this is that you can setup streetpass for the game, and when you walk by someone else that has streetpass on for the game, then you will exchange levels. these levels will the be accessable through a secondary menu in the level select menu, which allows you to swap save files (only for that session)

* REQUIREMENTS

  * Save file should be perfectly ready to go already to be sent, because limit is 11kb, and save is 9. If the data does not already have the sending player's name, then it will need to be included in the save file.

  * SUPER STRETCH GOAL - the times for the origional creator and you could be saved seperately, possibly in a "Streetpass Level pack" Struct, so that you could get a special thing when you finish the level faster than the creator.

  * to actually handle recieving, the save file probably needs to be edited to include the amount of streetpass level packs you already have.

  * Bool in the save file that denotes if streetpass is on
  * Have a button glyph on the main menu saying "X: Streetpass", which brings up popup that has multiple pages explaining how streetpass will work, then the last one asks to set it up. 
    * if they say yes and streetpass is off, then do the setup for a homebrew app to setup streetpass
    * if they say yes and streetpass is on, do nothing. 
    * if they say no and streetpass is off, do nothing.
    * if they say no and streetpass is on, disable it.

  * in level select, have a glyph that says "X: Streetpass Levels", only if the user has streetpass enabled.

  * when the user presses X on the level select screen with streetpass enabled, they are taken to the Streetpass Menu. 

    * There will be a limit on the amount of streetpass tags you can have, which is to be decided. most heavily likely to be one menu page's worth, so probably 6. (not including the main save)

    * the buttons on the bottom screen are going to be selectors for all of the different people you have met, and the top screen will show *something*, unknown as of now. if the mii is included in the streetpass data, it sould be shown. 

    * there should be a indicator "X: Delete Level Set"
      * if the user presses X, a popup (very similar delete in level select) asks "Would you like to Delete {player name}'s Level Set?".
        * pressing yes does this, which clears up the slot for another streetpass level.


  * Streetpass levels **WILL NOT BE EDITABLE**, but you can play them all you like.
    * GameContext needs a boolean 'Level Editable', which is false when you are playing on a streetpass save.
      * when this is false, the maker button on the main menu will be greyed out and inaccessable.

  



# TODO FOR CIA VERSION 

## Custom banner

* make this in blender
* probably the game title on top, on the bottom make sklumbo in the background.
* also need to make sound for this, I'll come up with something.

## Custom E-Manual
I think I know how to do this. 
Found a guide for it.





# FUTURE IDEAS 

## NFC / AMIIBO
* in main on startup call "NFC Init" which first calls CFGU_IsNFCSupported(), which checks if the nfc functionality is avalible at all, and writes that bool to game context.

* face button on the main menu takes you to a special menu for amiibos.
  
* there will be a new scene that is the amiibo menu, which has 2 buttons, that each take you to a different scene.
  * the first one is the one that takes you to the amiibo rewards. this is always avalible.
  * the second one takes you to the scanning scene
    * this button will be greyed out if the nfc support is false.


* the rewards sceene will look similar to the level select scene, but the levels are not copyable or renameable or deletable, and the levels will be saved in romfs, rather than your save file. 
  * when one of these levels is selected, the maker will be greyed out, and editable_level will be set to false.
  * on this scene, you will be able to do the button combination (UP + X + SELECT) to get all of the amiibo rewards. this is to prevent the requirement for the actual amiibos. 

* the scanning scene is pretty self explanitory. it is where you scan amiibos to get rewards.

* there will be a specific set of amiibos that will give a reward. they will probably include kirby, and 3 others.

