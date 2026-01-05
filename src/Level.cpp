#include "Level.h"
#include <SDL.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

Level::Level()
    : bgTexture(nullptr)
    , frameWidth(800)
    , frameHeight(600)
    , bgOffset(0.0f)
    , scrollSpeed(100.0f)
    , rows(10)
    , cols(16)
    , bgRepeat(true)
    , parallax(0.5f)
    , prevCamX(0.0f)
    , prevCamValid(false)
    , bgMaxSpeed(-1.0f)
{
    grid.assign(rows, std::vector<int>(cols, 0));
}

Level::~Level() = default;

void Level::setBackgroundTexture(SDL_Texture* tex) {
    bgTexture = tex;
    prevCamValid = false; // force snap next time camera update runs
}

void Level::setBackgroundRepeat(bool repeat) {
    bgRepeat = repeat;
}

void Level::setFrameSize(int width, int height) {
    frameWidth = width;
    frameHeight = height;
    prevCamValid = false; // frame size change may change mapping; snap
}

void Level::setScrollSpeed(float speed) {
    scrollSpeed = speed;
}

void Level::setParallax(float factor) {
    parallax = factor;
    if (parallax < 0.0f) parallax = 0.0f;
    if (parallax > 1.0f) parallax = 1.0f;
}

void Level::setBackgroundMaxSpeed(float pxPerSec) {
    bgMaxSpeed = pxPerSec;
}

int Level::getFrameWidth() const {
    return frameWidth;
}

int Level::getFrameHeight() const {
    return frameHeight;
}

void Level::setBackgroundOffsetFromCamera(float camX, float maxCam, float dt) {
    if (!bgTexture) return;
    int texW = 0, texH = 0;
    if (SDL_QueryTexture(bgTexture, nullptr, nullptr, &texW, &texH) != 0) return;
    if (texH == 0) return;

    float scale = static_cast<float>(frameHeight) / static_cast<float>(texH);
    int scaledW = static_cast<int>(texW * scale);
    if (scaledW <= 0) return;

    int maxOffset = std::max(0, scaledW - frameWidth);

    // max player speed calc
    float maxStep = (bgMaxSpeed > 0.0f) ? (bgMaxSpeed * dt) : std::numeric_limits<float>::infinity();

    // If no previous camera value, snap to the mapped position.
    if (!prevCamValid) {
        if (bgRepeat) {
            float desired = camX * parallax;
            if (scaledW > 0) {
                bgOffset = std::fmod(desired, static_cast<float>(scaledW));
                if (bgOffset < 0.0f) bgOffset += static_cast<float>(scaledW);
            } else {
                bgOffset = 0.0f;
            }
        } else {
            if (maxCam <= 0.0f || maxOffset == 0) {
                bgOffset = 0.0f;
            } else {
                float ratio = camX / maxCam;
                ratio = std::max(0.0f, std::min(1.0f, ratio));
                // snap to full mapped range for non-repeating backgrounds
                bgOffset = ratio * static_cast<float>(maxOffset);
            }
        }
        prevCamX = camX;
        prevCamValid = true;
        return;
    }

    // compute camera delta and apply proportional movement for consistent linear response
    float delta = camX - prevCamX;
    prevCamX = camX;

    if (delta == 0.0f) return;

    if (bgRepeat) {
        // repeating backgrounds: move directly by camera delta scaled by parallax, then wrap
        float desiredMove = delta * parallax;

        // clamp by maxStep if configured
        if (maxStep < std::numeric_limits<float>::infinity()) {
            if (desiredMove > maxStep) desiredMove = maxStep;
            if (desiredMove < -maxStep) desiredMove = -maxStep;
        }

        bgOffset += desiredMove;

        if (scaledW > 0) {
            bgOffset = std::fmod(bgOffset, static_cast<float>(scaledW));
            if (bgOffset < 0.0f) bgOffset += static_cast<float>(scaledW);
        } else {
            bgOffset = 0.0f;
        }
    } else {
        // non-repeating: map camera movement to background movement using maxOffset / maxCam
        if (maxCam <= 0.0f || maxOffset == 0) {
            bgOffset = 0.0f;
            return;
        }
        float scalePerCam = static_cast<float>(maxOffset) / maxCam;
        float move = delta * scalePerCam; // full-range mapping

        // clamp by maxStep if configured
        if (maxStep < std::numeric_limits<float>::infinity()) {
            if (move > maxStep) move = maxStep;
            if (move < -maxStep) move = -maxStep;
        }

        bgOffset += move;

        // clamp to valid range
        if (bgOffset < 0.0f) bgOffset = 0.0f;
        if (bgOffset > static_cast<float>(maxOffset)) bgOffset = static_cast<float>(maxOffset);
    }
}

