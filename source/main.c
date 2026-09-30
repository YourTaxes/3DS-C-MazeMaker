#include <malloc.h> // mallinfo, for the heap stats on KEY_GPIO14
#include <3ds.h>
#include <citro2d.h>
#include "utils/debug.h"
#include "utils/input.h"
#include "utils/graphics.h"
#include "datatypes/game_state.h"
#include "datatypes/level_file.h"
#include "datatypes/save_file.h"
#include "states/MainMenu/MainMenu.h"
#include "states/Debug/Debug_state.h"
#include "states/LevelSelect/LevelSelect.h"
#include "states/forgotten/forgotten.h"


//global variables
Game_State State = STATE_MAIN_MENU;
Raw_Level rawLvl;
Built_Level builtLvl;
//the level starts out needing a build, since nothing has been compiled yet.
//the game boots on save slot 0.
GameContext ctx = { .rawLvl = &rawLvl, .builtLvl = &builtLvl, .rebuildLevel = true, .curSlot = 0, .cur_time = 0 };
C3D_RenderTarget* top_left;
C3D_RenderTarget* bottom;
C3D_RenderTarget* top_right;



/*
* one entry per Game_State. to add a state, write its four functions and add a line here.
* states not listed yet are all-NULL, and the main loop refuses to switch to them.
*
* STATE_MAZE_GAME   - play the level. includes the win and lose screens as substates.
* STATE_MAZE_MAKER  - edit the raw level on the bottom screen.
* STATE_SAVE_SELECT - pick which of the save slots is the current level.
* STATE_OOB         - out of bounds screen, A returns to the main menu.
* STATE_you_recieved_the_egg - him
*/
static const StateFns STATES[STATE_COUNT] = {
	[STATE_MAIN_MENU] = { MainMenu_Init, MainMenu_Logic, MainMenu_DrawTop, MainMenu_DrawBottom, MainMenu_End },
	[STATE_DEBUG]     = { Debug_Init,    Debug_logic,    Debug_DrawTop,    Debug_DrawBottom,    Debug_end    },
	[STATE_SAVE_SELECT]	= { LevelSelect_Init, LevelSelect_Logic, LevelSelect_DrawTop, LevelSelect_DrawBottom, LevelSelect_End	},
	[STATE_you_recieved_the_egg] = { forgotten_Init, forgotten_Logic, forgotten_DrawTop, forgotten_DrawBottom, forgotten_End	}
};


int main(int argc, char **argv)
{
	// Initialize services
	gfxInitDefault();

	//Enables 3d, so that right and left top screens are drawn to.
	gfxSet3D(true);

	// no on-screen console (both screens are used for rendering), so route
	// stderr to the attached debugger (GDB / Azahar log) via svcOutputDebugString
	consoleDebugInit(debugDevice_SVC);

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	romfsInit();

	top_left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	top_right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	MakeColors();

	//every gui picture, loaded once out of romfs and held by graphics.c until FreeImages
	//below. states borrow these with GetImage and must never free what they get back.
	//a failure here is not worth refusing to boot over: a missing image just draws nothing.
	if (!LoadImages()){
		printConsole("some gui images did not load, continuing without them");
	}

	if (SaveFile_Ensure()){
		//read what the last selected slot was and set curSlot to it.
		if (!SaveFile_ReadLastSlot(&ctx.curSlot)){
			printConsole("there was an error reading the last slot, defaulting to 0");
			ctx.curSlot = 0;
		}
	}

	//the save file lives on the SD card. make sure it exists and is ours, then
	//boot with the current slot as the loaded level. if either step fails the
	//game still runs, on the zeroed global rawLvl marked as an empty slot.
	if (!SaveFile_Ensure() || !SaveFile_ReadSlot(ctx.curSlot, &rawLvl)){
		printConsole("no usable save file, continuing with an empty level");
		rawLvl.empty = true;
	}

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

		if (in.kDown & KEY_DEBUG){
			printInputs(&in);
		}

		if (in.kDown & KEY_GPIO14){
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
			//Print Vram debug, used for detecting memory leaks with the image previews
			printConsole("vram free:   %lu", (unsigned long)vramSpaceFree());
		}

		//do frame logic
		Game_State next = STATES[State].logic(&in, &ctx);
		if (in.kDown & KEY_START) next = STATE_QUIT; // START always quits, from any state



		//Render the scene. the top screen is drawn once per eye, shifted in opposite
		//directions, and each state spends that shift on its own depth layers.
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		STATES[State].drawTop(top_left, in.screenDepth);
		//with less than half a pixel different, then nothing changes, so 3d is off.
		if (in.screenDepth >= MIN_PARALLAX){
			STATES[State].drawTop(top_right, -in.screenDepth);
		}
		//drawTop left the model matrix shifted, and the bottom screen has only one eye
		C2D_ViewReset();
		STATES[State].drawBottom(bottom);
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

	}

	//whether we left via STATE_QUIT or HOME (aptMainLoop), one state is still live
	STATES[State].end();

	//write what the last selected slot was.
	SaveFile_WriteLastSlot(ctx.curSlot);

	//after the last state's end(), so nothing is still holding a borrowed C2D_Image, and
	//before C2D_Fini, because the sheets are citro2d objects
	FreeImages();

	// Exit services
	C3D_RenderTargetDelete(top_left);
	C3D_RenderTargetDelete(top_right);
	C3D_RenderTargetDelete(bottom);
	C2D_Fini();
	C3D_Fini();
	romfsExit();
	gfxExit();
	return 0;
}

