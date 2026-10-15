#pragma once

typedef struct {} Rect;
typedef struct {} FrameInput;
typedef struct {} C3D_RenderTarget;

//this class will be used to define a struct for the menu buttons, 
//and they will act as a psudo linked list, 
//with each button having a reference to the other buttons on the sides of it,
//allowing one function to be called to handle the logic for each button.
//each button struct will have a function for what it does if it is pressed, which the caller provides on init.

//this as of now is only used in level select and main menu,
//but I am making it extendable so that it could be used in things like the maker and other things,
//including other 3ds projects.


typedef struct MenuButton{
    Rect* curRect;
    struct MenuButton* up;
    struct MenuButton* down;
    struct MenuButton* left;
    struct MenuButton* right;
    //if either of these function pointers are null, then calling them is skipped
    //function pointer to function to call when the button is pressed
    //passes a reference to the current menu button that was clicked, so that it can change itself in it's logic
    void (clicked_button)(struct MenuButton* self);
    //function pointer to function to call when the button is highlighted. 
    //not used in level select or main menu yet, but could be.
    void (cursor_highlighted)(struct MenuButton* self);
}MenuButton;

//in c file, have a static struct pointer that will hold all of the stuff for this logic.
//it should hold a Rect, which is the highlight box, which will be driven by the c file 
//it should also have a pointer (which will act as an array) of menu buttons, which is also malloced in init, and the length of the list is determined by the length of the button list given to init.
//starting after the copy in init, the actual buttons will live in this struct, and the calling file will use references to them.
//also hold an int that is the button count
//should also have a different pointer to the currently highlighted button, which is set in init.





//init function
//initalizes the struct that holds all of the info for the menu
//then, mallocs the list of buttons to the length provided,
//then, copies everything from the provided list of menuButtons to the newly malloced one
//then sets the current highlited button to the one that is passed in. 
//return true if successfull, return false if anything fails.
bool menu_nav_init(MenuButton* init_list, int list_len, MenuButton* init_selected);








//menu_get_current_selected
//returns pointer to current selected button
MenuButton* menu_nav_get_highlighted();






//menu_set_selected
//helper func that sets the cursor to a specific menu button
//takes in pointer to said menu button, so it can call cursor highlighted.
//calls it's cursor_highlighted function, if it has one.
//also moves the highlight rect to the button, and sizes it correctly.
void menu_nav_set_selected(MenuButton*);



//per frame logic
//this will take in the frame input.
//if the player did an action that would change highlighted butons, then change them

//actual flow
//menuButton *nextButton = NULL;
//bool immedieteClick = false;

//on dpad or cpad move, 
//and the current button's struct has a non null button in that direction, then set nextButton to the the pointer to that button, 
//unless it is null, then do nothing. 


//if there is touch screen pressed, then loop through the list and use the pointers to the rectangles to find if any of them intersected.
//the first one that intersects will:
//set next button to the pointer to that button,
//set immedieteClick to true
//break out of loop



//if nextButton still == NULL, then nothing happened, return

//if it is not NULL, then call menu_set_selected(nextButton);


//if immedieteClick is true or if the player pressed the A button this frame,
//call the current selected button's clicked_button funciton.

//return true if the menu button changed, and false if it didn't
bool menu_nav_logic(FrameInput* in);


//draws the cursor. needs to be called before the actual buttons.
void menu_nav_draw(C3D_RenderTarget* target);

//free function
//frees the list of buttons, and then frees the struct.
void menu_nav_free();