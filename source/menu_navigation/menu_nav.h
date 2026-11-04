#pragma once

#include "datatypes/game_state.h"
#include "datatypes/graphics_types.h"
#include "utils/input.h"

typedef struct MenuNav MenuNav;

//this class will be used to define a struct for the menu buttons, 
//the caller describes the menu once as a static const matrix of MenuDefs,
//then hands that table and it's own rect array to menu_nav_create, and from then on
//calls the logic, draw, and destroy.

//the table is made using the MENU_BTN and MENU_BTN_FULL macros,
//and one used for each button.
//MENU_BTN_FULL has more settings, which are by default all false normally.

//the callback return values can be different datatypes, 
//and more can be added by extending the union in the MenuEvent
//use the menu_* constructors to construct the different types to return in the callbacks

//an example of an implementation in a state file is 
/*
* static MenuEvent on_click_btn(int indes, void* user){
*   (void) user; this is a simple example
*   switch(index){
*       case BTN_0:
*           return menu_value(0);
*       case BTN_1:
*           return menu_value(1);
*       case BTN_2:
*           return menu_value(2);
*   }
*   return menu_handled();
* }
*
* static const MenuDef DefTable[buttonCount] = {
*   [0] = MENU_BTN(MENU_NONE, MENU_NONE, BTN_2, BTN_1, on_click_btn),
*   [1] = MENU_BTN(MENU_NONE, MENU_NONE, BTN_0, BTN_2, on_click_btn),
*   [2] = MENU_BTN(MENU_NONE, MENU_NONE, BTN_1, BTN_0, on_click_btn)
* }
*/

//things that this file uses but does not own, 
//1. the MenuDef table, which is static and is never going to change
//2. the rect array, which is in the caller's struct, so it should already be fine
//3. the user pointer, handed back to every callback
//button *i* uses rects[*i*], so the caller's button enum is the index to both, and is what every callback is passed

//this as of now is only used in level select and main menu,
//but I am making it extendable so that it could be used in things like the maker and other things,
//including other 3ds projects.


//standin for no link at a position, because 0 is taken already as an index
#define MENU_NONE -1

//macros for defining menu buttons,
//need to assign all links manually, because 0 appears as the first button
//to have a non link, ise MENU_NONE
#define MENU_BTN(l, r, u, d, click) {.left = (l), .right = (r), .up = (u), .down = (d), .on_click = (click)}
#define MENU_BTN_FULL(l, r, u, d, click, hover, flg) {.left = (l), .right = (r), .up = (u), .down = (d), .on_click = (click), .on_highlight = (hover), .flags = (flg)}

//the indexes of the 4 buttons when creating the table for the buttons
typedef enum {
    MENU_DIR_LEFT,
    MENU_DIR_RIGHT,
    MENU_DIR_UP,
    MENU_DIR_DOWN,
    MENU_DIR_COUNT
} MenuDir;


/*
* a link slot holding one of these means "pressing that way leaves this menu",
* handing context to the neighbour menu named by menu_nav_link.
*
* the SENTINEL names the neighbour to hand off to, NOT the slot it sits in:
* putting MENU_EXIT_UP in .left legally means "a left press hands context to the
* menu above me". the slot only decides which key triggers it.
*
* these are derived from MenuDir so sentinel <-> direction is arithmetic instead of
* another table that has to agree with the others, the same way DIR_KEYS is indexed.
* they sit below MENU_NONE, so the one existing "is this a real button index" test
* still separates links from non links.
*/
#define MENU_EXIT(dir)        ((s8)(MENU_NONE - 1 - (int)(dir)))
#define MENU_EXIT_LEFT        MENU_EXIT(MENU_DIR_LEFT)   /* -2 */
#define MENU_EXIT_RIGHT       MENU_EXIT(MENU_DIR_RIGHT)  /* -3 */
#define MENU_EXIT_UP          MENU_EXIT(MENU_DIR_UP)     /* -4 */
#define MENU_EXIT_DOWN        MENU_EXIT(MENU_DIR_DOWN)   /* -5 */

//is this link an exit sentinel, and which direction does it hand off in
#define MENU_LINK_IS_EXIT(l)  ((l) < MENU_NONE && (l) >= MENU_EXIT(MENU_DIR_COUNT - 1))
#define MENU_LINK_EXIT_DIR(l) ((MenuDir)(MENU_NONE - 1 - (int)(l)))

//the flags each button can have
typedef enum {
    MENU_BTN_NORMAL = 0,
    //the cursor steps over the button instead of landing on it, 
    //here because it does have different behavior than just not linking the button in the linked matrix
    MENU_BTN_NO_CURSOR = 1 << 0,
    //ignore taps on the button
    //IF BUTTON IS ON THE TOP SCREEN, THEN THIS NEEDS FLAG NEEDS TO BE SET
    MENU_BTN_NO_TOUCH = 1 << 1, 
    //makes tapping the button not immedietly click it, only move the cursor to it
    MENU_BTN_NO_TAP_CLICK = 1 << 2,
    //literallly makes the button unusable, but someone may want it so why not
    MENU_BTN_DISABLED = MENU_BTN_NO_CURSOR | MENU_BTN_NO_TOUCH,
} MenuButtonFlags;

