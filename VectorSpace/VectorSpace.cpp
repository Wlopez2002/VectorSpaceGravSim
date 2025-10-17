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
void renderMenu(GameState* gameState);
void renderGame(GameState* gameState);
void renderText(std::string text, int x, int y, int kerning, int FontSize);

Uint64 DTNOW = SDL_GetPerformanceCounter();
Uint64 DTLAST = 0;
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("Vector Space", WINLENGTH, WINHEIGHT, SDL_WINDOW_KEYBOARD_GRABBED, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowResizable(window, true);
    WINSCALE = (double)WINLENGTH / 1050.0;
    SDL_SetRenderScale(renderer, WINSCALE, WINSCALE);

    SDL_StartTextInput(window);
    
    SDL_Surface* textBMPSurf = SDL_LoadBMP("Resources/font.bmp");
    letterTexture = SDL_CreateTextureFromSurface(renderer, textBMPSurf);
    if (!letterTexture) {
        SDL_Log("Couldn't load text texture: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_DestroySurface(textBMPSurf);

    GameState* gameState = new GameState;
    gameState->curState = StageStart;
    gameState->menuSelectorY = 0;
    gameState->seed = (int)std::time(0);
    gameState->seedStringBuffer = std::to_string(gameState->seed);
    gameState->debugMode = false;
    gameState->player = new PlayerShip();
    gameState->player->forceLocation(Vector2D(0, 0));

    //generatePlaySpace(1000, 500, std::time(0), gameState);

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
        //WINSCALE = (double)WINLENGTH / 1050.0;
        SDL_SetRenderScale(renderer, WINSCALE, WINSCALE);
    }

    Vector2D moveVect(0, 0);
    double pMoveSpeed = gameState->player->getThrust();
    switch (gameState->curState)
    {
    case StageStart:
        if (event->type == SDL_EVENT_KEY_DOWN) {
            if (event->key.key == SDLK_SPACE) {
                switch (gameState->menuSelectorY)
                {
                case 0:
                    gameState->seed = std::stoi(gameState->seedStringBuffer);
                    resetGameState(gameState);
                    gameState->curState = StagePlay;
                    break;
                case 1:
                    break;
                case 2:
                    gameState->debugMode = !gameState->debugMode;
                    break;
                default:
                    break;
                }

            }
            if (key_board_state[SDL_SCANCODE_W]) {
                gameState->menuSelectorY--;
                if (gameState->menuSelectorY < 0) {
                    gameState->menuSelectorY = 0;
                }
            }
            if (key_board_state[SDL_SCANCODE_S]) {
                gameState->menuSelectorY++;
                if (gameState->menuSelectorY > 6) {
                    gameState->menuSelectorY = 6;
                }
            }
            if (key_board_state[SDL_SCANCODE_BACKSPACE]) {
                switch (gameState->menuSelectorY)
                {
                case 0:
                    break;
                case 1:
                    if (!gameState->seedStringBuffer.empty()) {
                        gameState->seedStringBuffer.pop_back();
                    }
                    break;
                case 2:
                    break;
                default:
                    break;
                }
            }
        }
        if (event->type == SDL_EVENT_TEXT_INPUT) {
            char key = *(event->text.text);
            switch (gameState->menuSelectorY)
            {
            case 0:
                break;
            case 1:
                if (key <= 57 and key >= 48) { // the ascii range for numbers
                    gameState->seedStringBuffer.push_back(key);
                }
                break;
            case 2:
                break;
            default:
                break;
            }
        }
        break;
    case StagePlay:
        if (key_board_state[SDL_SCANCODE_Q]) {
            gameState->player->incrementThrust(-12 * gameState->deltaT);
        }
        if (key_board_state[SDL_SCANCODE_E]) {
            gameState->player->incrementThrust(12 * gameState->deltaT);
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
        break;
    default:
        break;
    }

    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void* appstate)
{
    GameState* gameState = static_cast<GameState*> (appstate);
    if (!update(gameState)) {
        return SDL_APP_SUCCESS;
    }
    switch (gameState->curState)
    {
    case StageStart:
        renderMenu(gameState);
        break;
    case StagePlay:
        renderGame(gameState);
        break;
    default:
        break;
    }
    

    return SDL_APP_CONTINUE;
}

bool update(GameState* gameState) {
    // Calculate deltaT
    DTLAST = DTNOW;
    DTNOW = SDL_GetPerformanceCounter();
    gameState->deltaT = ((double)((DTNOW - DTLAST) * 1000 / (double)SDL_GetPerformanceFrequency())) * 0.001;

    switch (gameState->curState)
    {
    case StageStart:
        break;
    case StagePlay:
        for (auto body : gameState->dynamicGravBodies) {
            body->update(gameState);
        }
        gameState->player->update(gameState);
        break;
    default:
        break;
    }


    return true;
}


void renderMenu(GameState* gameState) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    renderText("Start", 30, 30, 32, 32);
    renderText("Seed       " + gameState->seedStringBuffer, 30, 100, 20, 20);
    if (gameState->debugMode) {
        renderText("Debug Mode true", 30, 140, 20, 20);
    }
    else {
        renderText("Debug Mode false", 30, 140, 20, 20);
    }

    if (gameState->menuSelectorY == 0) {
        DrawCircle(renderer, 15, 46, 8);
    }
    else {
        DrawCircle(renderer, 15, 110 + 40 * (gameState->menuSelectorY - 1), 8);
    }

    SDL_RenderPresent(renderer);
}

void renderGame(GameState* gameState) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    double pxoffset = gameState->player->getLocation().x - WINLENGTH / 2;
    double pyoffset = gameState->player->getLocation().y - WINHEIGHT / 2;

    // Draw Player
    DrawCircle(renderer, WINLENGTH/2, WINHEIGHT/2, 12);

    // Draw Bodies
    for (auto body : gameState->staticGravBodies) {
        double dist = (body->location - gameState->player->getLocation()).magnitude() - body->radius;
        if (dist < WINLENGTH) { // check if the body can be seen by the player
            DrawCircle(renderer, body->location.x - pxoffset, body->location.y - pyoffset, body->radius);
        }
    }
    for (auto body : gameState->dynamicGravBodies) {
        double dist = (body->location - gameState->player->getLocation()).magnitude() - body->radius;
        if (dist < WINLENGTH) { // check if the body can be seen by the player
            DrawCircle(renderer, body->location.x - pxoffset, body->location.y - pyoffset, body->radius);
        }
    }

    // Draw UI
    if (gameState->debugMode) {
        renderText("Player Location" + gameState->player->getLocation().toString(), 10, 10, 12, 12);
        renderText("Player Speed   " + gameState->player->getSpeed().toString(), 10, 30, 12, 12);
        renderText("Gravity Vector " + gameState->player->getGravDelta().toString(), 10, 50, 12, 12);
        renderText("Movement Vector" + gameState->player->getPlayerDelta().toString(), 10, 70, 12, 12);
        renderText("Thrust " + std::to_string(gameState->player->getThrust()), 10, 90, 12, 12);
        renderText("Player Health " + std::to_string(gameState->player->getHealth()), 10, 110, 12, 12);
        renderText("WinHeight " + std::to_string(WINHEIGHT), 10, 130, 12, 12);
        renderText("WinLength " + std::to_string(WINLENGTH), 10, 150, 12, 12);
        renderText("Dt " + std::to_string(gameState->deltaT), 10, 750, 12, 12);
        if (gameState->player->isParked()) {
            renderText("parked = true", 600, 5, 12, 12);
        }
        else {
            renderText("parked = false", 600, 5, 12, 12);
        }
        if (gameState->player->isParked()) {
            renderText("moving = true", 600, 40, 12, 12);
        }
        else {
            renderText("moving = false", 600, 40, 12, 12);
        }
    }
    else {
        renderText("Location " + gameState->player->getLocation().toString(), 10, 10, 12, 12);
        renderText("Thrust   " + std::to_string(gameState->player->getThrust()), 10, 30, 12, 12);
        renderText("Health   " + std::to_string(gameState->player->getHealth()), 10, 50, 12, 12);
    }
    

    SDL_RenderPresent(renderer);
}

void renderText(std::string text, int x, int y, int kerning, int FontSize) {
    SDL_FRect rect;
    SDL_FRect selectRect;
    rect.x = x; rect.y = y;
    rect.h = FontSize; rect.w = FontSize;
    selectRect.h = 32; selectRect.w = 32;

    for (char c : text) {
        int choice = c - 32;
        if (c == '\n') {
            rect.y += FontSize;
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
}