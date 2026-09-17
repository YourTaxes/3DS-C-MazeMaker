# DESIGN STRUCTURE

## main.c/h
This file will hold only the main loop for the game. It will do this
* initalize everything
    * the state global variable
    * the global switched_state variable 
        * this varaible will be set to true whenever the state switches, so that on the next frame, the new state can run it's init code.
        * this is always initalized to true when the game starts
    * the global variable that indicates the current loaded and coalesced level is
    * the global variable that indicates if the current playable level needs to be rebuilt from the raw level. 
        * should only be true in these 2 situations
            * when moving from the level select after selecting a different level to the main menu
            * when moving from the maze maker to the main menu when there were edits made in the maze maker (level number stays the same, but the raw data changed)
    * init gfx
    * the debug log, no console on 3ds screen
    * C3D and C2D
    * the romfs
    * the font
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
    * have the buffer for each text object be a global varible, so it can be accessed by the logic and drawing functions
    * need to make a rectangle struct (holds only float x, float y, float width, float height, u32 Color), and each rectangle (or set of rectangles) will be a static global variables.

* bool state_logic(GameState* state) - this is called in the main loop right after checking inputs. this function will determine what needs to be done and calculate the values, doing things such as moving the player and checking colision, or determining what tiles were clicked on and determining what should be done with that.
    * returns the State Switches Boolean, so the main loop knows weather or not to do the state switching code. 

* state_init() - this is called inside of the state_logic function, but only on the frame where switched_state is true, this should be the first thing that is done in the frame logic code.
    * this initalizes all of the variables
    * IMPORTANT - THIS INITALIZES ALL OF THE GLOBAL VALUES FOR THE FILE BY MALLOCING THEM. DON'T SCREW THIS UP. Also will copy neccicary data into those malloced instances. 
    * this loads the neccicary data into the correct places
    * it's an init function

* state_end(GameState* state) - this is only called in the main function if the state_logic returned true, meaning "State_Switches" is true.
    * reads the global variable "Next_State" and sets the current state to the next state. 
    * EXTREMELY IMPORTANT DO NOT FUCK UP - FREES ALL GLOBAL VARIABLES AND EVERYTHING CREATED IN INIT, CLEANS UP THE STATE PERFECTLY. WILL NEED A LOT OF TESTING. THE STATE SHOULD TAKE UP ALMOST NO MEMORY WHEN IT IS NOT THE ACTIVE STATE.
    


## Main_Menu.c/h
this file will load the currently selected save slot, and allow for switching to all other states (except for egg room and debug) 