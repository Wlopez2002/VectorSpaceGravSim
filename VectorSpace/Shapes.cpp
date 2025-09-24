#include "Shapes.h"

static const float R3O2 = 0.8660254; // root(3)/2
static const float R2O2 = 0.7071067; // root(2)/2

// Uses a unit circle to draw a rough circle
void DrawCircle(SDL_Renderer* renderer, float cx, float cy, float r)
{
	// Hard coded unit circle points
	SDL_FPoint points[18];
	points[0].x = cx ;				points[0].y = cy + r;
	points[1].x = cx + 0.5 * r;		points[1].y = cy + R3O2 * r;
	points[2].x = cx + R2O2 * r;	points[2].y = cy + R2O2 * r;
	points[3].x = cx + R3O2 * r;	points[3].y = cy + 0.5 * r;

	points[4].x = cx + r;			points[4].y = cy;
	points[5].x = cx + R3O2 * r;	points[5].y = cy + -0.5 * r;
	points[6].x = cx + R2O2 * r;	points[6].y = cy + -R2O2 * r;
	points[7].x = cx + 0.5 * r;		points[7].y = cy + -R3O2 * r;

	points[8].x = cx;				points[8].y = cy + -r;
	points[9].x = cx + -0.5 * r;	points[9].y = cy + -R3O2 * r;
	points[10].x = cx + -R2O2 * r;	points[10].y = cy + -R2O2 * r;
	points[11].x = cx + -R3O2 * r;	points[11].y = cy + -0.5 * r;

	points[12].x = cx + -r;			points[12].y = cy;
	points[13].x = cx + -R3O2 * r;	points[13].y = cy + 0.5 * r;
	points[14].x = cx + -R2O2 * r;	points[14].y = cy + R2O2 * r;
	points[15].x = cx + -0.5 * r;	points[15].y = cy + R3O2 * r;
	points[16].x = cx;				points[16].y = cy + r;

	SDL_RenderLines(renderer, points, 17);
}