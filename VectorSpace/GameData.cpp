#include "GameData.h"
#include <iostream>

double calcGravity(double mass, double distance) {
	double gConst = 2;
	double grav = (gConst * mass) / (distance * distance);
	return grav;
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
		if (locVec.magnitude() == 0) {
			continue;
		}
		if (locVec.magnitude() < body->radius) {
			return body;
		}
	}
	return nullptr;
}

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