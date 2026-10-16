#include "menu_nav.h"
#include "datatypes/graphics_types.h"
#include "datatypes/game_state.h"
#include "utils/debug.h"
#include "utils/graphics.h"
#include <stdlib.h>
#include <3ds.h>
#include <assert.h>

//defines 

//highlight dimensions
#define HIGHLIGHT_PAD 5


//define the struct for the state of menu navigation
typedef struct {
    Rect highlightBox; //the rectangle that moves to the selected button
    MenuButton* buttons; //the list of all of the buttons that are active in the menu, this is where they stay referenced.
    MenuButton* highlightedButton; //the pointer to the currently selected button
    int buttons_length; //the amount of buttons that exist in the menu
    bool touch;

} MenuNavState;

static MenuNavState* mnstate;


//helper static functions

static void set_selected(MenuButton* button){
    //set the highlighted button
    mnstate->highlightedButton = button;
    //move the rect to the correct space
    mnstate->highlightBox.x = button->curRect->x - HIGHLIGHT_PAD;
    mnstate->highlightBox.y = button->curRect->y - HIGHLIGHT_PAD;
    mnstate->highlightBox.height = button->curRect->height + (2 * HIGHLIGHT_PAD);
    mnstate->highlightBox.width = button->curRect->width + (2 * HIGHLIGHT_PAD);
    mnstate->highlightedButton->cursor_highlighted(mnstate->highlightedButton);
}





bool menu_nav_init(MenuButton* init_list, int list_len, MenuButton* init_selected, bool touch){
    
    //check that the init selected exists within the init list
    assert((u32)init_selected <= ((u32)init_list + list_len));

    //init state struct
    mnstate = malloc(sizeof(MenuNavState));
    if (mnstate == NULL){
        return false;
    }

    //init the list of buttons
    mnstate->buttons = malloc(sizeof(MenuButton) * list_len);
    if (mnstate == NULL){
        free(mnstate);
        return false;
    }
    memcpy(mnstate->buttons, init_list, list_len);
    mnstate->buttons_length = list_len;

    mnstate->highlightBox.Color = Colors[CLR_YELLOW];
    mnstate->highlightBox.z = 0;
    set_selected(init_selected);

    mnstate->touch = touch;

    return true;
}


MenuButton* menu_nav_get_highlighted(){
    return mnstate->highlightedButton;
}







bool menu_nav_logic(FrameInput* in){
    MenuButton* nextButton = NULL;
    bool immedieteClick = false;

    //d and c pad movement 
    //left and right movement
    if (in->kDown & KEY_LEFT && mnstate->highlightedButton->left != NULL && mnstate->highlightedButton->left->menuable){
        nextButton = mnstate->highlightedButton->left;
    }
    if (in->kDown & KEY_RIGHT && mnstate->highlightedButton->right != NULL && mnstate->highlightedButton->right->menuable){
        nextButton = mnstate->highlightedButton->right;
    }

    //up and down movement
    if (in->kDown & KEY_UP && mnstate->highlightedButton->up != NULL && mnstate->highlightedButton->up->menuable){
        nextButton = mnstate->highlightedButton->up;
    }
    if (in->kDown & KEY_DOWN && mnstate->highlightedButton->down != NULL && mnstate->highlightedButton->down->menuable){
        nextButton = mnstate->highlightedButton->down;
    }

    //touchscreen logic
    if (mnstate->touch && in->kDown & KEY_TOUCH){
        for (int i = 0; i < mnstate->buttons_length; i++){
            if (Rect_Tapped(mnstate->buttons[i].curRect, in)){
                immedieteClick = true;
                nextButton = &mnstate->buttons[i];
                break;
            }
        }
    }

    if (nextButton != NULL){
        set_selected(nextButton);
    }

    if (in->kDown & KEY_A || immedieteClick){
        mnstate->highlightedButton->clicked_button(mnstate->highlightedButton);
    }
    
    return nextButton != NULL;
}


void menu_nav_draw(){
    DrawRect(&mnstate->highlightBox);
}


//can be called at any point, as the rects are owned by the caller
void menu_nav_free(){
    //free the buttons 
    free(mnstate->buttons);
    //free the struct
    free(mnstate);
}