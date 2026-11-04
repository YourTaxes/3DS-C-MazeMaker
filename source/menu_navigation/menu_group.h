#pragma once

#include "menu_navigation/menu_nav.h"

/*
* several MenuNavs sharing one screen, with exactly ONE of them holding keyboard context
* at a time, and touch routed to whichever one the stylus landed in.
*
* the group OWNS its menus: it builds them from the configs handed to it, wires the links
* between them, and frees them. a state keeps the group handle and reaches a menu through
* menu_group_menu when it needs one of the menu_nav_* setters.
* DO NOT free a grouped menu yourself, menu_group_free does that.
*
* owning the menus is the whole reason the group takes configs rather than handles: linking
* needs every menu to exist first, and if the group builds them that ordering cannot be
* gotten wrong.
*
* only ONE group may exist at a time. a second menu_group_init fails and logs rather than
* replacing the first, because the caller is still holding a handle to it.
*
* a menu never NEEDS a group: menu_nav_link and the MENU_EXIT_* sentinels work standalone,
* and a menu with no neighbours behaves exactly as it always has. the group only adds the
* touch routing and the one-context bookkeeping that cannot be done from inside one menu.
*/
typedef struct MenuGroup MenuGroup;


//which menu sits on each side of one menu, as indexes into the group's own menu array.
//MENU_NONE for an edge with nothing beyond it.
//same union trick as MenuDef, for the same reason: the table is written with the names
//and read as an array indexed by MenuDir.
#define MENU_NEIGHBOURS(l, r, u, d) {.left = (l), .right = (r), .up = (u), .down = (d)}

typedef struct {
    union {
        struct {
            s8 left;
            s8 right;
            s8 up;
            s8 down;
        };
        s8 side[MENU_DIR_COUNT];
    };
} MenuLink;


//ZERO INITIALIZE THIS STRUCT (MenuGroupConfig cfg = {0};)
typedef struct {
    const MenuConfig* menus; //count many, read during init only and never kept
    const MenuLink* links; //count many, or NULL to call menu_nav_link by hand afterwards
    int count; //the number of menus, at least 1
    //the menu that holds context first.
    //it's MenuConfig.selected has to be a real button index, not MENU_NONE
    int start;
} MenuGroupConfig;


/*
* builds every menu, wires the links, and leaves cfg.start as the only one with context.
* returns NULL on a bad config, out of memory, or a group already existing,
* and the caller is expected to handle that.
*
* every menu's initial on_highlight fires during this call, in array order, and the start
* menu's fires LAST, so a callback that writes shared state settles on the focused menu.
*/
MenuGroup* menu_group_init(const MenuGroupConfig* cfg);


/*
* one frame of the whole group.
* a touch that BEGINS this frame moves context to the menu it landed in, and then the
* context menu runs it's own logic, which handles that same touch normally. so there is
* only ever one menu doing work, and only one place that reads the touchscreen.
*
* the returned event belongs to the context menu, which the state can identify with
* menu_group_context. the exception is MENU_EV_EXIT, which the group has already acted on,
* and whose MenuEvent.menu is the menu that holds context now.
*
* NULL safe, returning MENU_EV_NONE
*/
MenuEvent menu_group_logic(MenuGroup* group, const FrameInput* in);


//draws every menu's highlight, which is exactly one highlight: the rest have no context.
//call before drawing the buttons, or at a lower layer.
//ASSUMES THE GPU IS SET UP FOR 2D. NULL safe
void menu_group_draw(const MenuGroup* group);


//one of the group's menus, for passing to the menu_nav_* getters and setters.
//NULL if the group is NULL or the index is out of range
MenuNav* menu_group_menu(const MenuGroup* group, int index);


//the index of the menu that holds context. MENU_NONE only when the group is NULL
int menu_group_context(const MenuGroup* group);


/*
* forces context onto one menu, arriving through the given side, exactly as if a neighbour
* had handed off that way. the escape hatch for a state that needs to drive focus itself.
* returns false (changing nothing) if that menu refuses entry from that side.
*/
bool menu_group_focus(MenuGroup* group, int index, MenuDir side);


//the inverse of menu_group_init, which frees every menu it built. NULL safe.
//null your own handle afterwards, this cannot do it for you
void menu_group_free(MenuGroup* group);
