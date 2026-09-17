# DESIGN STRUCTURE

## main.c/h
This file will hold only the main loop for the game. It will do this
* global varibles 
    * Game_State State: init to MainMenu
    * bool State_Switch: init to true
    * bool Rebuld_Level: init to true
    * Raw_Level rawLvl: init to NULL
    * Built_Level builtLvl

* initalize everything
    * State
    * State_Switch 
        * this varaible will be set to true whenever the state switches, so that on the next frame, the new state can run it's init code.
        * this is always initalized to true when the game starts
    * the global variable that indicates if the current playable level needs to be rebuilt from the raw level.
        * should only be true in these 3 situations
            * when moving from the level select after selecting a different level to the main menu
            * when moving from the maze maker to the main menu when there were edits made in the maze maker (level number stays the same, but the raw data changed)
            * the game just started (now)
    * init gfx
    * the debug log, no console on 3ds screen
    * C3D and C2D
    * the romfs
    * the font
    * find the save file. if none exists, then create it.
    * raw_lvl: init this to the level in the first save slot. 
    * initalize the builtLvl to NULL, it will be initalized on the first frame.
    * call the init functions of all other files
    * the colors
    * the variables that hold the frame's inputs



* the main loop
    * first, it will check all inputs for the frame, and normalize the circle pad
    * do all logic for the frame, do this by calling the function for the corresponding frame's state, and record weather or not the state is changing
    * start the C3D frame
    * do the render function for the selected state. 
    * end C3D frame
    * if the state frame logic returned true, then do the logic for frame switching.
    * gspWaitForVBlank

* after main loop
    * if font exists, then free it
    * C2D finish
    * C3D finish
    * gfxExit
    * exit


## FORMAT FOR ALL STATE SPECIFIC FILES 
these files all should follow a similar layout, with these steps and functions
* STATIC global variables for each file, so that the values are shared between the functions without having to go through the main loop. 
    * INCREDIBLY IMPORTANT: ALL OF THESE STATIC VARIABLES (with the exception of datatypes smaller than 8 bytes) MUST BE POINTERS, AND IN state_init THEY ARE ALL SET TO MALLOCED REGIONS OF THE RIGHT SIZE. this ensures that when the state is not active, the memory usage is minimal.
    * global static bool "State_Switches" for if the actions this frame cause a state switch, initalized to false at start
    * global static GameState "Next_State" for what state the game will be set to next, initalized to -1 at start, and only set to a value when the variable above is set to true
    * need to make a rectangle struct (holds only float x, float y, float width, float height, u32 Color), and each rectangle (or set of rectangles) will be a static global variables.
    * static C2D_Text* and C2D_TextBuf* for each line of text on screen
        * the text buf will be created in init, and each text buff should be immedietly filled by with it's corresponding text using 
        buff = C2D_TextBufNew(strlen(str));
        C2D_TextBufClear(buff);
        const char* indicator = C2D_TextFontParse(result, *font, buff, str);
        C2D_TextOptimize(result);

* bool state_logic() - this is called in the main loop right after checking inputs. this function will determine what needs to be done and calculate the values, doing things such as moving the player and checking colision, or determining what tiles were clicked on and determining what should be done with that.
    * returns the State Switches Boolean, so the main loop knows weather or not to do the state switching code. 

* state_init() - this is called inside of the state_logic function, but only on the frame where switched_state is true, this should be the first thing that is done in the frame logic code.
    * this initalizes all of the variables
    * IMPORTANT - THIS INITALIZES ALL OF THE GLOBAL VALUES FOR THE FILE BY MALLOCING THEM. DON'T SCREW THIS UP. Also will copy neccicary data into those malloced instances. 
    * this loads the neccicary data into the correct places
    * it's an init function

* state_end(GameState* state) - this is only called in the main function if the state_logic returned true, meaning "State_Switches" is true.
    * reads the global variable "Next_State" and sets the current state to the next state. 
    * EXTREMELY IMPORTANT DO NOT FUCK UP - FREES ALL GLOBAL VARIABLES AND EVERYTHING CREATED IN INIT, CLEANS UP THE STATE PERFECTLY. WILL NEED A LOT OF TESTING. THE STATE SHOULD TAKE UP ALMOST NO MEMORY WHEN IT IS NOT THE ACTIVE STATE. free text buffs with C2D_TextBufFree
    


## Main_Menu.c/h
this file will load the currently selected save slot, and allow for switching to all other states (except for egg room and debug) 
* Globals 
    * static bool StateSwitch: init to false in code
    * static GameState* NextState: init to null in code and on init
    * static text buff and text as described in general state outline.

* All text and button positions will be hardcoded.

    

* bool MainMenu_logic(u32 kDown, bool Rebuild_Level, Raw_Level* rawLvl, Built_Level* builtLvl):
    * calls MainMenu_Init(Rebuild_Level, rawLvl, builtLvl)
    * checks if the player pressed or touched the a button to start the maze game, if they did, set "StateSwitch" to true, and set "NextState" to STATE_MAZE_GAME.
    * check if the player pressed or touched the button to go to the maze maker, if they did, set "StateSwitch" to true, and set "NextState" to STATE_MAZE_MAKER.
    *check if the player pressed or touched the button to go to the level select, if they did, set "StateSwitch" to true and set "NextState" to STATE_SAVE_SELECT.
    * return StateSwitch

* MainMenu_Init(bool Rebuild_Level, Raw_Level* rawLvl, Built_Level* builtLevel):
    * initalizes stateswitch to false
    * sets nextState to malloc(sizeof(Game_State))
    * create all of the text buffs as explained above
    * if Rebuild_Level is true, then rebuld the Built_Level from the Raw_Level. 
        * have not determined the logic for this yet, but it should be done here.

* MainMenu_Draw(): Probably the first thing to implement. draws everything for the main menu.
    * Use a custom Rectangle struct to hold the values for the buttons.
    * top screen is Game title, and my name below it. 
    * bottom screen is split into 4 horizontal spaces, with the top 3 being buttons to switch states and the last being a message to press start to exit.

* MainMenu_End(GameState* state): if this was called, it means StateSwitch was true.
    * free EVERY Global variable. they will get initalized again when the player comes back to the main menu.
    * set *state to *NextState.

## MEMORY LEAK TEST. 
* temporarily set main menu to go to the game screen always, not just on A press.
* set the game logic to be to always switch to the game screen
* leave running and see if memory usage increases.