/*
* This is the primary file for all functions that draw to the renderer.
*/

#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>

void DrawCircle(SDL_Renderer* renderer, float centerX, float centerY, float radius);