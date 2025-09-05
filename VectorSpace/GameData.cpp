#include "GameData.h"
#include <iostream>

double calcGravity(double mass, double distance) {
	double grav = (GCONST * mass) / (distance * distance);
	return grav;
}

// TODO: This doesn't work. It should not need the * 2300
Vector2D getOrbitSpeed(Body* toOrbit, Vector2D myLocation) {
	Vector2D n = myLocation - toOrbit->location;
	double velocity = sqrt((GCONST * toOrbit->mass * 2300) / n.magnitude());
	n = Vector2D(-1 * n.y, n.x); // clockwise vector rotation;
	n = n * (1 / n.magnitude());
	n = n * velocity;
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
	srand(seed);
	int curRad; int curWeightMod;

	// the core of a solar system
	curRad = rand() % (250 - 100) + 100;
	curWeightMod = rand() % (250 - 10) + 10;

	StaticGravBody* core = new StaticGravBody(location, curRad, curRad * curWeightMod);
	core->bodyID = state->staticGravBodies.size();
	state->staticGravBodies.push_back(core);

	double usedRadius = core->radius + 10;
	int maxPlanets = rand() % 10;
	double spacePerPlanet = (systemRadius - usedRadius) / maxPlanets;
	double maxRad = spacePerPlanet / 2;
	if (maxRad > core->radius) {
		maxRad = core->radius;
	}
	for (int i = 0; i < maxPlanets; i++) {
		usedRadius = randBodyOrbiting(core, seed -= 20, state, usedRadius + (spacePerPlanet/2), maxRad);
	}
	return;
}

// creates a random dynamic body orbiting another
// returns the working radius
double randBodyOrbiting(Body* toOrbit, int seed, GameState* state, double distance, double maxRadius){
	srand(seed);
	double spentDistance;
	int curRad; int curWeightMod;

	curRad = rand() % (int) (maxRadius - 10) + 10;
	curWeightMod = rand() % (25 - 5) + 5;
	float randomTimeComp = (float)(rand() % (10-1) + 1) / 10;

	DynamicGravBody* bod = new DynamicGravBody(Vector2D(toOrbit->location.x + distance, 0), curRad, curRad * curWeightMod, 1, -3.1415, 3.1415, randomTimeComp, distance, distance);
	bod->orbitBody = toOrbit;
	bod->bodyID = state->dynamicGravBodies.size();
	state->dynamicGravBodies.push_back(bod);

	spentDistance = distance + (2 * bod->radius);
	return spentDistance;
}