//these are the possible events the logic can return, and what they mean.
//not currently used in MainMenu or LevelSelect, but could be usefull to someone else maybe.
typedef enum {
    MENU_EV_NONE = 0, //nothing happened this frame
    MENU_EV_MOVED, //the highlight moved, but no button fired
    MENU_EV_HANDLED, //a button fired and did it's work in the callback, nothing needs to happen here
    MENU_EV_STATE, //a button fired and returned something in MenuEvent.state
    MENU_EV_VALUE, //a button fired and returned something in MenuEvent.value
    MENU_EV_PTR, //a button fired and returned something in MenuEvent.ptr
    /*
    * the cursor stepped onto an exit sentinel and the handoff succeeded:
    * this menu has released context, and MenuEvent.menu is whoever holds it now.
    * MenuEvent.button is the index the cursor left FROM.
    * a REFUSED exit (no neighbour that way, or the neighbour would not take it)
    * produces no event at all and moves nothing.
    */
    MENU_EV_EXIT,
} MenuEventKind;


//the return type from logic, 
//kind determines what union member to look at, 
//button is the index of the button that did it in the table of buttons, or MENU_NONE when the kind is MENU_EV_NONE
typedef struct {
    MenuEventKind kind;
    int button;
    union { //TO ADD MORE RETURN TYPES, ADD THEM HERE
        Game_State state;
        int value;
        void* ptr;
        MenuNav* menu; //MENU_EV_EXIT: whoever holds context now
    };
} MenuEvent;


//a click handler
//a funciton pointer to a user generated callback,
//it does not need to set MoveEvent.button, as this file does that automatically
//use the return helpers to return rather than making the MenuEvent by hand
typedef MenuEvent (*MenuAction)(int index, void* user);


//fires when the highlight shifts to a button, 
//including once from menu_nav_create so the initial selection is not a special case for the caller
//if the button is tappable, then this function must be idempotant, 
//as it is called whenever the button is tapped, even if it is already highlighted
typedef void (*MenuHighlight)(int index, void* user);


typedef struct {
    //these 2 things in the union take up the same amount of space
    //and are effectively the same, with the only difference being that 
    //one is used for the initial write (the struct)
    //and the other is used for reading (the list)
    union { 
        struct {
            s8 left; 
            s8 right;
            s8 up;
            s8 down;
        };
        s8 link[MENU_DIR_COUNT];
    };
    MenuAction on_click; //NULL for a button that does nothing, otherwise it is the function that runs when clicked
    MenuHighlight on_highlight; //same as above, but for the highlight
    u8 flags; //the MenuButtonFlags
} MenuDef;


//the configurations for this instance of the menu
//ZERO INITIALIZE THIS STRUCT (MenuConfig cfg = {0};).
//every field left zero is a documented default, and this struct gains fields over time,
//so a bare "MenuConfig cfg;" reads whatever was on the stack for anything not assigned.
typedef struct {
    const MenuDef* buttons; // must outlive the menu, so it is static or malloced.
    Rect* rects; // 'count' many contiguous rects, owned by caller, used for touch detection and drawing the highlight
    void* user; //handed to every callback, it is intended to be used as the way that the callbacks reference and modify the current state's global struct, but could hold anything.
    u32 highlightColor; //the color of the highlight rect
    u32 disabledColor;
    u32 standard_color;
    u32 activeKey; //the key that is used to denote when a button is pressed.
    float highlightPad; //the amount of padding the highlight rect has
    int count; // no limit, but must be at least 1
    //the index of the initial highlighted button. out of range is clamped to 0.
    //MENU_NONE starts the menu with NO context: nothing highlighted, keys ignored, and
    //draw does nothing, until something hands it context.
    int selected;
    /*
    * the button to select when context arrives through each side of this menu.
    * enter[MENU_DIR_LEFT] is "context came in through my left side", meaning the menu
    * to my left handed off rightwards. MENU_NONE SEALS that side: entry through it is
    * refused and the menu that tried to leave keeps its cursor.
    * these are button indexes, the same ones the links and the rects and the callbacks
    * use, so a zeroed config enters on button 0 from every side.
    */
    int enter[MENU_DIR_COUNT];
    bool touch; //if false, the touch screen is ignored entirely. this is required if buttons are only on the top screen
    /*
    * touch follows the stylus while it is HELD, instead of only on the frame it lands.
    * the highlight moves every frame of the drag, so on_highlight is the per frame hook
    * for anything that wants to track a drag. on_click still fires ONLY on the frame the
    * stylus went down, so a held stylus cannot fire a button over and over.
    */
    bool draggable;
} MenuConfig;


//builds and inits the menu, returns NULL if bad config or out of memory, and the caller is expected to handle that
//call this in init
//this fires the initial selection's on_highlight before returning, so call this after the rects are made and layed out
MenuNav* menu_nav_init(const MenuConfig* cfg);