void Level::updateBackground(float dt) {
    if (!bgTexture) return;
    if (scrollSpeed == 0.0f) return; // preserve camera-driven behavior when scrollSpeed == 0

    int texW = 0, texH = 0;
    SDL_QueryTexture(bgTexture, nullptr, nullptr, &texW, &texH);
    if (texH == 0) return;

    float scale = static_cast<float>(frameHeight) / static_cast<float>(texH);
    int scaledW = static_cast<int>(texW * scale);
    if (scaledW <= 0) return;

    bgOffset += scrollSpeed * dt;

    if (bgRepeat) {
        bgOffset = std::fmod(bgOffset, static_cast<float>(scaledW));
        if (bgOffset < 0.0f) bgOffset += static_cast<float>(scaledW);
    } else {
        int maxOffset = std::max(0, scaledW - frameWidth);
        if (bgOffset < 0.0f) bgOffset = 0.0f;
        if (bgOffset > static_cast<float>(maxOffset)) bgOffset = static_cast<float>(maxOffset);
    }
}

void Level::renderBackground(SDL_Renderer* renderer) {
    if (!bgTexture || !renderer) return;

    int texW = 0, texH = 0;
    SDL_QueryTexture(bgTexture, nullptr, nullptr, &texW, &texH);
    if (texH == 0) return;

    float scale = static_cast<float>(frameHeight) / static_cast<float>(texH);
    int scaledW = static_cast<int>(texW * scale);
    int scaledH = frameHeight;
    if (scaledW <= 0) return;

    int startX = -static_cast<int>(bgOffset);

    if (bgRepeat) {
        for (int x = startX; x < frameWidth; x += scaledW) {
            SDL_Rect dst{ x, 0, scaledW, scaledH };
            SDL_RenderCopy(renderer, bgTexture, nullptr, &dst);
        }
    } else {
        SDL_Rect dst{ startX, 0, scaledW, scaledH };
        SDL_RenderCopy(renderer, bgTexture, nullptr, &dst);
    }
}

void Level::toggleCell(int r, int c) {
    if (r < 0 || c < 0) return;
    // ensure grid has enough rows/cols
    if (r >= rows) {
        grid.resize(r + 1);
        for (int i = rows; i <= r; ++i) grid[i].assign(cols, 0);
        rows = r + 1;
    }
    if (c >= cols) {
        for (auto &row : grid) row.resize(c + 1, 0);
        cols = c + 1;
    }
    // toggle between 0 and 1
    grid[r][c] = (grid[r][c] == 0) ? 1 : 0;
}

void Level::ensureCell(int r, int c) {
    if (r < 0 || c < 0) return;
    if (r >= rows) {
        grid.resize(r + 1);
        for (int i = rows; i <= r; ++i) grid[i].assign(cols, 0);
        rows = r + 1;
    }
    if (c >= cols) {
        for (auto &row : grid) row.resize(c + 1, 0);
        cols = c + 1;
    }
}

bool Level::saveToZip(const std::string& path) const {
    // For build/time reasons this writes a plain JSON-like dump to the given path.
    // Replace with a real .zip writer (minizip, libzip, etc.) when desired.
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;

    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"rows\": " << rows << ",\n";
    ss << "  \"cols\": " << cols << ",\n";
    ss << "  \"backgroundPath\": \"" << backgroundPath << "\",\n";
    ss << "  \"usedAssets\": [";
    for (size_t i = 0; i < usedAssets.size(); ++i) {
        if (i) ss << ", ";
        ss << "\"" << usedAssets[i] << "\"";
    }
    ss << "],\n";
    ss << "  \"grid\": [\n";
    for (int r = 0; r < rows; ++r) {
        ss << "    [";
        for (int c = 0; c < cols; ++c) {
            if (c) ss << ", ";
            int v = 0;
            if (r < static_cast<int>(grid.size()) && c < static_cast<int>(grid[r].size())) v = grid[r][c];
            ss << v;
        }
        ss << "]";
        if (r < rows - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";

    std::string data = ss.str();
    ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
    return ofs.good();
}