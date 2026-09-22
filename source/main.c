#include <malloc.h> // mallinfo, for the heap stats on KEY_Y
#include <3ds.h>
#include <citro2d.h>
#include "debug.h"
#include "input.h"
#include "graphics.h"
#include "game_state.h"
#include "level_file.h"
#include "MainMenu.h"
#include "Debug_state.h"


//global variables
Game_State State = STATE_MAIN_MENU;
Raw_Level rawLvl;
Built_Level builtLvl;
//the level starts out needing a build, since nothing has been compiled yet
GameContext ctx = { .rawLvl = &rawLvl, .builtLvl = &builtLvl, .rebuildLevel = true };

C3D_RenderTarget* top;
C3D_RenderTarget* bottom;

/*
* one entry per Game_State. to add a state, write its four functions and add a line here.
* states not listed yet are all-NULL, and the main loop refuses to switch to them.
*
* STATE_MAZE_GAME   - play the level. includes the win and lose screens as substates.
* STATE_MAZE_MAKER  - edit the raw level on the bottom screen.
* STATE_SAVE_SELECT - pick which of the save slots is the current level.
* STATE_OOB         - out of bounds screen, A returns to the main menu.
* STATE_you_recieved_the_egg - him.
*/
static const StateFns STATES[STATE_COUNT] = {
	[STATE_MAIN_MENU] = { MainMenu_Init, MainMenu_Logic, MainMenu_Draw, MainMenu_End },
	[STATE_DEBUG]     = { Debug_Init,    Debug_logic,    Debug_Draw,    Debug_end    },
};


int main(int argc, char **argv)
{
	// Initialize services
	gfxInitDefault();

	// no on-screen console (both screens are used for rendering), so route
	// stderr to the attached debugger (GDB / Azahar log) via svcOutputDebugString
	consoleDebugInit(debugDevice_SVC);

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	romfsInit();

	top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	MakeColors();

	

	printConsole("By Finnegan McDevitt");

	hidScanInput();

	//from here until the loop exits there is always exactly one live state
	STATES[State].init(&ctx);

	// Main loop
	FrameInput in;
	bool running = true;
	while (running && aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		Input_Read(&in);

		if (in.kDown & KEY_X){
			printInputs(&in);
		}

		//keyboard test demo
		if(in.kDown & KEY_B)
		{
			char buff[20];

			if (GetKeyboard(buff, 20, "Testing Keyboard", SWKBD_TYPE_NORMAL)){
				printConsole("%s", buff);
			}
		}

		//do frame logic
		Game_State next = STATES[State].logic(&in, &ctx);
		if (in.kDown & KEY_START) next = STATE_QUIT; // START always quits, from any state

		//Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		STATES[State].draw(top, bottom);
		//End the frame after every screen has been drawn
		C3D_FrameEnd(0);

		//handle state switching. the old state is drawn one last time above,
		//then torn down, and the new one is set up before the next frame.
		if (next == STATE_QUIT){
			running = false; //the live state is torn down after the loop
		} else if (next != STATE_NONE){
			if (STATES[next].logic == NULL){
				printConsole("state %d is not implemented yet, staying in state %d", next, State);
			} else {
				STATES[State].end();
				State = next;
				STATES[State].init(&ctx);
			}
		}


		if (in.kDown & KEY_Y){
			//percentages are printed as integers on purpose: newlib's printf mallocs
			//scratch buffers the first time it formats a float and never frees them,
			//which would shift the very heap number we're trying to read.
			int cpu = (int)(C3D_GetProcessingTime()*600.0f); //hundredths of a percent
			int gpu = (int)(C3D_GetDrawingTime()*600.0f);
			int cmd = (int)(C3D_GetCmdBufUsage()*10000.0f);
			printConsole("CPU:     %3d.%02d%%", cpu/100, cpu%100);
			printConsole("GPU:     %3d.%02d%%", gpu/100, gpu%100);
			printConsole("CmdBuf:  %3d.%02d%%", cmd/100, cmd%100);
			//guest-side memory: bytes currently malloc'd, and free linear (GPU) memory.
			//if these stay flat across state switches, the game itself is not leaking.
			printConsole("heap used:   %d", mallinfo().uordblks);
			printConsole("linear free: %lu", (unsigned long)linearSpaceFree());
		}

	}

	//whether we left via STATE_QUIT or HOME (aptMainLoop), one state is still live
	STATES[State].end();

	// Exit services
	C3D_RenderTargetDelete(top);
	C3D_RenderTargetDelete(bottom);
	C2D_Fini();
	C3D_Fini();
	romfsExit();
	gfxExit();
	return 0;
}

