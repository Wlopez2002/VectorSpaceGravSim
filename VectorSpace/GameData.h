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
class City;
class NavigationObject;
class PlayerShip;
class Entity;
class EntityCargo;
class EntityPirate;
class Projectile;

static const double GCONST = 2000.0; // Gravity constant
static const double AREASIZE = 8000; // the size of an area

// See GameData.cpp for descriptions.
double calcGravity(double mass, double distance);
Vector2D getOrbitSpeed(Body* toOrbit, Vector2D myLocation);
Vector2D doGravity(GameState* state, Vector2D location);
Body* willCollide(GameState* state, Vector2D location);
Body* closestToPoint(GameState* state, Vector2D location);
void generatePlaySpace(double systemRad, double systemPad, int seed, GameState* state);
void randSystemAt(Vector2D location, int seed, GameState* state, double systemRadius);
void resetGameState(GameState* state);
double randBodyOrbiting(Body* toOrbit, int seed, GameState* state, double distance, double maxRadius);

// taylor series approx of Sin and Cos derivative.
// These are included for ease of access when using function based acceleration for a DynamicGravBody.
static std::vector<double> taylorDSin = { 1.0, 0, -1.0 / 2, 0, 1.0 / 24, 0, -1.0 / 720, 0, 1.0 / 40320, 0, -1.0 / 3628800 };
static std::vector<double> tayloyDCos = { 0, -1.0, 0, 1.0 / 6, 0, -1.0 / 120, 0, 1.0 / 5040, 0, -1.0 / 362880 };

enum Stage {StageStart, StagePlay};

// This structure contains all data needed to run the game
struct GameState {
	Stage curState;
	int menuSelectorY;
	int seed;
	bool debugMode;
	float deltaT;
	PlayerShip* player;
	std::string seedStringBuffer;
	std::vector<StaticGravBody*> staticGravBodies;
	std::vector<DynamicGravBody*> dynamicGravBodies;
	std::vector<Entity*> entities;
	std::vector<City*> cities;
	std::vector<Projectile*> projectiles;
};

