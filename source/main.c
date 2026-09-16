#include "main.h"




//display defines


//global variables

//player positions


int main(int argc, char **argv)
{
	// Initialize services
	gfxInitDefault();

	#ifdef BOTH
		// no on-screen console in the both-screen build, so route stderr
		// to the attached debugger (GDB / Azahar log) via svcOutputDebugString
		consoleDebugInit(debugDevice_SVC);
	#endif

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	romfsInit();
	MakeFont();

	#ifdef TOP
		Init_Top();
	#endif

	#ifdef BOTTOM
		Init_Bottom();
	#endif

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

		if (kDown & KEY_X){
			printInputs(&circle_pad, kDown, kHeld, kUp, kDownOld, kHeldOld, kUpOld);
		}
		//Print the CirclePad position
		

		
		
		//check if player wants to change com speed
		if(kDown & KEY_B)
		{
			char buff[20];
			
			if (GetKeyboard(buff, 20, "Testing Keyboard", SWKBD_TYPE_NORMAL)){
				printConsole(25, 1, "%s", buff);
			} else {
				#ifndef BOTH
					printConsole(25, 1, "                    ");
				#endif
			}
		}
		
		
		//Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		
		#ifdef TOP
			Top_Tick();
		#endif

		#ifdef BOTTOM
			Bottom_Tick();
		#endif

		//End the frame once, after every screen has been drawn
		C3D_FrameEnd(0);

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
