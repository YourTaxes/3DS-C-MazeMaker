#include "menu_nav.h"
#include "utils/graphics.h"
#include "utils/debug.h"
#include <stdlib.h>
#include <string.h> //for memset
#include <stddef.h> //for offsetof, the union layout assert
#include <3ds.h>

//defines 


//MenuDir is used as an index into 3 things that have to agree,
//DIR_KEYS below, MenuDef's link[], and MenuDef's named left/right/up/down.
//the first assert pins these values, the second pins the union layout they rely on
_Static_assert(MENU_DIR_LEFT == 0 && MENU_DIR_RIGHT == 1 && MENU_DIR_UP == 2 && MENU_DIR_DOWN == 3, "DIR_KEYS and MenuDef's link[] are indexed by MenuDir, in this order");
_Static_assert(offsetof(MenuDef, left) == offsetof(MenuDef, link[MENU_DIR_LEFT])
            && offsetof(MenuDef, right) == offsetof(MenuDef, link[MENU_DIR_RIGHT])
            && offsetof(MenuDef, up) == offsetof(MenuDef, link[MENU_DIR_UP])
            && offsetof(MenuDef, down) == offsetof(MenuDef, link[MENU_DIR_DOWN]),
                "a table writes .up and the movement loop reads link[MENU_DIR_UP], so the named links and the array must overlap each other exactly");


//the key bit that moves the cursor in each direction, put into a list
//if for some reason, I want to add somethning like the L button as the alternative up key, I make changes here
static const u32 DIR_KEYS[MENU_DIR_COUNT] = {
    [MENU_DIR_LEFT] = KEY_LEFT,
    [MENU_DIR_RIGHT] = KEY_RIGHT,
    [MENU_DIR_UP] = KEY_UP,
    [MENU_DIR_DOWN] = KEY_DOWN,
};

//the side a neighbour sees context arriving through is the opposite of the side it left by.
//dir ^ 1 would work given the enum order pinned above, but the table says what it means.
static const MenuDir DIR_OPPOSITE[MENU_DIR_COUNT] = {
    [MENU_DIR_LEFT]  = MENU_DIR_RIGHT,
    [MENU_DIR_RIGHT] = MENU_DIR_LEFT,
    [MENU_DIR_UP]    = MENU_DIR_DOWN,
    [MENU_DIR_DOWN]  = MENU_DIR_UP,
};

//the exit sentinels are derived from MenuDir, so they inherit its order.
//if MenuDir is ever reordered these change with it, which is the point, but the numbers
//are written down here so a renumber is a visible failure instead of a silent one.
_Static_assert(MENU_EXIT_LEFT == -2 && MENU_EXIT_RIGHT == -3
            && MENU_EXIT_UP == -4 && MENU_EXIT_DOWN == -5,
    "the exit sentinels follow MenuDir's order");
_Static_assert(MENU_EXIT(MENU_DIR_COUNT - 1) > -128,
    "every exit sentinel has to fit in MenuDef's s8 link field");

//the struct used to store all the data about the current menu
struct MenuNav {
    Rect highlightRect; //the rect which represents the current highlighted rectangle
    const MenuDef* buttons; //borrowed, static lifetime
    Rect* rects; //borrowed, caller owned
    void* user; //the pointer to the state struct or anything else the callbacks need
    u32 highlightColor;
    float highlightPad;
    u32 disabledColor;
    u32 standardColor;
    u32 activeKey; //the key mask that will register a click
    int count; //the amount of buttons
    int selected; //the index of the currently selected button
    bool touch; //true if this menu will register touchscreen input
    bool draggable; //true if touch follows a held stylus instead of only the frame it lands
    MenuNav* nav[MENU_DIR_COUNT]; //the menus on each side, NULL for none. set by menu_nav_link
    int enter[MENU_DIR_COUNT]; //the button to land on when context arrives through each side
    //one menubuttonflags per button, or'd on top of the table's static flags
    //which is what menu_nav_set_enabled writes.
    //flexable array number, so it shares the struct's single allocation and the button count needs no cap
    u8 live[];
};

//static helpers

//a button's effective flags: what the table says, plus what has been set at runtime
static u8 button_flags(const MenuNav* menu, int i){
    return (u8)(menu->buttons[i].flags | menu->live[i]);
}

