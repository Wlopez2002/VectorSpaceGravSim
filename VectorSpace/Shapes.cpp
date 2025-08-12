#include "Shapes.h";


void DrawCircle(SDL_Renderer* renderer, float centreX, float centreY, float radius)
{
    const float diameter = (radius * 2);

    float x = (radius - 1);
    float y = 0;
    float tx = 1;
    float ty = 1;
    float error = (tx - diameter);

    while (x >= y)
    {
        //  Each of the following renders an octant of the circle
        SDL_RenderPoint(renderer, centreX + x, centreY - y);
        SDL_RenderPoint(renderer, centreX + x, centreY + y);
        SDL_RenderPoint(renderer, centreX - x, centreY - y);
        SDL_RenderPoint(renderer, centreX - x, centreY + y);
        SDL_RenderPoint(renderer, centreX + y, centreY - x);
        SDL_RenderPoint(renderer, centreX + y, centreY + x);
        SDL_RenderPoint(renderer, centreX - y, centreY - x);
        SDL_RenderPoint(renderer, centreX - y, centreY + x);

        if (error <= 0)
        {
            ++y;
            error += ty;
            ty += 2;
        }

        if (error > 0)
        {
            --x;
            tx += 2;
            error += (tx - diameter);
        }
    }
}