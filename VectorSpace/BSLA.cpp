#include "BSLA.h"

Vector2D rotateVector2D(Vector2D toRotate, float delta) {
	Matrix2D rm = Matrix2D(cos(delta), -1 * sin(delta), sin(delta), cos(delta));
	return rm * toRotate;
}