//returns if the button can be selected by the cursor
static bool cursor_ok(const MenuNav* menu, int i){
    return !(button_flags(menu, i) & MENU_BTN_NO_CURSOR);
}

//returns if the button can be selected by touching
static bool touch_ok(const MenuNav* menu, int i){
    return !(button_flags(menu, i) & MENU_BTN_NO_TOUCH);
}

//one move in one direction
//walks past buttons the cursor is not allowed to land on, 
//and gives up if it runs off a dead end or all the way around a ring
//in which case the cursor does not move. 
static int step(const MenuNav* menu, int from, MenuDir dir){
    int cur = from;
    for (int guard = 0; guard < menu->count; guard++){
        //the next is the link in the current direction from the current button
        const s8 next = menu->buttons[cur].link[dir]; 
        
        if (next == MENU_NONE){
            return from; //nothing that way
        }
        if (MENU_LINK_IS_EXIT(next)){
            //the caller deals with leaving the menu. it cannot be followed here, because
            //cursor_ok below would index buttons[-4]
            return next;
        }
        cur = next;
        if (cur == from){
            return from; //walked a whole ring, everything skipped
        }
        if (cursor_ok(menu, cur)){
            return cur;
        }
    }
    //shouldn't really get here, but just in case
    return from;
}



static void build_highlight_rect(MenuNav* menu){
    if (menu->selected == MENU_NONE){
        return; //no context, so there is no button to frame. the stale rect is never drawn
    }
    menu->highlightRect.Color = menu->highlightColor;
    menu->highlightRect.x = menu->rects[menu->selected].x - menu->highlightPad;
    menu->highlightRect.y = menu->rects[menu->selected].y - menu->highlightPad;
    menu->highlightRect.z = menu->rects[menu->selected].z;
    menu->highlightRect.width = menu->rects[menu->selected].width + 2.0f * menu->highlightPad;
    menu->highlightRect.height = menu->rects[menu->selected].height + 2.0f * menu->highlightPad;
}

//moves the highlight to the given button,
//or gives up context entirely when index is MENU_NONE.
//this is the only place selected is written, so the no context case is handled once
static void select_index(MenuNav* menu, int index){
    menu->selected = index;
    if (index == MENU_NONE){
        return; //released: no button to highlight, and nobody to notify about it
    }
    const MenuHighlight hover = menu->buttons[index].on_highlight;
    //check if the on highlight exists before calling it
    if (hover != NULL){
        hover(index, menu->user);
    }
    build_highlight_rect(menu);
}


//the main init function that creates the MenuNav struct
MenuNav* menu_nav_init(const MenuConfig* cfg){
    if (cfg == NULL || cfg->buttons == NULL || cfg->rects == NULL || cfg->count < 1){
        return NULL;
    }

    //every link has to name a real button, or be MENU_NONE, or be an exit sentinel.
    //this cannot catch a link that was never written, since that reads as 0
    for (int i = 0; i < cfg->count; i++){
        for (int d = 0; d < MENU_DIR_COUNT; d++){
            const s8 link = cfg->buttons[i].link[d];
            if (link == MENU_NONE || MENU_LINK_IS_EXIT(link)){
                continue;
            }
            if (link < 0 || link >= cfg->count){
                return NULL;
            }
        }
    }

    //same for the entry buttons, where MENU_NONE means that side is sealed
    for (int d = 0; d < MENU_DIR_COUNT; d++){
        const int e = cfg->enter[d];
        if (e != MENU_NONE && (e < 0 || e >= cfg->count)){
            return NULL;
        }
    }

    //the struct and it's per button live[] flags in one block, so there is exactly one allocation and one failure path.
    MenuNav* menu = malloc(sizeof(*menu) + (size_t)cfg->count);
    if (menu == NULL){
        return NULL;
    }

    menu->buttons = cfg->buttons;
    menu->rects = cfg->rects; 
    menu->user = cfg->user;
    menu->highlightColor = cfg->highlightColor; 
    menu->highlightPad = cfg->highlightPad;
    menu->activeKey = cfg->activeKey; 
    menu->count = cfg->count; 
    menu->touch = cfg->touch;
    menu->disabledColor = cfg->disabledColor;
    menu->standardColor = cfg->standard_color;
    menu->draggable = cfg->draggable;
    menu->selected = 0;
    memset(menu->live, MENU_BTN_NORMAL, (size_t)cfg->count); //nothing disabled to start

    for (int d = 0; d < MENU_DIR_COUNT; d++){
        menu->nav[d] = NULL; //menu_nav_link fills these in once every menu exists
        menu->enter[d] = cfg->enter[d];
    }

    //select the initial button.
    //MENU_NONE is honoured rather than clamped, so a menu can be created without context
    int start = cfg->selected;
    if (start != MENU_NONE && (start < 0 || start >= cfg->count)){
        start = 0;
    }
    select_index(menu, start); //fires on_highlight, or does nothing at all for MENU_NONE

    return menu;
}

