/*
* This is the primary file for handling the games data and calculations.
*/

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

static const double GCONST = 2.0; // Gravity constant
static const double AREASIZE = 4000; // the size of an area

// See GameData.cpp for descriptions.
double calcGravity(double mass, double distance);
Vector2D getOrbitSpeed(Body* toOrbit, Vector2D myLocation);
Vector2D doGravity(GameState* state, Vector2D location);
Body* willCollide(GameState* state, Vector2D location);
Body* closestToPoint(GameState* state, Vector2D location);
void generatePlaySpace(double systemRad, double systemPad, int seed, GameState* state);
void randSystemAt(Vector2D location, int seed, GameState* state, double systemRadius);
double randBodyOrbiting(Body* toOrbit, int seed, GameState* state, double distance, double maxRadius);

// taylor series approx of Sin and Cos derivative.
// These are included for ease of access when using function based acceleration for a DynamicGravBody.
static std::vector<double> taylorDSin = { 1.0, 0, -1.0 / 2, 0, 1.0 / 24, 0, -1.0 / 720, 0, 1.0 / 40320, 0, -1.0 / 3628800 };
static std::vector<double> tayloyDCos = { 0, -1.0, 0, 1.0 / 6, 0, -1.0 / 120, 0, 1.0 / 5040, 0, -1.0 / 362880 };

// This structure contains all data needed to run the game
struct GameState {
	double deltaT;
	PlayerShip* player;
	std::vector<StaticGravBody*> staticGravBodies;
	std::vector<DynamicGravBody*> dynamicGravBodies;
};

// The base parent class for physical bodies, represents planets, suns, etc.
class Body {
public:
	Vector2D speed = Vector2D(0, 0);
	Vector2D location;
	double radius;
	double mass;
	int bodyID;
};

// This class is for bodies that need to move.
// moveType affecrts how speed is changed over time.
// 0 for using the set speed, 
// 1 sets the speed to maintain an eliptical orbit about a point (with cos and sin).
// 2 for a following function vectors to set speed,
// 3 for set speed impacted by gravity (with collision).
class DynamicGravBody : public Body {
public:
	Vector2D gravDelta;

	// Each of these values impact each moveType in a different way.
	// look close at the update function to see how they affect the body.
	double timetart = 0;
	double timeEnd = 999999;
	double timeCur = timetart;
	float deltaMul = 1; float XMul = 1; float YMul = 1;

	char moveType = 0; 

	Body* orbitBody = nullptr; // the body to orbit if using move type 1.

	// These follow a c0 + c1*x + c2*x^2 + ... + c3*x^i format where c is the value at the index of the vector,
	// they are good for taylor series. Used for move type 2.
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
		// iterate the current time.
		timeCur += state->deltaT * deltaMul;
		if (timeCur > timeEnd) {
			timeCur = timetart;
		}

