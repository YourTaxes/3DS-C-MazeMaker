#include "main.h"





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
	MakeFont();

	top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	Init_Top();
	Init_Bottom();

	MakeColors();

	

	u32 kDown = 0, kHeld = 0, kUp = 0, kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information about keys detected in the previous frame

	printConsole(27, 1, "By Finnegan McDevitt");




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

		if (kDown & KEY_X){
			printInputs(normX, normY, kDown, kHeld, kUp, kDownOld, kHeldOld, kUpOld);
		}
		

		
		
		//check if player wants to change com speed
		if(kDown & KEY_B)
		{
			char buff[20];
			
			if (GetKeyboard(buff, 20, "Testing Keyboard", SWKBD_TYPE_NORMAL)){
				printConsole(25, 1, "%s", buff);
			} 
		}

		//TEMP TESTING
		if (kDown & KEY_A){
			printConsole(1, 1, "A pressed on debug state");
			State = STATE_MAZE_GAME;
			StateSwitch = true;
		}

		if (StateSwitch == true){
			printConsole(26, 1, "state switch is true");
		}


		//do frame logic
		switch (State){
		case STATE_DEBUG:
			
		case STATE_MAIN_MENU:
			//from the main menu, if the player presses A or , they go to the mazemaker game
			MainMenu_Logic(kDown, &rawLvl, &builtLvl, &StateSwitch, RebuildLevel);
		case STATE_MAZE_GAME:
			//if the player hits the button to stop,
			//then they will stop playing and go to the menu
			//this state includes 2 substates which are the win and lose screens.
		case STATE_MAZE_MAKER:
			//maze maker stuff
		case STATE_SAVE_SELECT:
			//save select
		case STATE_OOB:
			//press A to go back to main menu
		case STATE_you_recieved_the_egg:
			//him
	};
		
		
		
		//Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		//handle drawing
		switch (State){
		case STATE_DEBUG:
			Top_Tick(&State, top);
			Bottom_Tick(&State, bottom);
		case STATE_MAIN_MENU:
			//from the main menu, if the player presses A or , they go to the mazemaker game
			MainMenu_Draw(top, bottom);
		case STATE_MAZE_GAME:
			//if the player hits the button to stop,
			//then they will stop playing and go to the menu
			//this state includes 2 substates which are the win and lose screens.
		case STATE_MAZE_MAKER:
			//maze maker stuff
		case STATE_SAVE_SELECT:
			//save select
		case STATE_OOB:
			//press A to go back to main menu
		case STATE_you_recieved_the_egg:
			//him
		};
		
		
		


		//End the frame once, after every screen has been drawn
		C3D_FrameEnd(0);

		//handle state switching
		switch (State){
		case STATE_DEBUG:
			//end debug state
		case STATE_MAIN_MENU:
			//from the main menu, if the player presses A or , they go to the mazemaker game
			MainMenu_End(&State);
		case STATE_MAZE_GAME:
			//if the player hits the button to stop,
			//then they will stop playing and go to the menu
			//this state includes 2 substates which are the win and lose screens.
		case STATE_MAZE_MAKER:
			//maze maker stuff
		case STATE_SAVE_SELECT:
			//save select
		case STATE_OOB:
			//press A to go back to main menu
		case STATE_you_recieved_the_egg:
			//him
		};

		if (kDown & KEY_Y){
			printConsole(10, 1, "CPU:     %6.2f%%\x1b[K", C3D_GetProcessingTime()*6.0f);
			printConsole(11, 1, "GPU:     %6.2f%%\x1b[K", C3D_GetDrawingTime()*6.0f);
			printConsole(12, 1, "CmdBuf:  %6.2f%%\x1b[K", C3D_GetCmdBufUsage()*100.0f);
		}

		//Wait for VBlank
		gspWaitForVBlank();
	}

	// Exit services
	if (font) C2D_FontFree(font);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}

