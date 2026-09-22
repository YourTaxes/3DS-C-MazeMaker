#include "utils/graphics.h"
#include "utils/debug.h"

u32 Colors[11];

/*
* creates the colors for the color array
*/
void MakeColors(void){
    Colors[CLR_RED] = C2D_Color32(255, 0, 0, 255);
    Colors[CLR_ORANGE] = C2D_Color32(255, 200, 0, 255);
    Colors[CLR_YELLOW] = C2D_Color32(255, 255, 0, 255);
    Colors[CLR_GREEN] = C2D_Color32(0, 255, 0, 255);
    Colors[CLR_CYAN] = C2D_Color32(0, 255, 255, 255);
    Colors[CLR_BLUE] = C2D_Color32(0, 0, 255, 255);
    Colors[CLR_LT_GRAY] = C2D_Color32(192, 192, 192, 255);
    Colors[CLR_GRAY] = C2D_Color32(128, 128, 128, 255);
    Colors[CLR_DK_GRAY] = C2D_Color32(64, 64, 64, 255);
    Colors[CLR_BLACK] = C2D_Color32(0, 0, 0, 255);
    Colors[CLR_WHITE] = C2D_Color32(255, 255, 255, 255);
}


void DrawRect(const Rect* rect){
    C2D_DrawRectSolid(rect->x, rect->y, rect->z, rect->width, rect->height, rect->Color);
}

bool Rect_Contains(const Rect* rect, int px, int py){
    return px >= rect->x && px < rect->x + rect->width
        && py >= rect->y && py < rect->y + rect->height;
}

bool Rect_Tapped(const Rect* rect, const FrameInput* in){
    //libctru sets KEY_TOUCH in the key bitmask while the screen is touched,
    //so kDown & KEY_TOUCH is "the touch began this frame"
    return (in->kDown & KEY_TOUCH) && Rect_Contains(rect, in->touch.px, in->touch.py);
}


void MakeText(const char* str, C2D_Text* result, C2D_TextBuf buf)
{
    const char* indicator = C2D_TextParse(result, buf, str);
    C2D_TextOptimize(result);

    if (indicator == NULL) {
        printConsole("MakeText: parse failed for \"%s\"", str);
        *result = (C2D_Text){0}; //draws nothing
    } else if (*indicator != '\0') {
        printConsole("MakeText: buffer full, stopped at '%c' in \"%s\"", *indicator, str);
    }
}

void DrawTextCentered(const C2D_Text* text, float centerX, float centerY, float scaleX, float scaleY, u32 color){
    float width, height;
    C2D_TextGetDimensions(text, scaleX, scaleY, &width, &height);

    float drawX = centerX - (width / 2.0f);
    float drawY = centerY - (height / 2.0f);

    C2D_DrawText(text, C2D_WithColor, drawX, drawY, 1.0f, scaleX, scaleY, color);
}

void DrawTextInRect(const C2D_Text* text, const Rect* rect, float scale, u32 color){
    DrawTextCentered(text, rect->x + rect->width / 2.0f, rect->y + rect->height / 2.0f, scale, scale, color);
}
