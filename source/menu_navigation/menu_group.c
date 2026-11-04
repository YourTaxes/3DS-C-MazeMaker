#include "menu_group.h"
#include "utils/debug.h"
#include <stdlib.h>
#include <stddef.h> //for offsetof, the union layout assert
#include <3ds.h>

//MenuLink's named sides and it's side[] array are read by MenuDir, the same deal as
//MenuDef's links, so the same assert applies: a table writes .up and the code reads
//side[MENU_DIR_UP], so they have to overlap exactly
_Static_assert(offsetof(MenuLink, left) == offsetof(MenuLink, side[MENU_DIR_LEFT])
            && offsetof(MenuLink, right) == offsetof(MenuLink, side[MENU_DIR_RIGHT])
            && offsetof(MenuLink, up) == offsetof(MenuLink, side[MENU_DIR_UP])
            && offsetof(MenuLink, down) == offsetof(MenuLink, side[MENU_DIR_DOWN]),
                "a link table writes .up and the linker reads side[MENU_DIR_UP], so the named sides and the array must overlap each other exactly");


//only one group at a time, so this is the one that exists.
//unlike popup_init, a second init does NOT replace this one, because the caller is still
//holding the handle to it and freeing it out from under them would dangle that pointer
static MenuGroup* g_active;


struct MenuGroup {
    int count; //the number of menus
    int context; //the index of the menu currently holding keyboard context
    //owned, and in the struct's single allocation, the same trick as MenuNav's live[]
    MenuNav* menus[];
};


//static helpers

//which index a menu sits at, or MENU_NONE if it is not one of ours.
//the counts here are tiny (a handful of menus on one screen), so a scan is the whole story
static int index_of(const MenuGroup* group, const MenuNav* menu){
    for (int i = 0; i < group->count; i++){
        if (group->menus[i] == menu){
            return i;
        }
    }
    return MENU_NONE;
}


//frees the first 'built' menus, for backing out of a half finished init
static void unwind(MenuGroup* group, int built){
    for (int i = 0; i < built; i++){
        menu_nav_free(group->menus[i]);
    }
    free(group);
}


//checks the link table names real menus and nothing links to itself.
//done as it's own pass so a bad table fails init instead of half wiring the group
static bool links_ok(const MenuGroupConfig* cfg){
    for (int i = 0; i < cfg->count; i++){
        for (int d = 0; d < MENU_DIR_COUNT; d++){
            const s8 to = cfg->links[i].side[d];
            if (to == MENU_NONE){
                continue;
            }
            if (to < 0 || to >= cfg->count){
                printConsole("menu_group_init: menu %d links to %d, which is not a menu", i, (int)to);
                return false;
            }
            if (to == i){
                printConsole("menu_group_init: menu %d links to itself", i);
                return false;
            }
        }
    }
    return true;
}


MenuGroup* menu_group_init(const MenuGroupConfig* cfg){
    if (cfg == NULL || cfg->menus == NULL || cfg->count < 1){
        return NULL;
    }
    if (cfg->start < 0 || cfg->start >= cfg->count){
        return NULL;
    }
    //the start menu is the one that has context, so it has to actually have a button.
    //this is a config mistake rather than a state to support, so it fails loudly here
    if (cfg->menus[cfg->start].selected == MENU_NONE){
        printConsole("menu_group_init: the start menu cannot be created without a selection");
        return NULL;
    }
    if (cfg->links != NULL && !links_ok(cfg)){
        return NULL;
    }
    if (g_active != NULL){
        printConsole("menu_group_init: a group already exists, free that one first");
        return NULL;
    }

    //the struct and it's menu pointers in one block, so there is one allocation to free
    MenuGroup* group = malloc(sizeof(*group) + (size_t)cfg->count * sizeof(MenuNav*));
    if (group == NULL){
        return NULL;
    }
    group->count = cfg->count;
    group->context = cfg->start;

    //build every menu first, since linking needs all of them to exist
    for (int i = 0; i < cfg->count; i++){
        group->menus[i] = menu_nav_init(&cfg->menus[i]);
        if (group->menus[i] == NULL){
            printConsole("menu_group_init: menu %d failed to build", i);
            unwind(group, i);
            return NULL;
        }
    }

    //now that they all exist, wire them to each other
    if (cfg->links != NULL){
        for (int i = 0; i < cfg->count; i++){
            for (int d = 0; d < MENU_DIR_COUNT; d++){
                const s8 to = cfg->links[i].side[d];
                menu_nav_link(group->menus[i], (MenuDir)d, to == MENU_NONE ? NULL : group->menus[to]);
            }
        }
    }

    //exactly one menu holds context, so everything else gives it up
    for (int i = 0; i < cfg->count; i++){
        if (i != cfg->start){
            menu_nav_release(group->menus[i]);
        }
    }

    //every menu fired it's initial on_highlight on the way up, in array order, so refire
    //the start menu's last. that way a callback writing shared state (a preview picture,
    //a label) settles on the menu that actually has the cursor
    menu_nav_select(group->menus[cfg->start], menu_nav_selected(group->menus[cfg->start]));

    g_active = group;
    printConsole("menu group built: %d menus, starting on %d", group->count, group->context);
    return group;
}


