#ifndef PHYSICS_H
#define PHYSICS_H

#include "raylib/src/raylib.h"

typedef enum {
	RIGID,     // rigid bodies prevent overlapping
	PHANTOM    // phantom bodies allow overlapping
} BodyType;

typedef struct Box {
	Vector3 minSize;
	Vector3 maxSize;
} Box;

/* SATBox is the structure holding all the data needed by SAT for a box
 * 'v' is the array containing the 8 vertices of the box
 * 'n' is the array containing the 3 axis defined by the normals of the box
 * 's' is the array containing the 3 sides defined by the sides of the box */
typedef struct SATBox {
	Vector3 v[8];
	Vector3 n[3];
	Vector3 s[3];
} SATBox;

typedef struct RigidBody {
	BodyType type;
	Vector3 pos;
	Box box;
	union {
		SATBox *satBox;
		Mesh *mesh;
	};
	int meshCount;
	/* I am using a pointer function so that the user may define it's own collision handler */
	int (*collisionHandler)(struct RigidBody *a, struct RigidBody *b); 
} RigidBody;

/* Creates a pointer to a rigid body using the position 'x' and 'y' (top left) and the size 'width' and
 * 'height' to determine the rect for collision handling in AABB */
RigidBody *createRigidBody(BodyType type, Vector3 position, Vector3 size);

/* Creates a pointer to a rigid body using the array of meshes 'mesh' to determine
 * the box for collision handling in AABB */
RigidBody *createRigidBodyFromMesh(BodyType type, Mesh *meshes, int meshCount, Vector3 position);

/* Checks for a collision between the two bodies 'a' and 'b' using AABB. It returns 0 if there are no
 * collisions, 1 if there is a collision */
int checkCollisionAABB(RigidBody *a, RigidBody *b);

/* Creates a SATBox pointer defining the vertices using the box 'b' and its position 'pos',  * calculating the separating axis to check for SAT  */
SATBox *createSATBox(Box *b, Vector3 pos);

/* Checks for collision between two rigid bodies 'a' and 'b', checking for the type of 
 * object and using the corresponding collision funciton */
int checkCollision(RigidBody *a, RigidBody *b);

/* Checks for collision between a box 'b' and a complex shape 'm' using SAT */
int collisionSATBoxAndComplexShape(RigidBody *a, RigidBody *b);

/* Checks if a box 'bf' overlaps the triangle 'v1', 'v2', 'v3' on axis n using
 * the dot product (axis projection) */
int checkOverlappingBoxAndTriangleOnAxis(SATBox *sb, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 n);

/* Frees the memory allocated to a rigid body 'r' */
void freeRigidBody(RigidBody *r);

/* Calculates the Vector product between 'v1' and 'v2' */
Vector3 vectorProduct(Vector3 v1, Vector3 v2);

/* Calculates the Vector product between 'v1' and 'v2' normalized */
Vector3 vectorProductNormalized(Vector3 v1, Vector3 v2);

/* Calculates the dot product between 'v1' and 'v2' */
float dotProduct(Vector3 v1, Vector3 v2);
#endif