// The base parent class for physical bodies, represents planets, suns, etc.
class Body {
public:
	Vector2D speed = Vector2D(0, 0);
	Vector2D location = Vector2D(0, 0);
	double radius = 0;
	double mass = 0;
	int bodyID = -1;
	char bodyType; // p planet s star
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
			Body* collided = willCollide(state, location + Vector2D(dtSpeed.x, dtSpeed.y));
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

class City {
protected:
	float pcPS = 1; // produce or consume per second
	float storageLimit = 100;
	float currentStorage = 0;
	int cityID = -1;
	Body* tiedBody = nullptr;
public:
	City(float pcps, int sl, int id, Body* tb) {
		pcPS = pcps;
		storageLimit = sl;
		tiedBody = tb;
		cityID = id;
	}
	float getpcPS() { return pcPS; }
	float getStorageLimit() { return storageLimit; }
	float getCurStorage() { return currentStorage; }
	int getID() { return cityID; }
	Body* getTiedBody() { return tiedBody; }
	float take(float requested) { // returns how much was taken
		if (requested >= currentStorage) {
			int temp = currentStorage;
			currentStorage = 0;
			return temp;
		}
		currentStorage -= requested;
		return requested;
	}
	float give(float requested) { // returns the the remainder
		if (currentStorage + requested > storageLimit) {
			int remainder = storageLimit - (currentStorage + requested);
			currentStorage = storageLimit;
			return remainder;
		}
		currentStorage += requested;
		return 0;
	}
	void update(GameState* state) {
		currentStorage += pcPS * state->deltaT;
		if (currentStorage > storageLimit) {
			currentStorage = storageLimit;
		}
		if (currentStorage < 0) {
			currentStorage = 0;
		}
	}
};


// TODO: this class is being experimented with and is subject to frequent changes
// The class represting things that navigate the a world of bodies.
class NavigationObject {
protected:
	Vector2D location = Vector2D(0, 0);
	Vector2D destination = Vector2D(0, 0);
	Vector2D start = location;
	Vector2D currentDest = destination;
	Vector2D speed = Vector2D(0, 0);
	double impulseSpeed;
public:
	Body* closestBody = nullptr;
	Vector2D getLocation() { return location; }
	Vector2D getD() { return destination; }
	Vector2D getCD() { return currentDest; }
	void forceLocation(Vector2D newLoc) { location = newLoc; }
	void setDestination(Vector2D dest) { destination = dest; }
	void avoidBodies(GameState* state) {
		// Body Avoidance
		// draw a line from location to destination. if a body intersects the line add a vertex away from it as a temp destination
		// check to see if a body intersects by projecting the distance and seeing if that point is within the body radius
		/*
					Body
				   /  '
		dVect ->  /   '
				 /    ' <- C -> B (distB)
				/     '
			   /	  '
			  A ------------------ B <- line from location to destination (lineVect)
			  A ----- C <- projection of A -> Body onto A -> B (lineVecrProj)
		*/
		currentDest = destination;
		double closestDist = -1;
		closestBody = nullptr;
		
		Vector2D lineVect = destination - location;
		Vector2D closestCBody;
		bool closestBehind = false;
		for (auto body : state->staticGravBodies) {
			// if the destination is in a body, go there anyway
			if ((destination - body->location).magnitude() <= body->radius) {
				continue;
			}

			Vector2D dVect = body->location - location;
			Vector2D lineVecrProj;
			lineVecrProj = dVect.proj(lineVect);
			Vector2D C = lineVecrProj + location; // the location of C
			double distB = (body->location - C).magnitude();

			// This method will consider bodies that are "Behind the body" thus those need to be passed over
			// A body is in front if lineVecrProj and lineVect have the same normal (or the same signs for x and y)
			if (std::signbit(lineVecrProj.x) != std::signbit(lineVect.x) and std::signbit(lineVecrProj.y) != std::signbit(lineVect.y)){
				continue;
			}
			// We also should not concider bodies that are further than B
			//if (lineVecrProj.magnitude() > lineVect.magnitude()) {
			if (lineVecrProj.cmpMag(lineVect)){
				continue;
			}
			if (distB <= body->radius + 10) {
				if (closestDist == -1 or lineVecrProj.magnitude() < closestDist) {
					closestDist = lineVecrProj.magnitude();
					closestBody = body;
					closestCBody = (body->location - C);
					
				}
			}
		}
		for (auto body : state->dynamicGravBodies) { // The exact same code as above
			if ((destination - body->location).magnitude() <= body->radius) {
				continue;
			}

			Vector2D dVect = body->location - location;
			Vector2D lineVecrProj;
			lineVecrProj = dVect.proj(lineVect);
			Vector2D C = lineVecrProj + location;
			double distB = (body->location - C).magnitude();
			if (std::signbit(lineVecrProj.x) != std::signbit(lineVect.x) and std::signbit(lineVecrProj.y) != std::signbit(lineVect.y)) {
				continue;
			}
			if (lineVecrProj.cmpMag(lineVect)) {
				continue;
			}
			if (distB <= body->radius + 10) {
				if (closestDist == -1 or lineVecrProj.magnitude() < closestDist) {
					closestDist = lineVecrProj.magnitude();
					closestBody = body;
					closestCBody = (body->location - C);
				}
			}
		}
		if (closestBody != nullptr) {
			if (closestCBody == Vector2D(0, 0)) {
				Vector2D avoidVect = (closestBody->location - location).normalize();
				avoidVect = Vector2D(-avoidVect.y, avoidVect.x);
				avoidVect = avoidVect * -(closestBody->radius + 60);
				currentDest = closestBody->location + avoidVect;
			}
			else {
				currentDest = closestBody->location + (closestCBody.normalize() * -(closestBody->radius + 60));
			}
		}
	}
	void update(GameState* state) {
		/*
		* The current avoidance and speed system is not perfect, some collisions still happen
		* but I feel they are reasonable.
		*/
		Vector2D newSpeed = speed;
		Vector2D gravVect = (doGravity(state, location) * state->deltaT);
		newSpeed = newSpeed + gravVect;

		// Temporary testing code just to see the object move
		// get a random body to use for a new destination
		if ((destination - location).magnitude() < 18) {
			int randIndex = rand() % (state->staticGravBodies.size() - 1);
			StaticGravBody* bod = state->staticGravBodies.at(randIndex);
			Vector2D pVect = Vector2D(rand(), rand());
			pVect = pVect.normalize();
			pVect = pVect * (bod->radius + 100);
			pVect = pVect + bod->location;

			start = destination;
			destination = pVect;
		}

		// Run the avoid bodies function to get the current destination
		avoidBodies(state);

		impulseSpeed = 20;
		Vector2D locationAsIs = location + (newSpeed * state->deltaT);
		// if moving forward is worse thank brakeing
		Vector2D locationWithBrake = location + ((newSpeed * 0.9) * state->deltaT);
		if ((currentDest - locationAsIs).cmpMag(currentDest - locationWithBrake)) {
			newSpeed = newSpeed * 0.9;
		}

		// if the current speed gets close to currentDest do not add an impulse to speed
		Vector2D speedWithImpulse = newSpeed + ((currentDest - location).normalize() * impulseSpeed);
		Vector2D locationWithImpulse = location + (speedWithImpulse * state->deltaT);
		if ((currentDest - locationAsIs).cmpMag(currentDest - locationWithImpulse)) { // is the locationAsIs worse than locationWithImpulse
			newSpeed = speedWithImpulse;
			// The limit is to prevent zigzagging
			if (abs(newSpeed.x) < impulseSpeed / 4) {
				newSpeed.x = 0;
			}
			if (abs(newSpeed.y) < impulseSpeed / 4) {
				newSpeed.y = 0;
			}
		}

		Vector2D dtSpeed = (newSpeed * state->deltaT);
		Body* collided = willCollide(state, location + Vector2D(dtSpeed.x, dtSpeed.y));
		if (collided != nullptr) { // A body was collided with
			Vector2D n;
			Vector2D relativeSpeed = newSpeed - collided->speed;
			n = location - collided->location;
			n = n * (1 / n.magnitude());

			newSpeed = newSpeed * 0.75; // a little friction
			double impulse = relativeSpeed.dot(n) * -(1.5);
			newSpeed = newSpeed + (n * impulse);
		}

		// cap the new speed
		if (newSpeed.x > 800) {
			newSpeed.x = 800;
		}
		if (newSpeed.x < -800) {
			newSpeed.x = -800;
		}
		if (newSpeed.y > 800) {
			newSpeed.y = 800;
		}
		if (newSpeed.y < -800) {
			newSpeed.y = -800;
		}

		speed = newSpeed;

		// change location by speed
		location = location + (speed * state->deltaT);

		if (location.x < -AREASIZE) {
			location.x = -AREASIZE;
			speed.x = 0;
		}
		if (location.x > AREASIZE) {
			location.x = AREASIZE;
			speed.x = 0;
		}
		if (location.y < -AREASIZE) {
			location.y = -AREASIZE;
			speed.y = 0;
		}
		if (location.y > AREASIZE) {
			location.y = AREASIZE;
			speed.y = 0;
		}
	}
};

// The class representing the player and their ship.
class PlayerShip {
private:
	int health = 10;
	int thrustDir = 0;
	Vector2D location = Vector2D(0, 0);
	Vector2D speed = Vector2D(0, 0);
	Vector2D gravDelta = Vector2D(0, 0);
	Vector2D playerDelta = Vector2D(0, 0);
	bool brake = false;
	bool moving = false;
	bool parked = false;
	double hitImmunity = 0;
	double thrust = 1000.0;
	Body* parkedOn = nullptr;
	Vector2D parkedDifference;
	Body* lastCollided = nullptr;
public:
	int damage(int dam, GameState* state) {
		health -= dam;

		if (health <= 0) {
			resetGameState(state);
		}
		return health;
	}
	int getHealth() { return health; }
	Vector2D getSpeed() { return speed; }
	Vector2D getLocation() { return location; }
	Vector2D getGravDelta() { return gravDelta; }
	Vector2D getPlayerDelta() { return playerDelta; }
	bool isMoving() { return moving; }
	bool isParked() { return parked; }
	double getThrust() { return thrust; }
	void setThrustDir(int thr) { thrustDir = thr; }
	void doBrake() { brake = true; }
	void unbrake() { brake = false; }
	void setHealth(int h) { health = h; }
	void forceLocation(Vector2D newLoc) { location = newLoc; }
	void resetPlayer() {
		setHealth(10);
		forceLocation(Vector2D(0, 0));
		speed = Vector2D(0, 0);
		thrust = 1000.0;
	}
	void incrementThrust(double incr) {
		thrust += incr;
		if (thrust > 4000) {
			thrust = 4000;
		}
		if (thrust < 0) {
			thrust = 0;
		}
	}
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