MenuEvent menu_group_logic(MenuGroup* group, const FrameInput* in){
    MenuEvent event;
    event.kind = MENU_EV_NONE;
    event.button = MENU_NONE;

    if (group == NULL || in == NULL){
        return event;
    }

    /*
    * a touch that BEGINS this frame can move context to the menu it landed in.
    *
    * gated on kDown and never kHeld, even for draggable menus, so a drag that slides off
    * a draggable menu's edge keeps the in progress gesture on the menu it started on.
    * that is safe because kHeld is a superset of kDown, so a draggable menu's held based
    * hit test still resolves correctly inside this gate.
    *
    * menus are scanned in array order and the first hit wins, so menus should not overlap,
    * and where they do, array order is the priority order.
    */
    if (in->kDown & KEY_TOUCH){
        for (int i = 0; i < group->count; i++){
            //the same hit test menu_nav_logic is about to run, so the two cannot disagree
            //about who owns this tap
            if (menu_nav_hit_test(group->menus[i], in) == MENU_NONE){
                continue;
            }
            if (i != group->context){
                menu_nav_release(group->menus[group->context]);
                group->context = i;
                //and nothing else. a tap has no direction, so there is no side to enter
                //from: the tapped menu has no selection, and it's own logic below will
                //land the cursor on the button the stylus actually hit, which beats any
                //guess enter[] could make
            }
            break;
        }
    }

    //only the context menu does any work, so there is one highlight and one set of callbacks
    event = menu_nav_logic(group->menus[group->context], in);

    //the menu handed context off by itself, this just keeps the bookkeeping in step
    if (event.kind == MENU_EV_EXIT){
        const int to = index_of(group, event.menu);
        if (to != MENU_NONE){
            group->context = to;
        } else {
            //cannot happen when the group wired the links, since every neighbour is one
            //of ours. menu_group_focus is the way back if it somehow does
            printConsole("menu_group_logic: context left to a menu outside the group");
        }
    }

    return event;
}


void menu_group_draw(const MenuGroup* group){
    if (group == NULL){
        return;
    }
    //every menu but one returns early inside here, so this draws a single highlight
    for (int i = 0; i < group->count; i++){
        menu_nav_draw(group->menus[i]);
    }
}


MenuNav* menu_group_menu(const MenuGroup* group, int index){
    if (group == NULL || index < 0 || index >= group->count){
        return NULL;
    }
    return group->menus[index];
}


int menu_group_context(const MenuGroup* group){
    return group != NULL ? group->context : MENU_NONE;
}


bool menu_group_focus(MenuGroup* group, int index, MenuDir side){
    if (group == NULL || index < 0 || index >= group->count){
        return false;
    }
    if (index == group->context){
        return true; //already there
    }
    //ask first, so a refusal leaves the current menu holding it's cursor
    if (!menu_nav_enter_from_side(group->menus[index], side)){
        return false;
    }
    menu_nav_release(group->menus[group->context]);
    group->context = index;
    return true;
}


void menu_group_free(MenuGroup* group){
    if (group == NULL){
        return;
    }
    for (int i = 0; i < group->count; i++){
        menu_nav_free(group->menus[i]);
    }
    if (g_active == group){
        g_active = NULL;
    }
    free(group); //the menu pointer array rides along inside it
}
