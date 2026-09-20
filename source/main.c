#include "main.h"
#include <malloc.h> // mallinfo, for the heap stats on KEY_Y





//display defines


//global variables
Game_State State = STATE_DEBUG;
bool StateSwitch = true;
bool RebuildLevel = true;
Raw_Level rawLvl;
Built_Level builtLvl;

C3D_RenderTarget* top;
C3D_RenderTarget* bottom;



//player positions

/*
* Does the framelogic function for the current selected state
*/
static void StateFrameLogic(touchPosition* touch, u32 kDown, bool* quitGame){
	switch (State){
		case STATE_DEBUG:
			Debug_logic(kDown, &StateSwitch);
			break;
		case STATE_MAIN_MENU:
			//from the main menu, if the player presses A or , they go to the mazemaker game
			*quitGame = MainMenu_Logic(kDown, touch, &rawLvl, &builtLvl, &StateSwitch, &RebuildLevel);
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
	};
}

/*
* Draws the screen for the current selected state
*/
static void DrawState(){
	switch (State){
		case STATE_DEBUG:
			Debug_Draw(top, bottom);
			break;
			//Top_Tick(&State, top);
			//Bottom_Tick(&State, bottom);
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
		Debug_end(&State);
		//end debug state
		break;
	case STATE_MAIN_MENU:
		//from the main menu, if the player presses A or , they go to the mazemaker game
		MainMenu_End(&State);
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

	//Init_Top();
	//Init_Bottom();

	MakeColors();

	

	u32 kDown = 0, kHeld = 0, kUp = 0, kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information about keys detected in the previous frame

	printConsole("By Finnegan McDevitt");

	bool quitGame = false;




	// Main loop
	while (aptMainLoop())
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

		if (kDown & KEY_START) break; // break in order to return to hbmenu

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
		

		StateFrameLogic(&touch, kDown, &quitGame);
		
		
		
		//Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		//handle drawing
		
		
		DrawState();
		


		//End the frame once, after every screen has been drawn
		C3D_FrameEnd(0);

		if (StateSwitch){
			//handle state switching
			EndCurrentState();
		}

		if (quitGame) break;


		if (kDown & KEY_Y){
			printConsole("CPU:     %6.2f%%", C3D_GetProcessingTime()*6.0f);
			printConsole("GPU:     %6.2f%%", C3D_GetDrawingTime()*6.0f);
			printConsole("CmdBuf:  %6.2f%%", C3D_GetCmdBufUsage()*100.0f);
			//guest-side memory: bytes currently malloc'd, and free linear (GPU) memory.
			//if these stay flat across state switches, the game itself is not leaking.
			printConsole("heap used:   %d", mallinfo().uordblks);
			printConsole("linear free: %lu", (unsigned long)linearSpaceFree());
		}

		//Wait for VBlank
		gspWaitForVBlank();
	}

	//the loop can exit (START / HOME) between a state's _Init and _End,
	//so tear down the live state here. StateSwitch == false means a state is live.
	if (!StateSwitch){
		EndCurrentState();
	}

	// Exit services
	C3D_RenderTargetDelete(top);
	C3D_RenderTargetDelete(bottom);
	C2D_Fini();
	C3D_Fini();
	romfsExit();
	gfxExit();
	return 0;
}