MenuEvent menu_nav_logic(MenuNav* menu, const FrameInput* in){
    MenuEvent event;
    event.kind = MENU_EV_NONE;
    event.button = MENU_NONE;

    //null check, just in case
    if (menu == NULL || in == NULL){
        return event;
    }

    event.button = menu->selected;

    int curBtn = menu->selected;
    //MENU_DIR_COUNT means "no exit pending this frame", so this needs no second flag
    MenuDir exitDir = MENU_DIR_COUNT;

    //the keys only mean anything while this menu holds context
    if (menu->selected != MENU_NONE){
        u32 keys = in->kDown;
        //check if user is pressing both left and right or up and down at the same time, in which case, cancel them out.
        if ((keys & KEY_LEFT) && (keys & KEY_RIGHT)){
            keys = keys & ~(u32)(KEY_LEFT | KEY_RIGHT);
        }
        if ((keys & KEY_UP) && (keys & KEY_DOWN)){
            keys = keys & ~(u32)(KEY_UP | KEY_DOWN);
        }

        //step the cursor in each direction if it is pressed
        for (int dir = 0; dir < MENU_DIR_COUNT; dir++){
            if (!(keys & DIR_KEYS[dir])){
                continue;
            }
            const int next = step(menu, curBtn, (MenuDir)dir);
            if (MENU_LINK_IS_EXIT(next)){
                //stop the loop here. curBtn has to stay a real index, because a second
                //pass would feed a sentinel back into step() as 'from' and read buttons[-4]
                exitDir = MENU_LINK_EXIT_DIR(next);
                break;
            }
            curBtn = next;
        }
    }

    //handle the touchscreen, through the shared hit test so that nothing can disagree
    //with this file about which button (or which menu) the stylus is on
    bool tapped = false; //a tap landed on a button, so the highlight moves to it.
    bool tapClicks = false; //that button lets a tap activate it as well
    const int hit = menu_nav_hit_test(menu, in);
    if (hit != MENU_NONE){
        curBtn = hit;
        tapped = true;
        //a drag moves the highlight every frame, but only fires on the frame it landed,
        //otherwise a held stylus would fire the button once per frame
        const bool fireFrame = !menu->draggable || (in->kDown & KEY_TOUCH);
        tapClicks = fireFrame && !(button_flags(menu, hit) & MENU_BTN_NO_TAP_CLICK);
        //an explicit tap says exactly where to go, so it outranks a keyboard exit
        exitDir = MENU_DIR_COUNT;
    }

    //the handoff to a neighbouring menu.
    //context is released BEFORE the neighbour is asked, so a degenerate self link cannot
    //blank this menu, and restored if the neighbour refuses, so a refusal moves nothing
    if (exitDir != MENU_DIR_COUNT && menu->nav[exitDir] != NULL){
        MenuNav* const to = menu->nav[exitDir];
        const int keep = menu->selected;
        menu->selected = MENU_NONE;
        if (menu_nav_enter_from_side(to, DIR_OPPOSITE[exitDir])){
            event.kind = MENU_EV_EXIT;
            event.button = keep; //the button the cursor left from
            event.menu = to;
            return event; //an activeKey press in this same frame is swallowed, by design
        }
        menu->selected = keep;
        //and fall through, so a move from another direction this frame is not thrown away
    }

    //a tap can re select it's own button, which is why on_highlight has to be idempotant
    //check here if the highlight moved
    if (curBtn != menu->selected || tapped){
        select_index(menu, curBtn);
        event.kind = MENU_EV_MOVED;
        event.button = menu->selected;
    }


    //check if the button that is currently selected
    //should be activated this frame
    const bool fire = tapClicks || ((in->kDown & menu->activeKey)
                                    && menu->selected != MENU_NONE
                                    && cursor_ok(menu, menu->selected));
    if (fire){
        const MenuAction click = menu->buttons[menu->selected].on_click;
        event = click != NULL ? click(menu->selected, menu->user) : menu_handled();
        event.button = menu->selected; //handlers do not change this field, needs to be set here
    }

    return event;
}