		// moveType dictates how speed is changed.
		switch (moveType)
		{
		case 0: 
			// Based on whatever the speed already is.
			break;
		case 1:
		{
			// rigid eliptical orbit about orbitPoint.
			// find where we want to be then set the speed needed to reach it,
			// this helps with collision calculations.
			if (orbitBody != nullptr) {
				Vector2D pastLocation = location;
				Vector2D newLocation = Vector2D(cos(timeCur) * XMul, sin(timeCur) * YMul);
				newLocation = newLocation + orbitBody->location;

				speed = newLocation - pastLocation;
				speed = speed * (1 / state->deltaT); // weight the speed to negate the later multiplication.
			}
		}
		break;
		case 2:
		{
			// uses function vectors as a polynomial to calcuate what the speed needs to be at some time.
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
			// Movement with gravity, this is the only body movement option that uses collision.
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

		// Wrap around if an edge is reached.
		if (location.x < -AREASIZE) {
			location.x = AREASIZE;
		}
		if (location.x > AREASIZE) {
			location.x = -AREASIZE;
		}
		if (location.y < -AREASIZE) {
			location.y = AREASIZE;
		}
		if (location.y > AREASIZE) {
			location.y = -AREASIZE;
		}
	}
};

// The class for a static bodies that do not move.
class StaticGravBody : public Body {
public:
	StaticGravBody(Vector2D loc, double r, double m) {
		location = loc;
		radius = r;
		mass = m;
	}
};


// The class representing the player and their ship.
class PlayerShip {
private:
	int health = 10;
	Vector2D location;
	Vector2D speed;
	Vector2D gravDelta;
	Vector2D playerDelta;
	bool brake = false;
	bool moving = false;
	bool parked = false;
	double hitImmunity = 0;
	double thrust = 1.0;
	Body* parkedOn = nullptr;
	Vector2D parkedDifference;
	Body* lastCollided = nullptr;
public:
	int damage(int dam) {
		health -= dam;
		return health;
	}
	int getHealth() { return health;}
	Vector2D getSpeed() { return speed; }
	Vector2D getLocation() { return location; }
	Vector2D getGravDelta() { return gravDelta; }
	Vector2D getPlayerDelta() { return playerDelta; }
	bool isMoving() { return moving; }
	bool isParked() { return parked; }
	double getThrust() { return thrust; }
	void incrementThrust(double incr) { thrust += incr; }
	void doBrake() { brake = true; }
	void unbrake() { brake = false; }
	void forceLocation(Vector2D newLoc) { location = newLoc; }
	void deltaSpeed(Vector2D accel) {
		if (accel == Vector2D(0, 0)) {
			moving = false;
		}
		else {
			moving = true;
		}
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

			// cap the new speed
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

			// decrements if the player is hit immune
			if (hitImmunity > 0.0) {
				hitImmunity -= state->deltaT;
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

				// Check if the player is to be damaged from the impact;
				if (!(hitImmunity > 0.0) and relativeSpeed.magnitude() > 400) {
					damage(relativeSpeed.magnitude()/300);
					hitImmunity = 1;
				}

				if ((collided->location - location).magnitude() < collided->radius) { // resolve if inside a body.
					location = collided->location + (n * (1 / n.magnitude())) * (collided->radius + 10); // move along vector n, IE where they collison would have been.
					newSpeed =  newSpeed + collided->speed;
				}
			}

			// A body can be considered parked if it is within a range and the speed of the player and body are close.
			if (lastCollided != nullptr and !moving) {
				if ((location - lastCollided->location).magnitude() < lastCollided->radius + 10
					&& speed.x > lastCollided->speed.x - 100 && speed.x < lastCollided->speed.x + 100
					&& speed.y > lastCollided->speed.y - 100 && speed.y < lastCollided->speed.y + 100) {
					Vector2D diff = (lastCollided->location - location);
					parkedDifference = diff + (diff * (1 / diff.magnitude())) * 4; // the * 4 is added to help with collison.
					parkedOn = lastCollided;
					parked = true;
				}
				else {
					parked = false;
				}
			}

			// If the player is not braking, then the speed can be updated to newSpeed;
			if (!brake) {
				speed = newSpeed;
			}
		}

		// if the player is braking and not parked.
		if (brake and !parked) {
			// getting the closest body to match speed.
			Body* closest = closestToPoint(state, location);

			Vector2D speedDiff = closest->speed - speed;
			if (closest != nullptr and speedDiff.magnitude() != 0) {
				speed = speed + speedDiff * (0.5 / speedDiff.magnitude());
			} else {
				speed = speed * 0.9999;
			}
		}

		// if the player is parked match their speed with what they are parked on.
		if (parked) {
			speed = parkedOn->speed;
			location = (parkedOn->location - parkedDifference);
		}
		else {
			location = location + (speed * state->deltaT);
		}

		// Wrap around
		if (location.x < -AREASIZE) {
			location.x = AREASIZE;
		}
		if (location.x > AREASIZE) {
			location.x = -AREASIZE;
		}
		if (location.y < -AREASIZE) {
			location.y = AREASIZE;
		}
		if (location.y > AREASIZE) {
			location.y = -AREASIZE;
		}
	}

};

