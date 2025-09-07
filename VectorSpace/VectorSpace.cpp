#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>

#include "GameData.h"
#include "Shapes.h"
#include "BSLA.h"

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* letterTexture = NULL;

static double WINSCALE;
static int WINLENGTH = 1050;
static int WINHEIGHT = 800;

const bool* key_board_state = SDL_GetKeyboardState(NULL);

bool update(GameState* gameState);
bool render(GameState* gameState);
void renderText(std::string text, int x, int y, int kerning, int colorHex, double scale);

Uint64 DTNOW = SDL_GetPerformanceCounter();
Uint64 DTLAST = 0;
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("Hello World", WINLENGTH, WINHEIGHT, SDL_WINDOW_KEYBOARD_GRABBED, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderScale(renderer, WINLENGTH / WINLENGTH, WINLENGTH / WINLENGTH);
    SDL_SetWindowResizable(window, true);
    WINSCALE = (double)WINLENGTH / 1050.0;
    
    SDL_Surface* textBMPSurf = SDL_LoadBMP("Resources/font.bmp");
    letterTexture = SDL_CreateTextureFromSurface(renderer, textBMPSurf);
    if (!letterTexture) {
        SDL_Log("Couldn't load text texture: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_DestroySurface(textBMPSurf);

    GameState* gameState = new GameState;
    gameState->player = new PlayerShip();
    gameState->player->forceLocation(Vector2D(WINLENGTH / 2, 200));

    randSystemAt(Vector2D(0, 0), std::time(0), gameState, 1000);

    gameState->deltaT = 0;
    DTNOW = SDL_GetPerformanceCounter();

    (*appstate) = gameState;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    GameState* gameState = static_cast<GameState*> (appstate);
    for (auto body : gameState->staticGravBodies) {
        delete(body);
    }
    for (auto body : gameState->dynamicGravBodies) {
        delete(body);
    }
    gameState->staticGravBodies.clear();
    gameState->dynamicGravBodies.clear();
    delete(gameState->player);
    delete(gameState);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    GameState* gameState = static_cast<GameState*> (appstate);
    if (event->key.key == SDLK_ESCAPE ||
        event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        SDL_GetWindowSize(window, &WINLENGTH, &WINHEIGHT);
        // TODO
        //WINSCALE = (double) WINLENGTH / 1050.0;
    }

    Vector2D moveVect(0, 0);
    double pMoveSpeed = gameState->player->getThrust();

    if (key_board_state[SDL_SCANCODE_Q]) {
        gameState->player->incrementThrust(-8 * gameState->deltaT);
    }
    if (key_board_state[SDL_SCANCODE_E]) {
        gameState->player->incrementThrust(8 * gameState->deltaT);
    }
    if (key_board_state[SDL_SCANCODE_W]) {
        moveVect = moveVect + Vector2D(0, -pMoveSpeed);
    }
    if (key_board_state[SDL_SCANCODE_S]) {
        moveVect = moveVect + Vector2D(0, pMoveSpeed);
    }
    if (key_board_state[SDL_SCANCODE_A]) {
        moveVect = moveVect + Vector2D(-pMoveSpeed, 0);
    }
    if (key_board_state[SDL_SCANCODE_D]) {
        moveVect = moveVect + Vector2D(pMoveSpeed, 0);
    }

    gameState->player->deltaSpeed(moveVect);

    if (event->type == SDL_EVENT_KEY_DOWN) {
        switch (event->key.key)
        {
        case SDLK_SPACE:
            gameState->player->doBrake();
            break;
        default:
            break;
        }
    }
    if (event->type == SDL_EVENT_KEY_UP) {
        switch (event->key.key)
        {
        case SDLK_SPACE:
            gameState->player->unbrake();
            break;
        default:
            break;
        }
    }
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void* appstate)
{
    GameState* gameState = static_cast<GameState*> (appstate);
    if (!update(gameState)) {
        return SDL_APP_SUCCESS;
    }
    render(gameState);

    return SDL_APP_CONTINUE;
}

bool update(GameState* gameState) {
    // Calculate deltaT
    DTLAST = DTNOW;
    DTNOW = SDL_GetPerformanceCounter();
    gameState->deltaT = ((double)((DTNOW - DTLAST) * 1000 / (double)SDL_GetPerformanceFrequency())) * 0.001;

    gameState->player->update(gameState);
    for (auto body : gameState->dynamicGravBodies) {
        body->update(gameState);
    }
    return true;
}

bool render(GameState* gameState) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_SetRenderScale(renderer, WINSCALE, WINSCALE);

    double pxoffset = gameState->player->getLocation().x - WINLENGTH / 2;
    double pyoffset = gameState->player->getLocation().y - WINHEIGHT / 2;

    DrawCircle(renderer, WINLENGTH/2, WINHEIGHT/2, 12);

    for (auto body : gameState->staticGravBodies) {
        DrawCircle(renderer, body->location.x - pxoffset, body->location.y - pyoffset, body->radius);
    }
    for (auto body : gameState->dynamicGravBodies) {
        DrawCircle(renderer, body->location.x - pxoffset, body->location.y - pyoffset, body->radius);
    }

    renderText("Player Location" + gameState->player->getLocation().toString(), 10, 5, 24, 0x00FFFF, 0.5);
    renderText("Player Speed   " + gameState->player->getSpeed().toString(), 10, 40, 24, 0x00FFFF, 0.5);
    renderText("Gravity Vector " + gameState->player->getGravDelta().toString(), 10, 80, 24, 0x00FFFF, 0.5);
    renderText("Movement Vector" + gameState->player->getPlayerDelta().toString(), 10, 120, 24, 0x00FFFF, 0.5);
    renderText("Thrust " + std::to_string(gameState->player->getThrust()), 10, 160, 24, 0x00FFFF, 0.5);
    renderText("Player Health " + std::to_string(gameState->player->getHealth()), 10, 200, 24, 0x00FFFF, 0.5);
    if (gameState->player->isParked()) {
        renderText("parked = true", 1000, 5, 24, 0x00FFFF, 0.5);
    }
    else {
        renderText("parked = false", 1000, 5, 24, 0x00FFFF, 0.5);
    }
    if (gameState->player->isParked()) {
        renderText("moving = true", 1000, 40, 24, 0x00FFFF, 0.5);
    }
    else {
        renderText("moving = false", 1000, 40, 24, 0x00FFFF, 0.5);
    }
    

    SDL_RenderPresent(renderer);
    return 1;
}

void renderText(std::string text, int x, int y, int kerning, int colorHex, double scale) {
    SDL_SetRenderScale(renderer, WINSCALE * scale, WINSCALE * scale);
    SDL_FRect rect;
    SDL_FRect selectRect;
    rect.x = x; rect.y = y;
    rect.h = 32; rect.w = 32;
    selectRect.h = 32; selectRect.w = 32;

    if (!SDL_SetTextureColorMod(letterTexture, (colorHex & 0xFF0000) >> 16, (colorHex & 0x00FF00) >> 8, colorHex & 0x0000FF)) {
        std::cout << "set color failure\n";
    }

    for (char c : text) {
        int choice = c - 32;
        if (c == '\n') {
            rect.y += 32;
            rect.x = x;
        }
        if (choice < 0 || choice > 127 - 32) {
            continue;
        }
        int getx = choice % 11;
        int gety = choice / 11;

        selectRect.x = getx * 32; selectRect.y = gety * 32;

        SDL_RenderTexture(renderer, letterTexture, &selectRect, &rect);
        rect.x += kerning;
    }
    if (!SDL_SetTextureColorMod(letterTexture, 255, 255, 255)) {
        std::cout << "reset color failure\n";
    }
    SDL_SetRenderScale(renderer, WINSCALE, WINSCALE);
}