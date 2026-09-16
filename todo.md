### Todo list

## setup for both screens
* create 2 main files in different folders, one with only the top screen, with:
    * a top display and bottom terminal
    * a bottom screen and top terminal
    * both screens and no terminals (later down the line)

## Top screen todos
* Port over the basic movement for the player - make them controlled by the circle pad and d pad
* make player colision with "colide obj" struct, which should hold all of the information needed for each of the rectangles.
* make a version of the draw rectangle funciton that takes a "collide obj" as a perameter and draws the collide obj
* make the keys and doors.
* import the graphics and music, - make sure these load when the game starts.
* make hard mode
* make the win and hard win screens.
* make the out of bounds screen.
* add toggle for speedrun timer on the bottom screen
* import "You broke my dance" to be the background ost when playing normal mode, and have it dynamically switch to "breakdance breakdance" when either hard mode or speedrun mode is on

LATER
* make there a 1 - 200 chance that when you go through a screen transition, you are sent to the egg room. (before this save the location the player would be sent to)
* import the tree and music
* make it so that there is an invisible block that disapears like a key beind the tree, so that when you walk behind the tree, the bottom screen says, "you recieved given an egg" on the bottom screen, and the corresponding sound plays.
and when you walk out of the room, you are transported to the saved location you would be sent to.



## Bottom screen todos
* make a struct called "button_rect", that stores a rectangle, and is added to a hash set, and every frame this set is looped through, and each one is checked to see if it is being touched.
* make an enum called "Screen State" which stores the current state of the bottom screen, either "MAIN_MENU", "MAKER", "SELECT_MAP", and "PLAYING"
* make a map select screen, which only allows you to choose beteween 4 slots, which are stored in your save data
* in playing mode, there should be the timer display, the keys display, the map display, and the controls display.
* make the grid of tiles you can click.
* make a pallet with 6 buttons with text along the bottom of the maker screen, with 2 arrow buttons and a page number in between. for ease of use, the button_rect objects should be stored in a list of arrays, with each array being 6 long and holding the buttons. this should be hardcoded, and easily be able to be added to. 

Backend
* make it so that when you compile a level, it coaleses blocks in it's row, or makes the actual count of wall tiles as small as possible. 


Notes -
Planned NEW features
* corner tiles - this tile is a thin wall across 2 adjacent sides of the wall. 
* hallway tiles - same as above but with the walls across from each other