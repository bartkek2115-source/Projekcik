#pragma once
#include "Level.h"

class LevelEditor {
public:
    LevelEditor(Level* l, int w, int h, float scale = 1.0f, int baseTile = 32);
    void handleMouse(float mx, float my, float camX_editor_f);

private:
    Level* level;
    int windowW;
    int windowH;
    float tileScale;
    int baseTilePixels; // fixed tile size in pixels
};