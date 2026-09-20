#include "state_utils.h"




void init_Rect(Rect** rect, float x, float y, float z, float width, float height, u32 color){
    *rect = calloc(1, sizeof(Rect));
    (*rect)->x = x;
    (*rect)->y = y;
    (*rect)->z = z;
    (*rect)->width = width;
    (*rect)->height = height;
    (*rect)->Color = color;
}

void DrawRect(Rect* rect){
    C2D_DrawRectSolid(rect->x, rect->y, rect->z, rect->width, rect->height, rect->Color);
}

bool touchingRect(Rect* rect, touchPosition* touch){
    float tx = (float)touch->px;
    float ty = (float)touch->py;

    //hidTouchRead gives 0,0 when the screen is not being touched
    bool inside = (touch->px != 0 || touch->py != 0)
        && tx >= rect->x && tx < rect->x + rect->width
        && ty >= rect->y && ty < rect->y + rect->height;

    //only true on the frame we go from outside to inside
    bool entered = inside && !rect->wasTouched;
    rect->wasTouched = inside;
    return entered;
}




void MakeText(char* str, C2D_Text* result)
{
    C2D_TextBuf buff = C2D_TextBufNew(strlen(str));
	C2D_TextBufClear(buff);
	const char* indicator = C2D_TextParse(result, buff, str);
	C2D_TextOptimize(result);

    //printConsole("Char Array = %s", str);
    if (indicator == NULL) {
        printConsole("Indicator is Null");
    } else if (*indicator != '\0') {
        printConsole("Indicator is %c", *indicator);
    }
	return;
	
}

void DrawTextCentered(C2D_Text* text, float centerX, float centerY, float scaleX, float scaleY, u32 color){
    float width, height;
    C2D_TextGetDimensions(text, scaleX, scaleY, &width, &height);

    float drawX = centerX - (width / 2.0f);
    float drawY = centerY - (height / 2.0f);

    C2D_DrawText(text, C2D_WithColor, drawX, drawY, 1.0f, scaleX, scaleY, color);
}