void menu_nav_draw(const MenuNav* menu){
    if (menu == NULL || menu->selected == MENU_NONE){
        return; //no context, so no highlight. this is what makes a group draw one highlight
    }


    DrawRect(&menu->highlightRect);
}




int menu_nav_selected(const MenuNav* menu){
    return menu != NULL ? menu->selected : MENU_NONE;
}


void menu_nav_select(MenuNav* menu, int index){
    if (menu == NULL || index < 0 || index >= menu->count){
        return;
    }
    select_index(menu, index);
}

u8 menu_nav_get_flags(MenuNav* menu, int index){
    if (menu == NULL || index < 0 || index >= menu->count){
        return 0;
    }
    return button_flags(menu, index);
}

void menu_nav_set_flags(MenuNav* menu, int index, u8 flags){
    if (menu == NULL || index < 0 || index >= menu->count){
        return;
    }
    menu->live[index] = flags; //the runtime overlay only, the table's flags are permanent
}
    
    
void menu_nav_set_enabled(MenuNav* menu, int index, bool on, bool changeColor){
    if (menu == NULL || index < 0 || index >= menu->count){
        return;
    }
    if (on){
        menu->live[index] &= (u8)~MENU_BTN_DISABLED;
    } else {
        menu->live[index] |= (u8)MENU_BTN_DISABLED;
    }
    if (changeColor){
        menu->rects[index].Color = on ? menu->standardColor : menu->disabledColor;
    }
}


//multi menu support

int menu_nav_hit_test(const MenuNav* menu, const FrameInput* in){
    if (menu == NULL || in == NULL || !menu->touch){
        return MENU_NONE;
    }

    //Rect_Tapped hardcodes kDown, so the key test is hoisted out here and the geometry
    //is checked with Rect_Contains, which is what Rect_Tapped does underneath anyway
    const u32 touchKeys = menu->draggable ? in->kHeld : in->kDown;
    if (!(touchKeys & KEY_TOUCH)){
        return MENU_NONE;
    }

    for (int i = 0; i < menu->count; i++){
        if (!touch_ok(menu, i)){
            //check if the button is able to be touched.
            continue;
        }
        if (Rect_Contains(&menu->rects[i], in->touch.px, in->touch.py)){
            return i;
        }
    }
    return MENU_NONE;
}


void menu_nav_link(MenuNav* menu, MenuDir dir, MenuNav* neighbour){
    if (menu == NULL || dir < 0 || dir >= MENU_DIR_COUNT){
        return;
    }
    if (neighbour == menu){
        printConsole("menu_nav_link: a menu cannot be it's own neighbour");
        return;
    }
    menu->nav[dir] = neighbour; //NULL unlinks, and links are one directional on purpose
}


bool menu_nav_enter_from_side(MenuNav* menu, MenuDir side){
    if (menu == NULL || side < 0 || side >= MENU_DIR_COUNT){
        return false;
    }

    int target = menu->enter[side];
    if (target == MENU_NONE || target < 0 || target >= menu->count){
        return false; //this side is sealed
    }

    //the entry button may have been disabled at runtime, so walk on in the direction of
    //travel, which is away from the side the context arrived through
    if (!cursor_ok(menu, target)){
        const int walked = step(menu, target, DIR_OPPOSITE[side]);
        //exactly ONE hop. a sentinel here would bounce context straight back out, and a
        //pair of menus pointing at each other that way would never settle on a button
        if (walked == target || MENU_LINK_IS_EXIT(walked)){
            return false;
        }
        target = walked;
    }

    select_index(menu, target);
    return true;
}


void menu_nav_release(MenuNav* menu){
    if (menu == NULL){
        return;
    }
    select_index(menu, MENU_NONE);
}


bool menu_nav_has_context(const MenuNav* menu){
    return menu != NULL && menu->selected != MENU_NONE;
}


void menu_nav_set_draggable(MenuNav* menu, bool on){
    if (menu == NULL){
        return;
    }
    menu->draggable = on;
}


