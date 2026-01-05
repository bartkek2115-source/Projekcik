#include "LevelEditor.h"
#include <algorithm>
#include <cmath>

LevelEditor::LevelEditor(Level* l, int w, int h, float scale, int baseTile)
    : level(l), windowW(w), windowH(h), tileScale(scale), baseTilePixels(baseTile) {}

void LevelEditor::handleMouse(float mx, float my, float camX_editor_f){
    if (!level) return;
    if (windowW <= 0 || windowH <= 0) return;

    float cellWf = std::max(1.0f, baseTilePixels * tileScale);
    float cellHf = cellWf;

    float worldX_editor_f = mx + camX_editor_f;
    float worldY_editor_f = my;

    int col = static_cast<int>(std::floor(worldX_editor_f / cellWf));
    int row = static_cast<int>(std::floor(worldY_editor_f / cellHf));

    if (row < 0 || col < 0) return;

    // Ensure the grid is large enough and cycle the cell
    // 0 -> 1 -> 2 -> 3 -> 0 (empty -> solid -> damaging -> pickup -> empty)
    level->ensureCell(row, col);
    level->grid[row][col] = (level->grid[row][col] + 1) % 4;
}