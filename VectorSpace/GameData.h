#pragma once
#include <vector>
#include <iostream>
#include <math.h>

#include "BSLA.h"

struct GameState;
class Body;
class DynamicGravBody;
class StaticGravBody;
class PlayerShip;

static const double GCONST = 2.0;

double calcGravity(double mass, double distance);
Vector2D getOrbitSpeed(Body* toOrbit, Vector2D myLocation);
Vector2D doGravity(GameState* state, Vector2D location);
Body* willCollide(GameState* state, Vector2D location);
Body* closestToPoint(GameState* state, Vector2D location);
void randSystemAt(Vector2D location, int seed, GameState* state, double systemRadius);
double randBodyOrbiting(Body* toOrbit, int seed, GameState* state, double distance, double maxRadius);

// taylor series approx of Sin and Cos derivative
static std::vector<double> taylorDSin = { 1.0, 0, -1.0 / 2, 0, 1.0 / 24, 0, -1.0 / 720, 0, 1.0 / 40320, 0, -1.0 / 3628800 };
static std::vector<double> tayloyDCos = { 0, -1.0, 0, 1.0 / 6, 0, -1.0 / 120, 0, 1.0 / 5040, 0, -1.0 / 362880 };

struct GameState {
	double deltaT;
	PlayerShip* player;
	std::vector<StaticGravBody*> staticGravBodies;
	std::vector<DynamicGravBody*> dynamicGravBodies;
};

class Body {
public:
	Vector2D speed = Vector2D(0, 0);
	Vector2D location;
	double radius;
	double mass;
	int bodyID;
};

// Bodies that move acording to some function
class DynamicGravBody : public Body {
public:
	Vector2D gravDelta;

	// Stuff function input
	double timetart = 0;
	double timeEnd = 999999;
	double timeCur = timetart;
	float deltaMul = 1; float XMul = 1; float YMul = 1;

	// 0 for using the set speed, 1 for rigid eliptical orbit about a point, 2 for a following the function vectors,
	// 3 for gravity movement
	char moveType = 0; 

	Body* orbitBody = nullptr; // the body to orbit

	// These follow a c0 + c1*x + c2*x^2 + ... + c3*x^i format where c is the value at the index of the vector
	// good for taylor series
	std::vector<double> functionX;
	std::vector<double> functionY;

	DynamicGravBody(Vector2D loc, double r, double m, char movet) {
		location = loc; radius = r; mass = m; moveType = movet;
	}
	DynamicGravBody(Vector2D loc, double r, double m, char movet, double tstart, double tend, float dm, float xm, float ym) {
		location = loc; radius = r; mass = m; moveType = movet;
		timetart = tstart; timeEnd = tend; deltaMul = dm; XMul = xm; YMul = ym;
	}
	void update(GameState* state) {
		timeCur += state->deltaT * deltaMul;
		if (timeCur > timeEnd) {
			timeCur = timetart;
		}

		// moveType dictates how speed is changed
		switch (moveType)
		{
		case 0: 
			// Based on whatever the speed already is
			break;
		case 1:
		{
			// rigid eliptical orbit about orbitPoint
			// calculate where we want to be then figure out speed
			if (orbitBody != nullptr) {
				Vector2D pastLocation = location;
				Vector2D newLocation = Vector2D(cos(timeCur) * XMul, sin(timeCur) * YMul);
				newLocation = newLocation + orbitBody->location;

				// Speed is not calculated based on where we want to be
				speed = newLocation - pastLocation;
				speed = speed * (1 / state->deltaT); // weight the speed to negate the later multiplication
			}
		}
		break;
		case 2:
		{
			// use function vectors
			int index = 0; double xComp = 0; double yComp = 0;
			for (double elem : functionX) {
				xComp += pow(timeCur, index) * elem;
				index++;
			}
			index = 0;
			for (double elem : functionY) {
				yComp += pow(timeCur, index) * elem;
				index++;
			}
			speed.x = xComp * XMul; speed.y = yComp * YMul;
		}
		break;
		case 3:
		{
			// Movement with gravity, this is the only body movement option that uses collision
			gravDelta = doGravity(state, location);
			Vector2D newSpeed = speed + gravDelta;
			
			Vector2D dtSpeed = (newSpeed * state->deltaT);
			Body* collided = willCollide(state, location + Vector2D(dtSpeed.x, dtSpeed.y)); // https://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
			if (collided != nullptr and collided != this) { // A body was collided with
				Vector2D n;
				Vector2D reflection;
				Vector2D relativeSpeed = newSpeed - collided->speed;
				n = location - collided->location;
				n = n * (1 / n.magnitude());
				reflection = relativeSpeed - n * 2 * newSpeed.dot(n);
				newSpeed = reflection;
			}
			speed = newSpeed;
			
		}
		break;
		default:
		break;
		}

		location = location + (speed * state->deltaT);
		// Wrap around
		if (location.x < -2000) {
			location.x = 2000;
		}
		if (location.x > 2000) {
			location.x = -2000;
		}
		if (location.y < -2000) {
			location.y = 2000;
		}
		if (location.y > 2000) {
			location.y = -2000;
		}
	}
};

