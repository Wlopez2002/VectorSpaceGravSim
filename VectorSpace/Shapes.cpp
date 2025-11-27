#include "Shapes.h"

static const float R3O2 = 0.8660254f; // root(3)/2
static const float R2O2 = 0.7071067f; // root(2)/2


// Uses a unit circle to draw a rough circle
void drawCircle(SDL_Renderer* renderer, float cx, float cy, float r)
{
	// Hard coded unit circle points
	SDL_FPoint points[17];
	points[0].x = cx ;				points[0].y = cy + r;
	points[1].x = cx + 0.5f * r;	points[1].y = cy + R3O2 * r;
	points[2].x = cx + R2O2 * r;	points[2].y = cy + R2O2 * r;
	points[3].x = cx + R3O2 * r;	points[3].y = cy + 0.5f * r;

	points[4].x = cx + r;			points[4].y = cy;
	points[5].x = cx + R3O2 * r;	points[5].y = cy + -0.5f * r;
	points[6].x = cx + R2O2 * r;	points[6].y = cy + -R2O2 * r;
	points[7].x = cx + 0.5f * r;		points[7].y = cy + -R3O2 * r;

	points[8].x = cx;				points[8].y = cy + -r;
	points[9].x = cx + -0.5f * r;	points[9].y = cy + -R3O2 * r;
	points[10].x = cx + -R2O2 * r;	points[10].y = cy + -R2O2 * r;
	points[11].x = cx + -R3O2 * r;	points[11].y = cy + -0.5f * r;

	points[12].x = cx + -r;			points[12].y = cy;
	points[13].x = cx + -R3O2 * r;	points[13].y = cy + 0.5f * r;
	points[14].x = cx + -R2O2 * r;	points[14].y = cy + R2O2 * r;
	points[15].x = cx + -0.5f * r;	points[15].y = cy + R3O2 * r;
	points[16].x = cx;				points[16].y = cy + r;

	SDL_RenderLines(renderer, points, 17);
}

void drawSquare(SDL_Renderer* renderer, float cx, float cy, float r) {
	SDL_FPoint points[5];
	points[0].x = cx - r;	points[0].y = cy - r;
	points[1].x = cx + r;	points[1].y = cy - r;
	points[2].x = cx + r;	points[2].y = cy + r;
	points[3].x = cx - r;	points[3].y = cy + r;
	points[4].x = cx - r;	points[4].y = cy - r;
	SDL_RenderLines(renderer, points, 5);
}

void drawTiltedSquare(SDL_Renderer* renderer, float cx, float cy, float r) {
	SDL_FPoint points[5];
	points[0].x = cx - r;	points[0].y = cy;
	points[1].x = cx;		points[1].y = cy + r;
	points[2].x = cx + r;	points[2].y = cy;
	points[3].x = cx;		points[3].y = cy - r;
	points[4].x = cx - r;	points[4].y = cy;
	SDL_RenderLines(renderer, points, 5);
}

void drawCity(SDL_Renderer* renderer, float cx, float cy, float r) {
	for (int i = 0; i < 7; i++) {
		float delta = (2 * 3.1415 / 7) * i;
		SDL_FPoint points[4];
		points[0].x = cx + 10;	points[0].y = cy - r;
		points[1].x = cx + 10;	points[1].y = cy - r - 40;
		points[2].x = cx - 10;	points[2].y = cy - r - 40;
		points[3].x = cx - 10;	points[3].y = cy - r;
		for (int i = 0; i < 4; i++) {
			// Rotation: https://academo.org/demos/rotation-about-point/
			float px = points[i].x; float py = points[i].y;
			px -= cx; py -= cy;
			float xnew = px * cos(delta) - py * sin(delta);
			float ynew = py * cos(delta) + px * sin(delta);
			points[i].x = xnew + cx;
			points[i].y = ynew + cy;
		}
		SDL_RenderLines(renderer, points, 4);
	}
}

void drawTriangle(SDL_Renderer* renderer, float cx, float cy, float r) {
	SDL_FPoint points[4];
	points[0].x = cx;		points[0].y = cy + r;
	points[1].x = cx - r;	points[1].y = cy - r;
	points[2].x = cx + r;	points[2].y = cy - r;
	points[3].x = cx;		points[3].y = cy + r;
	SDL_RenderLines(renderer, points, 4);
}