#include "Texture.h"
#include <vector>
#include <SDL.h>

class Player {
public:
    float x = 100.f, y = 800.f;
    float vy = 0.f;
    bool onGround = false;
    std::vector<Texture*> frames;
    int curFrame = 0;
    double frameTime = 0;
    double frameDelay = 150.0; // ms per frame
    int width = 64, height = 64;

    // Stats
    int health = 3;
    int score = 0;
    float invuln = 0.5f;      // seconds of invulnerability after taking damage
    float invulnTimer = 0.0f; // timer for invulnerability
    bool facingLeft = false;

    void update(double dt, const Uint8* kb);
    void render(SDL_Renderer* r, int camX, int camY, float renderScale = 1.0f);
};