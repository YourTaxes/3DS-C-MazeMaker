#include "main.h"
#include <malloc.h> // mallinfo, for the heap stats on KEY_Y





//display defines


//global variables
Game_State State = STATE_DEBUG;
bool RebuildLevel = true;
Raw_Level rawLvl;
Built_Level builtLvl;

C3D_RenderTarget* top;
C3D_RenderTarget* bottom;



/*
* runs the _Init of the current state so it allocates everything it needs.
* called once before the main loop, and again right after each state switch.
*/
static void InitCurrentState(void)
{
	switch (State){
	case STATE_DEBUG:
		Debug_Init();
		break;
	case STATE_MAIN_MENU:
		MainMenu_Init(&rawLvl, &builtLvl, &RebuildLevel);
		break;
	default:
		break;
	};
}

/*
* Does the framelogic function for the current selected state.
* returns the state to switch to, STATE_NONE to stay, or STATE_QUIT
*/
static Game_State StateFrameLogic(touchPosition* touch, u32 kDown){
	switch (State){
		case STATE_DEBUG:
			return Debug_logic(kDown);
		case STATE_MAIN_MENU:
			return MainMenu_Logic(kDown, touch, &rawLvl, &builtLvl, &RebuildLevel);
		case STATE_MAZE_GAME:
			//if the player hits the button to stop,
			//then they will stop playing and go to the menu
			//this state includes 2 substates which are the win and lose screens.
			break;
		case STATE_MAZE_MAKER:
			//maze maker stuff
			break;
		case STATE_SAVE_SELECT:
			//save select
			break;
		case STATE_OOB:
			//press A to go back to main menu
			break;
		case STATE_you_recieved_the_egg:
			//him
			break;
		default:
			break;
	};
	return STATE_NONE;
}

/*
* Draws the screen for the current selected state
*/
static void DrawState(){
	switch (State){
		case STATE_DEBUG:
			Debug_Draw(top, bottom);
			break;
		case STATE_MAIN_MENU:
			//from the main menu, if the player presses A or , they go to the mazemaker game
			MainMenu_Draw(top, bottom);
			break;
		case STATE_MAZE_GAME:
			//if the player hits the button to stop,
			//then they will stop playing and go to the menu
			//this state includes 2 substates which are the win and lose screens.
			break;
		case STATE_MAZE_MAKER:
			//maze maker stuff
			break;
		case STATE_SAVE_SELECT:
			//save select
			break;
		case STATE_OOB:
			//press A to go back to main menu
			break;
		case STATE_you_recieved_the_egg:
			//him
			break;
		default:
			break;
		};
}

/*
* runs the _End of whichever state is current so it frees everything it malloc'd.
* used both for the normal state switch and to tear down the live state on exit.
*/
static void EndCurrentState(void)
{
	switch (State){
	case STATE_DEBUG:
		Debug_end();
		break;
	case STATE_MAIN_MENU:
		MainMenu_End();
		break;
	case STATE_MAZE_GAME:
		//if the player hits the button to stop,
		//then they will stop playing and go to the menu
		//this state includes 2 substates which are the win and lose screens.
		break;
	case STATE_MAZE_MAKER:
		//maze maker stuff
		break;
	case STATE_SAVE_SELECT:
		//save select
		break;
	case STATE_OOB:
		//press A to go back to main menu
		break;
	case STATE_you_recieved_the_egg:
		//him
		break;
	default:
		break;
	};
}


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

	

	u32 kDown = 0, kHeld = 0, kUp = 0, kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information about keys detected in the previous frame

	printConsole("By Finnegan McDevitt");

	//from here until the loop exits there is always exactly one live state
	InitCurrentState();

	// Main loop
	bool running = true;
	while (running && aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		kDownOld = kDown;
		kHeldOld = kHeld;
		kUpOld = kUp;

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		kDown = hidKeysDown();
		//hidKeysHeld returns information about which buttons have are held down in this frame
		kHeld = hidKeysHeld();
		//hidKeysUp returns information about which buttons are released on a frame.
		kUp = hidKeysUp();

		circlePosition circle_pad;

		//Read the CirclePad position
		hidCircleRead(&circle_pad);

		float normX = 0.0f;
		float normY = 0.0f;
		normalizeCirclePad(&circle_pad, &normX, &normY);

		touchPosition touch;
		hidTouchRead(&touch);

		if (kDown & KEY_X){
			printInputs(normX, normY, kDown, kHeld, kUp, kDownOld, kHeldOld, kUpOld);
		}
		

		
		
		//keyboard test demo
		if(kDown & KEY_B)
		{
			char buff[20];
			
			if (GetKeyboard(buff, 20, "Testing Keyboard", SWKBD_TYPE_NORMAL)){
				printConsole("%s", buff);
			} 
		}





		//do frame logic
		Game_State next = StateFrameLogic(&touch, kDown);
		if (kDown & KEY_START) next = STATE_QUIT; // START always quits, from any state

		//Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		DrawState();
		//End the frame once, after every screen has been drawn
		C3D_FrameEnd(0);

		//handle state switching. the old state is drawn one last time above,
		//then torn down, and the new one is set up before the next frame.
		if (next == STATE_QUIT){
			running = false; //the live state is torn down after the loop
		} else if (next != STATE_NONE){
			EndCurrentState();
			State = next;
			InitCurrentState();
		}


		if (kDown & KEY_Y){
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
	EndCurrentState();

	// Exit services
	C3D_RenderTargetDelete(top);
	C3D_RenderTargetDelete(bottom);
	C2D_Fini();
	C3D_Fini();
	romfsExit();
	gfxExit();
	return 0;
}

