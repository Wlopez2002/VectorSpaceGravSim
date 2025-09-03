#include "GameData.h"
#include <iostream>

double calcGravity(double mass, double distance) {
	double grav = (GCONST * mass) / (distance * distance);
	return grav;
}

// TODO: This doesn't work. It should not need the * 2300
Vector2D getOrbitSpeed(Body* toOrbit, Vector2D myLocation) {
	Vector2D n = myLocation - toOrbit->location;
	std::cout << n.magnitude() << "\n";
	double velocity = sqrt((GCONST * toOrbit->mass * 2300) / n.magnitude());
	std::cout << velocity << "\n";
	n = Vector2D(-1 * n.y, n.x); // clockwise vector rotation;
	n = n * (1 / n.magnitude());
	n = n * velocity;
	std::cout << n.toString() << "\n";
	return n;
}

Vector2D doGravity(GameState* state, Vector2D location) {
	std::vector<StaticGravBody*> staticGravBodies = state->staticGravBodies;
	std::vector<DynamicGravBody*> dynamicGravBodies = state->dynamicGravBodies;
	Vector2D deltaVec;

	for (auto body : staticGravBodies) {
		Vector2D locVec = body->location - location;

		double grav = calcGravity(body->mass, locVec.magnitude());

		// normalize vector
		locVec = locVec * (1 / locVec.magnitude());
		 
		deltaVec = deltaVec + locVec * grav;
	}  
	for (auto body : dynamicGravBodies) {
		Vector2D locVec = body->location - location;
		if (locVec.magnitude() == 0) {
			continue;
		}

		double grav = calcGravity(body->mass, locVec.magnitude());

		if (locVec.x == 0) { // Sets vector direction
			locVec.x = 0;
		}
		else {
			locVec.x = locVec.x / abs(locVec.x);
		}
		if (locVec.y == 0) {
			locVec.x = 0;
		}
		else {
			locVec.y = locVec.y / abs(locVec.y);
		}

		deltaVec = deltaVec + locVec * grav;
	}
	
	return deltaVec;
}


// checks if a location is in a body
Body* willCollide(GameState* state, Vector2D location) {
	std::vector<StaticGravBody*> staticGravBodies = state->staticGravBodies;
	std::vector<DynamicGravBody*> dynamicGravBodies = state->dynamicGravBodies;
	for (auto body : staticGravBodies) {
		Vector2D locVec = body->location - location;
		if (locVec.magnitude() < body->radius) {
			return body;
		}
	}
	for (auto body : dynamicGravBodies) {
		Vector2D locVec = body->location - location;
		if (locVec.magnitude() != 0) {
			if (locVec.magnitude() < body->radius) {
				return body;
			}
		}
	}
	return nullptr;
}

// gets the closest body to a location
Body* closestToPoint(GameState* state, Vector2D location) {
	Body* toReturn = nullptr;
	double curLowMag = -1;
	std::vector<StaticGravBody*> staticGravBodies = state->staticGravBodies;
	std::vector<DynamicGravBody*> dynamicGravBodies = state->dynamicGravBodies;

	for (auto body : staticGravBodies) {
		// get distance from the bodies surface
		double locVec = (body->location - location).magnitude() - body->radius;

		if (curLowMag == -1 or locVec < curLowMag) {
			toReturn = body;
			curLowMag = locVec;
		}
	}
	for (auto body : dynamicGravBodies) {
		double locVec = (body->location - location).magnitude() - body->radius;
		if (curLowMag == -1 or locVec < curLowMag) {
			toReturn = body;
			curLowMag = locVec;
		}
	}

	return toReturn;
}

// Creates a random solar system at a location
void randSystemAt(Vector2D location, int seed, GameState* state, double systemRadius){
	double remainingRadius = systemRadius;
	srand(seed);
	int max; int min; int curRad; int curWeight;

	// the core of a solar system
	min = 100; max = 500;
	curRad = rand() % (max - min) + max;

	StaticGravBody* core = new StaticGravBody(location, 200, 10000);
	core->bodyID = 1;
	state->staticGravBodies.push_back(core);
}

// creates a random dynamic body ordering another
void randBodyOrbiting(Body* toOrbit, int distance, int seed, GameState* state, double radius, int moons) {
	srand(seed);
	int max = radius; int min = 10; int curRad; int curWeight;
	int moonspace = radius / 4; // save a least 1/4 of the radius for the moons
	if (moons) {
		max = radius - moonspace;
		curRad = rand() % (max - min) + max;
	}
	else {
		curRad = rand() % (max - min) + max;
	}
	moonspace = radius - curRad - 2;  // moon space is now the space left with a little padding

	curWeight = (rand() % 400 - 50) + 400;
	DynamicGravBody* core = new DynamicGravBody(toOrbit->location + Vector2D(radius + toOrbit->radius, 0),
		curRad, curWeight, 1, -3.1415, 3.1415, 1, radius + toOrbit->radius + distance, radius + toOrbit->radius + distance);
	core->bodyID = state->dynamicGravBodies.size() + 1;
	core->orbitBody = toOrbit;

	//for (int i = 0; i < moons; i++) {
	//	curRad = rand() % (max - min) + max;
	//	curWeight = (rand() % 400 - 50) + 400;
	//	DynamicGravBody* newMoon = new DynamicGravBody(toOrbit->location + Vector2D(radius + toOrbit->radius, 0),
	//		curRad, curWeight, 1, -3.1415, 3.1415, 1, radius + toOrbit->radius + distance, radius + toOrbit->radius + distance);
	//}

	state->dynamicGravBodies.push_back(core);
}