		incrementThrust(thrustDir * 500 * state->deltaT);

		if (moving) {
			parked = false;
		}
		if (!parked) {
			gravDelta = doGravity(state, location);
			Vector2D newSpeed = speed;
			if (!brake) {
				newSpeed = newSpeed + (playerDelta * state->deltaT);
			}
			newSpeed = newSpeed + (gravDelta * state->deltaT);

			// decrements if the player is hit immune
			if (hitImmunity > 0.0) {
				hitImmunity -= state->deltaT;
			}

			// deltaT the new Speed for collision check.
			Vector2D dtSpeed = (newSpeed * state->deltaT);
			Body* collided = willCollide(state, location + Vector2D(dtSpeed.x, dtSpeed.y));
			if (collided != nullptr) { // A body was collided with
				Vector2D n;
				Vector2D relativeSpeed = newSpeed - collided->speed;
				n = location - collided->location;
				n = n * (1 / n.magnitude());

				newSpeed = newSpeed * 0.75; // a little friction
				double impulse = relativeSpeed.dot(n) * -(1.5);
				newSpeed = newSpeed + (n * impulse);
				lastCollided = collided;

				// Check if the player is to be damaged from the impact;
				if (!(hitImmunity > 0.0) and relativeSpeed.magnitude() > 400) {
					damage((int)newSpeed.magnitude() / 300, state);
					hitImmunity = 1;
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

			// cap the new speed
			if (newSpeed.x > 10000) {
				newSpeed.x = 10000;
			}
			if (newSpeed.x < -10000) {
				newSpeed.x = -10000;
			}
			if (newSpeed.y > 10000) {
				newSpeed.y = 10000;
			}
			if (newSpeed.y < -10000) {
				newSpeed.y = -10000;
			}

			speed = newSpeed;
		}

		// if the player is braking and not parked.
		if (brake and !parked) {
			// getting the closest body to match speed.
			Body* closest = closestToPoint(state, location);
			Vector2D speedDiff = closest->speed - speed;
			if (closest != nullptr and speedDiff.magnitude() != 0) {
				speed = speed + (speedDiff * (1 / speedDiff.magnitude())) * thrust * state->deltaT;
			}
			else {
				speedDiff = speed * -1;
				speed = speed + (speedDiff * (1 / speedDiff.magnitude())) * thrust * state->deltaT;
			}
		}

		// if the player is parked match their speed with what they are parked on.
		// Otherwise let the player move as normal
		if (parked) {
			speed = parkedOn->speed;
			location = (parkedOn->location - parkedDifference);
		}
		else {
			location = location + (speed * state->deltaT);
		}

		// Stop the player of they would go over the AREASIZE
		if (location.x < -AREASIZE) {
			location.x = -AREASIZE;
			speed.x = 0;
		}
		if (location.x > AREASIZE) {
			location.x = AREASIZE;
			speed.x = 0;
		}
		if (location.y < -AREASIZE) {
			location.y = -AREASIZE;
			speed.y = 0;
		}
		if (location.y > AREASIZE) {
			location.y = AREASIZE;
			speed.y = 0;
		}
	}

};

// An entity is much like a player
// much of it's movement is handled by a Navigation Object.
class Entity {
protected:
	char faction = 'n'; // no faction
	char entityType;
	int health = 10;
	NavigationObject navHandler;
public:
	Entity() {
		entityType = 'b';
	}
	Entity(char type) {
		entityType = type;
	}
	char getFaction() { return faction; }
	char getType() { return entityType; }
	int damage(int dam, GameState* state) {
		health -= dam;

		if (health <= 0) {
			std::cout << "killed\n";
		}
		return health;
	}
	Vector2D getLocation() { return navHandler.getLocation(); }
	NavigationObject* getNav() { return &navHandler; }
	int getHealth() { return health; }
	void setHealth(int h) { health = h; }
	void update(GameState* state) {
		navHandler.update(state);
	}
};

// An entity that brings supplies from producer to consumer cities;
// Should this project ever introduce parallelization there will likely be issues here.
class EntityCargo: Entity {
protected:
	int cargoCount  = 0;
	int cargoCap = 10;
	City* destCity = nullptr;
public:
	EntityCargo() {
		entityType = 'c';
	}

