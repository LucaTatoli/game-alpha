#ifndef PHYSICS_H
#define PHYSICS_H

#include "raylib/src/raylib.h"
#include <math.h>

/* =============== Structs =============== */

typedef enum {
	RIGID,		 // rigid bodies prevent overlapping and move after collision resolution (e.g. the player)
	RIGID_FIXED, // rigid fixed bodies prevent overlapping, but do not move after collision resolution (e.g. a wall)
	PHANTOM      // phantom bodies allow overlapping (e.g. a trigger)
} BodyType;

/* Box is the structure holding all the data needed by SAT for a box
 * 'v'  is the array containing the 8 vertices of the box
 * 'vw' is the array containing the 8 vertices of the box translated in the world position
 * 'n'  is the array containing the 3 axis defined by the normals of the box
 * 'center' is the center of the box
 * 'wCetner' is the center of the box in world position
 * sides are not stored because for SAT purposes they're parallel to normals */ 
typedef struct Box {
	Vector3 v[8];
	Vector3 vw[8];
	Vector3 n[3];
	Vector3 center;
	Vector3 wCenter;
} Box;

typedef struct RigidBody {
	BodyType type;
	Vector3 pos;
	Vector3 vel;
	Box box;
	Mesh *mesh;
	int meshCount;
	int grounded;
	/* This function pointer is the callback that will be called after a collision is detected
	 * it returns the amount of displacement that the rigid body had been moved by after
	 * the collision resolution */
	Vector3 (*callback)(struct RigidBody *a, struct RigidBody *b); 
} RigidBody;

typedef struct CollisionInfo {
	float baseLength;
	Vector3 baseDirection;
	float length;
	Vector3 direction;
	Vector3 v1;
	Vector3 v2;
	Vector3 v3;
} CollisionInfo;

typedef struct World {
	float gravity;
	int bodyCount;
	int maxBodies;
	RigidBody *bodies[1000];
} World;

/* =============== Constants =============== */

static constexpr Vector3 upVector = (Vector3) { 0, 1, 0 };
static constexpr float maxSlope = cos(3.141592654 / 3);
static World world = {
	.gravity = 2,
	.bodyCount = 0,
	.maxBodies = 1000
};

/* =============== Structs Manipulation Functions =============== */

/* Creates a pointer to a rigid body using the position 'x' and 'y' (top left) and the size 'width' and
 * 'height' to determine the rect for collision handling in AABB */
RigidBody *createRigidBody(BodyType type, Vector3 position, Vector3 size);

/* Creates a pointer to a rigid body using the array of meshes 'mesh' to determine
 * the box for collision handling in AABB */
RigidBody *createRigidBodyFromMesh(BodyType type, Mesh *meshes, int meshCount, Vector3 position);

/* Creates a Box pointer defining the vertices using the minSize and maxSize vector3
 * and calculating the separating axis to check for SAT  */
Box createBox(Vector3 minSize, Vector3 maxSize, Vector3 position);

/* Frees the memory allocated to a rigid body 'r' */
void freeRigidBody(RigidBody *r);

/* Updates the rigid body position and computes the new box vertices world position */
void updateRigidBodyPosition(RigidBody *r, Vector3 pos);

/* =============== Check Collision Functions =============== */

/* Checks for a collision between the two bodies 'a' and 'b' using AABB. It returns 0 if there are no
 * collisions, 1 if there is a collision */
CollisionInfo checkCollisionAABB(RigidBody *a, RigidBody *b);

/* Checks for collision between two rigid bodies 'a' and 'b', checking for the type of 
 * object and using the corresponding collision funciton */
CollisionInfo checkCollision(RigidBody *a, RigidBody *b);

/* Checks for collision between a box 'b' and a complex shape 'm' using SAT */
CollisionInfo collisionSATBoxAndComplexShape(RigidBody *a, RigidBody *b);

/* Checks if a box 'bf' overlaps the triangle 'v1', 'v2', 'v3' on axis n using
 * the dot product (axis projection) and return the length of ther intersection */
float checkOverlappingBoxAndTriangleOnAxis(Box *b, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 n);

/* Checks if the base of the  box 'bf' overlaps the triangle 'v1', 'v2', 'v3' on axis n
 * using the dot product (axis projection) and return the length of ther intersection */
float checkOverlappingBoxBaseAndTriangleOnAxis(Box *b, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 n);

/* =============== Collision Resolution Functions =============== */

/* Handles the collision between the two bodies 'a' and 'b' using 'i' as info
 * to handle the collision correctly */
void handleCollision(RigidBody *a, RigidBody *b, CollisionInfo i, float frameTime);

/* =============== State Update Functions =============== */

void updateWorld(float frameTime);

/* =============== Vector Utility Functions =============== */

/* Calculates the Vector product between 'v1' and 'v2' */
Vector3 vectorProduct(Vector3 v1, Vector3 v2);

/* Calculates the Vector product between 'v1' and 'v2' normalized */
Vector3 vectorProductNormalized(Vector3 v1, Vector3 v2);

/* Calculates the dot product between 'v1' and 'v2' */
float dotProduct(Vector3 v1, Vector3 v2);

/* Calculates the distance between to vectors */
float vectorDistance(Vector3 v1, Vector3 v2);

/* =============== Other Utility Functions =============== */

/* Maps 'value' between 'min' and 'max' to a new value between 'nMin' and 'nMax' */
float map(float value, float min, float max, float nMin, float nMax);

#endif