/*
* one frame of logic.
* moves the highlight, then activates a button if select_key was pressed or a tap landed on
* one. a tap moves the highlight and (unless the flag is set) activates the button, in that
* order.
*
* resolution order when things collide in one frame:
* - a tap OUTRANKS a pending keyboard exit, since the tap says exactly where to go.
* - a successful handoff returns immediately, so an activeKey press in the same frame is
*   swallowed rather than firing a button on the menu being left.
* - with no context (selected == MENU_NONE) the keys do nothing at all and only touch is read.
*
* Null safe, returning MENU_EV_NONE
*/
MenuEvent menu_nav_logic(MenuNav* menu, const FrameInput* in);


//draws the highlight behind the selected button, call before drawing buttons, or at a lower layer
//ASSUMES THE GPU IS SET UP FOR 2D.
//NULL safe
void menu_nav_draw(const MenuNav* menu);


//getters and setters, used internally for convienience, but mainly made for the caller's logic to affect the menu

//gets the currently sected index
//MENU_NONE if m is NULL
int menu_nav_selected(const MenuNav* menu); 

//sets the currently selected index, moves the higlight there, and fires on_highlight
void menu_nav_select(MenuNav* menu, int index); 

//get the flags, so that they can be edited and then set back.
u8 menu_nav_get_flags(MenuNav* menu, int index);

/*
* allows disabling buttons at runtime.
* this writes only the RUNTIME overlay, which is OR'd on top of the static table's flags,
* so runtime flags can only ever ADD restrictions: this cannot clear a flag the MenuDef
* table set, and a get_flags -> modify -> set_flags round trip copies the table's bits
* into the overlay permanently.
*/
void menu_nav_set_flags(MenuNav* menu, int index, u8 flags); 

//runtime disable, both input paths, used for greying out a button and disabling it
//the color changing can be disabled to not mess with a state having different settings.
void menu_nav_set_enabled(MenuNav* menu, int index, bool on, bool changeColor); 


//multi menu support
//a menu never NEEDS any of this: with no neighbours linked it behaves exactly as it always
//has. this is what lets several menus share a screen, with one holding context at a time.

/*
* names the menu that sits on one side of this one, so an exit sentinel in that direction
* has somewhere to go. call this after every menu exists, since linking needs both handles.
*
* links are ONE DIRECTIONAL on purpose: layouts are legitimately asymmetric (A exits right
* to B, but B's left exits to C), so the reverse edge is a second call.
* a NULL neighbour unlinks that side. a menu may not be linked to itself.
*/
void menu_nav_link(MenuNav* menu, MenuDir dir, MenuNav* neighbour);

/*
* hands context to this menu, arriving through the given side, which selects cfg.enter[side].
* if that button has been disabled at runtime, the cursor walks ONE step onwards in the
* direction of travel; if that fails too, entry is refused.
*
* returns whether the menu actually took context, so a caller can leave its own cursor
* where it was when the handoff is refused. NULL safe, returning false.
*/
bool menu_nav_enter_from_side(MenuNav* menu, MenuDir side);

//gives up context: nothing highlighted, keys ignored, draw does nothing.
//no callback fires, since nothing is being highlighted. NULL safe
void menu_nav_release(MenuNav* menu);

//does this menu currently hold context
bool menu_nav_has_context(const MenuNav* menu);

/*
* which of this menu's buttons the stylus is on this frame, or MENU_NONE for none.
* applies cfg.touch, each button's MENU_BTN_NO_TOUCH, and cfg.draggable's held vs landed rule.
*
* this is the ONE hit test: menu_nav_logic uses it, and so must anything routing touch
* between several menus, so the two can never disagree about which menu owns a tap.
* NULL safe, returning MENU_NONE
*/
int menu_nav_hit_test(const MenuNav* menu, const FrameInput* in);

//runtime toggle for cfg.draggable
void menu_nav_set_draggable(MenuNav* menu, bool on);

//the inverse of menu_nav_init. NULL safe.
//do NOT call this on a menu owned by a MenuGroup, that group frees its own menus.
void menu_nav_free(MenuNav* menu);


//The return value constructors that the click handler uses, one per union member
//these are made for convience, but should save a lot of time, and prevent mismatches
//for my sake, ONLY RETURN FROM PRESSED CALLBACKS WITH THESE CONSTRUCTORS.
//also, if you add a new datatype to the menuEvent union, add a new constructor here

static inline MenuEvent menu_handled(void){
    return (MenuEvent){.kind = MENU_EV_HANDLED};
}

static inline MenuEvent menu_state(Game_State s){
    return (MenuEvent){.kind = MENU_EV_STATE, .state = s};
}

static inline MenuEvent menu_value(int v){
    return (MenuEvent){.kind = MENU_EV_VALUE, .value = v};
}

static inline MenuEvent menu_ptr(void* p){
    return (MenuEvent){.kind = MENU_EV_PTR, .ptr = p};
}