	void update(GameState* state) {
;		if (destCity == nullptr) {
			if (cargoCount >= cargoCap) {
				destCity = getBestConsumer(state);
			}
			else {
				destCity = getBestProducer(state);
			}
		} else if ((navHandler.getLocation() - destCity->getTiedBody()->location).magnitude() <= destCity->getTiedBody()->radius + 100) { // take or supply the city if close by
			if (destCity->getpcPS() > 0) {
				cargoCount = destCity->take(cargoCap);
				destCity = getBestConsumer(state);
			}
			else {
				cargoCount = destCity->give(cargoCap);
				destCity = getBestProducer(state);
			}
		}
		
		
		if (destCity != nullptr) {
			Vector2D destLoc = destCity->getTiedBody()->location;
			navHandler.setDestination(destLoc);
		}

		navHandler.update(state);
	}
	// checks if a city has the supply to fill to cargoCap
	City* getBestProducer(GameState* state) {
		City* closest = nullptr;
		float closestDis = -1;
		for (City* city : state->cities) {
			if (city->getpcPS() > 0 and city->getCurStorage() > 0) { // only producers
				float distance = (navHandler.getLocation() - city->getTiedBody()->location).magnitude();
				if (city->getCurStorage() > cargoCap) { // A trip is only worth it if the city can fill the cargo hold
					if ((closest == nullptr) or (distance < closestDis)) {
						closest = city;
						closestDis = distance;
					}
				}
			}
		}
		return closest;
	}
	City* getBestConsumer(GameState* state) {
		City* closest = nullptr;
		float closestDis = -1;
		for (City* city : state->cities) {
			if (city->getpcPS() <= 0 and city->getCurStorage() < city->getStorageLimit()) { // only producers
				float distance = (navHandler.getLocation() - city->getTiedBody()->location).magnitude();
				if ((closest == nullptr) or (distance < closestDis)) {
					closest = city;
					closestDis = distance;
				}
			}
		}
		return closest;
	}
};

class EntityPirate : Entity {
protected:
	enum AIBehavior;
	enum AIBAim;
	AIBehavior currentBehavior;
public:
	enum AIBehavior { Reckless, Cautious, Driveby };
	enum AIBAim { PoorAim, ExactAim, LeadingAim };
	EntityPirate(AIBehavior behavior) {
		currentBehavior = behavior;
		entityType = 'p';
		faction = 'e';
	}
	void update(GameState* state) {
		float dfp = (navHandler.getLocation() - state->player->getLocation()).magnitude();
		Vector2D pte = (state->player->getLocation() - navHandler.getLocation()).normalize(); // player to entity
		switch (currentBehavior) {
		case Reckless:
			if (dfp > 100) {
				navHandler.setDestination(state->player->getLocation() + pte * 100);
			}
			break;
		case Cautious:
			navHandler.setDestination(state->player->getLocation() - pte * 300);
			break;
		case Driveby:
			Vector2D s = state->player->getLocation() + rotateVector2D(pte * 300, 3.1415/2);
			navHandler.setDestination(s);
			break;
		}
		navHandler.update(state);
	}
};

class Projectile {
protected:
	Vector2D location;
	Vector2D direction;
	float hitRange;
	float grace = 0.0;
	float timeLimit = 10.0;
	bool cullMe = false;
public:
	Projectile(Vector2D location, Vector2D direction, float hitRange, float grace) {
		this->location = location; this->direction = direction; this->hitRange = hitRange;
		this->grace = grace;
	}
	bool isCull() { return cullMe; }
	Vector2D getLocation() { return location; }
	// 0 = nothing, 1 = hit, 2 = is in cull distance
	int update(GameState* state) {
		location = location + (direction * state->deltaT);
		timeLimit -= state->deltaT;

		if (grace) { // if the projectile is in grace it cannot hit the player or an entity
			grace -= state->deltaT;
			std::cout << grace << "\n";
			if (grace < 0) {
				grace = 0;
			}
		}
		else {
			if ((state->player->getLocation() - location).magnitude() <= hitRange) {
				hitPlayer(state);
			}
			for (auto entity : state->entities) {
				if ((location - entity->getLocation()).magnitude() <= hitRange) {
					hitEntity(state, entity);
				}
			}
		}
		for (auto body : state->staticGravBodies) {
			if ((body->location - location).magnitude() - body->radius <= hitRange) {
				hitBody(state);
			}
		}
		for (auto body : state->dynamicGravBodies) {
			if ((body->location - location).magnitude() - body->radius <= hitRange) {
				hitBody(state);
			}
		}


		if (timeLimit < 0) {
			cullMe = true;
			return 2;
		}	
	}

	void hitBody(GameState* state) {
		cullMe = true;
	}
	void hitEntity(GameState* state, Entity* entity) {
		cullMe = true;
	}
	void hitPlayer(GameState* state) {
		cullMe = true;
	}
};