void menu_nav_free(MenuNav* menu){
    free(menu); //one allocation, live[] rides along inside it
}


// //define the struct for the state of menu navigation
// typedef struct {
//     Rect highlightBox; //the rectangle that moves to the selected button
//     MenuButton* buttons; //the list of all of the buttons that are active in the menu, this is where they stay referenced.
//     MenuButton* highlightedButton; //the pointer to the currently selected button
//     int buttons_length; //the amount of buttons that exist in the menu
//     bool touch;

// } MenuNavState;

// static MenuNavState* mnstate;


// //helper static functions

// static void set_selected(MenuButton* button){
//     //set the highlighted button
//     mnstate->highlightedButton = button;
//     //move the rect to the correct space
//     mnstate->highlightBox.x = button->curRect->x - HIGHLIGHT_PAD;
//     mnstate->highlightBox.y = button->curRect->y - HIGHLIGHT_PAD;
//     mnstate->highlightBox.height = button->curRect->height + (2 * HIGHLIGHT_PAD);
//     mnstate->highlightBox.width = button->curRect->width + (2 * HIGHLIGHT_PAD);
//     if (mnstate->highlightedButton->cursor_highlighted != NULL){
//         mnstate->highlightedButton->cursor_highlighted(mnstate->highlightedButton);
//     }
// }





// bool menu_nav_init(MenuButton* init_list, int list_len, MenuButton* init_selected, bool touch){
    
//     //check that the init selected exists within the init list
//     assert((u32)init_selected <= ((u32)init_list + list_len));

//     //init state struct
//     mnstate = malloc(sizeof(MenuNavState));
//     if (mnstate == NULL){
//         return false;
//     }

//     //init the list of buttons
//     mnstate->buttons = malloc(sizeof(MenuButton) * list_len);
//     if (mnstate->buttons == NULL){
//         free(mnstate);
//         return false;
//     }
//     memcpy(mnstate->buttons, init_list, list_len * sizeof(MenuButton));
//     mnstate->buttons_length = list_len;

//     mnstate->highlightBox.Color = Colors[CLR_YELLOW];
//     mnstate->highlightBox.z = 0;
//     set_selected(init_selected);

//     mnstate->touch = touch;

//     return true;
// }


// MenuButton* menu_nav_get_highlighted(){
//     return mnstate->highlightedButton;
// }







// void* menu_nav_logic(const FrameInput* in){
//     MenuButton* nextButton = NULL;
//     bool immedieteClick = false;
//     void* returnVal = NULL;

//     //d and c pad movement 
//     //left and right movement
//     if (in->kDown & KEY_LEFT && mnstate->highlightedButton->left != NULL && mnstate->highlightedButton->left->menuable){
//         nextButton = mnstate->highlightedButton->left;
//     }
//     if (in->kDown & KEY_RIGHT && mnstate->highlightedButton->right != NULL && mnstate->highlightedButton->right->menuable){
//         nextButton = mnstate->highlightedButton->right;
//     }

//     //up and down movement
//     if (in->kDown & KEY_UP && mnstate->highlightedButton->up != NULL && mnstate->highlightedButton->up->menuable){
//         nextButton = mnstate->highlightedButton->up;
//     }
//     if (in->kDown & KEY_DOWN && mnstate->highlightedButton->down != NULL && mnstate->highlightedButton->down->menuable){
//         nextButton = mnstate->highlightedButton->down;
//     }

//     //touchscreen logic
//     if (mnstate->touch && in->kDown & KEY_TOUCH){
//         for (int i = 0; i < mnstate->buttons_length; i++){
//             if (Rect_Tapped(mnstate->buttons[i].curRect, in)){
//                 immedieteClick = true;
//                 nextButton = &mnstate->buttons[i];
//                 break;
//             }
//         }
//     }

//     if (nextButton != NULL){
//         set_selected(nextButton);
//     }

//     if (mnstate->highlightedButton->menuable && (in->kDown & KEY_A || immedieteClick)){
//         if (mnstate->highlightedButton->clicked_button != NULL){
//             returnVal = mnstate->highlightedButton->clicked_button(mnstate->highlightedButton);
//         }
//     }
//     return returnVal;
// }


// void menu_nav_draw(){
//     DrawRect(&mnstate->highlightBox);
    // }