class StaticGravBody : public Body {
public:
	StaticGravBody(Vector2D loc, double r, double m) {
		location = loc;
		radius = r;
		mass = m;
	}
};

class PlayerShip {
public:
	Vector2D location;
	Vector2D speed;
	Vector2D gravDelta;
	Vector2D playerDelta;
	bool brake = false;
	bool moving = false;
	bool parked = false;
	double moveSpeed = 1.0;
	Body* parkedOn = nullptr;
	Vector2D parkedDifference;
	Body* lastCollided = nullptr;
	void deltaSpeed(Vector2D accel) {
		playerDelta = accel;

	}
	void update(GameState* state) {
		if (moving) {
			parked = false;
		}
		if (!parked) {
			gravDelta = doGravity(state, location);
			Vector2D newSpeed = speed;
			newSpeed = newSpeed + gravDelta + playerDelta;
			if (newSpeed.x > 1000) {
				newSpeed.x = 1000;
			}
			if (newSpeed.x < -1000) {
				newSpeed.x = -1000;
			}
			if (newSpeed.y > 1000) {
				newSpeed.y = 1000;
			}
			if (newSpeed.y < -1000) {
				newSpeed.y = -1000;
			}

			// deltaT the new Speed for collision check.

			Vector2D dtSpeed = (newSpeed * state->deltaT);
			Body* collided = willCollide(state, location + Vector2D(dtSpeed.x, dtSpeed.y)); // https://www.sunshine2k.de/articles/coding/vectorreflection/vectorreflection.html
			if (collided != nullptr) { // A body was collided with
				Vector2D n;
				Vector2D reflection;
				Vector2D relativeSpeed = newSpeed - collided->speed;
				n = location - collided->location;
				n = n * (1 / n.magnitude());
				reflection = relativeSpeed - n * 2 * newSpeed.dot(n);
				newSpeed = reflection * 0.75; // slow the reflection speed as a friction;
				lastCollided = collided;

				if ((collided->location - location).magnitude() < collided->radius) { // resolve if inside 
					location = collided->location + (n * (1 / n.magnitude())) * collided->radius; // move along vector n, IE where they collison will be
					newSpeed =  newSpeed + collided->speed;
				}
			}

			// A body can be concidered parked if it is within a range and the speed of the player and body are comparable
			if (lastCollided != nullptr and !moving) {
				if ((location - lastCollided->location).magnitude() < lastCollided->radius + 10
					&& speed.x > lastCollided->speed.x - 100 && speed.x < lastCollided->speed.x + 100
					&& speed.y > lastCollided->speed.y - 100 && speed.y < lastCollided->speed.y + 100) {
					Vector2D diff = (lastCollided->location - location);
					parkedDifference = diff + (diff * (1 / diff.magnitude())) * 4; // A little is added to help with collison
					parkedOn = lastCollided;
					parked = true;
				}
				else {
					parked = false;
				}
			}

			if (!brake) {
				speed = newSpeed;
			}
		}

		if (brake and !parked) {
			// getting the closest body
			Body* closest = closestToPoint(state, location);

			Vector2D speedDiff = closest->speed - speed;
			if (closest != nullptr and speedDiff.magnitude() != 0) {
				speed = speed + speedDiff * (0.5 / speedDiff.magnitude());
			} else {
				speed = speed * 0.9999;
			}
		}

		if (parked) {
			speed = parkedOn->speed;
			location = (parkedOn->location - parkedDifference);
		}
		else {
			location = location + (speed * state->deltaT);
		}

		// Wrap around
		if (location.x < -2000) {
			location.x = 2000;
		}
		if (location.x > 2000) {
			location.x = -2000;
		}
		if (location.y < -2000) {
			location.y = 2000;
		}
		if (location.y > 2000) {
			location.y = -2000;
		}
	}

};

