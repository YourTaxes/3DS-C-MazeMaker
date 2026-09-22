#include "state_utils.h"




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