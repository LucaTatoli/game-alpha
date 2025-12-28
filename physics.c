#include "physics.h"
#include "utils.h"
#include "raylib/src/raymath.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* =============== Structs Manipulation Functions ===============  */

RigidBody *createRigidBody(BodyType type, Vector3 position, Vector3 size) {
	RigidBody *r = xmalloc(sizeof(*r));

	r->type = type;
	r->pos = position;
	Vector3 minSize = (Vector3) { -size.x / 2, -size.y / 2, -size.z / 2 };
	Vector3 maxSize = (Vector3) {  size.x / 2,  size.y / 2,  size.z / 2 };
	r->box = createBox(minSize, maxSize, position);

	return r;
}

RigidBody *createRigidBodyFromMesh(BodyType type, Mesh *mesh, int meshCount, Vector3 position) {
	RigidBody *r = xmalloc(sizeof(*r));

	r->type = type;
	r->mesh = mesh;
	r->meshCount = meshCount;
	r->pos = position;

	float minX =  FLT_MAX, minZ =  FLT_MAX, minY =  FLT_MAX;
	float maxX = -FLT_MAX, maxZ = -FLT_MAX, maxY = -FLT_MAX;
	for(int j = 0; j < meshCount; j++) {
		for(int i = 0; i < mesh[j].vertexCount; i++) {
			float x = mesh[j].vertices[i * 3    ];
			float y = mesh[j].vertices[i * 3 + 1];
			float z = mesh[j].vertices[i * 3 + 2];

			if(x < minX) minX = x;
			if(x > maxX) maxX = x;
			if(y < minY) minY = y;
			if(y > maxY) maxY = y;
			if(z < minZ) minZ = z;
			if(z > maxZ) maxZ = z;
		}
	}

	Vector3 minSize = (Vector3) { minX, minY, minZ };
	Vector3 maxSize = (Vector3) { maxX, maxY, maxZ };

	r->box = createBox(minSize, maxSize, position);

	return r;
}

Box createBox(Vector3 minSize, Vector3 maxSize, Vector3 pos) {
	Box b; 

	b.v[0] = (Vector3) { minSize.x, minSize.y, maxSize.z };
	b.v[1] = (Vector3) { minSize.x, maxSize.y, maxSize.z };
	b.v[2] = (Vector3) { maxSize.x, maxSize.y, maxSize.z };
	b.v[3] = (Vector3) { maxSize.x, minSize.y, maxSize.z };
	b.v[4] = (Vector3) { minSize.x, minSize.y, minSize.z };
	b.v[5] = (Vector3) { minSize.x, maxSize.y, minSize.z };
	b.v[6] = (Vector3) { maxSize.x, maxSize.y, minSize.z };
	b.v[7] = (Vector3) { maxSize.x, minSize.y, minSize.z };

	for(int i = 0; i < 8; i++) b.vw[i] = Vector3Add(b.v[i], pos);

	Vector3 s0 = Vector3Normalize(Vector3Subtract(b.v[1], b.v[0]));
	Vector3 s1 = Vector3Normalize(Vector3Subtract(b.v[3], b.v[0]));
	Vector3 s2 = Vector3Normalize(Vector3Subtract(b.v[0], b.v[4]));

	b.n[0] = vectorProductNormalized(s1, s0);
	b.n[1] = vectorProductNormalized(s0, s2);
	b.n[2] = vectorProductNormalized(s2, s1);

	return b;
}


void freeRigidBody(RigidBody *r) {
	if(r == NULL) {
		fprintf(stderr, "ERROR trying to free an invalid pointer\n");
		exit(1);
	}
	free(r);
}

void updateRigidBodyPosition(RigidBody *r, Vector3 pos) {
	r->pos = pos;
	for(int i = 0; i < 8; i++) r->box.vw[i] = Vector3Add(r->box.v[i], pos);
}

/* ============= Check Collision Functions =============  */

int checkCollisionAABB(RigidBody *a, RigidBody *b) {
	if(a == NULL || b == NULL) {
		fprintf(stderr, "ERROR cannot check for collision a NULL pointer to a RigidBody!\n");
		exit(1);
	}
	
	Vector3 *av = a->box.vw;
	Vector3 *bv = b->box.vw;
	return !(av[3].x < bv[0].x || av[0].x > bv[3].x || 
			 av[1].y < bv[0].y || av[0].y > bv[1].y ||
			 av[0].z < bv[4].z || av[4].z > bv[0].z);
}

int checkCollision(RigidBody *a, RigidBody *b) {
	if(a == NULL || b == NULL) {
		fprintf(stderr, "ERROR cannot check for collision a NULL pointer to a RigidBody!\n");
		exit(1);
	}
	/* TODO check if rigid body are complex shape or AABB
	 * and use the appropriate collision detection algorithm */

	// check collision using SAT
	return collisionSATBoxAndComplexShape(a, b);
}

int collisionSATBoxAndComplexShape(RigidBody *a, RigidBody *b) {
	Box *box = &(a->box);
	Mesh *m = b->mesh;
	// loop through all triangles
	for(int j = 0; j < b->meshCount; j++) {
		float *vs = m[j].vertices;
		for(int i = 0; i < m[j].vertexCount; i += 3) {
			Vector3 bP = b->pos;
			Vector3 v1 = (Vector3){ vs[i*3+0] + bP.x, vs[i*3+1] + bP.y, vs[i*3+2] + bP.z };
			Vector3 v2 = (Vector3){ vs[i*3+3] + bP.x, vs[i*3+4] + bP.y, vs[i*3+5] + bP.z };
			Vector3 v3 = (Vector3){ vs[i*3+6] + bP.x, vs[i*3+7] + bP.y, vs[i*3+8] + bP.z };
	
			// triangle sides
			Vector3 lt1 = Vector3Subtract(v2, v1);
			Vector3 lt2 = Vector3Subtract(v3, v2);
			Vector3 lt3 = Vector3Subtract(v1, v3);
			// calculate the normal to the triangle
			Vector3 vN = vectorProductNormalized(lt1, lt2);
			// check collision on triangle axis
			if(!checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, vN)) continue;

			// check collision on box axes and vector product between triangle sides and box axes
			for(int j = 0; j < 3; j++) {
				if(!checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, box->n[j]))
					goto noCollisionFound;
			
				vN = vectorProductNormalized(box->n[j], lt1);
				if(!checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, vN))
					goto noCollisionFound;
			
				vN = vectorProductNormalized(box->n[j], lt2);
				if(!checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, vN))
					goto noCollisionFound;
			
				vN = vectorProductNormalized(box->n[j], lt3);
				if(!checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, vN))
					goto noCollisionFound;
			
			}

			return 1;
			noCollisionFound: 
			continue;
		}
	}

	return 0;
}

int checkOverlappingBoxAndTriangleOnAxis(Box *b, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 n) {
	float sbMin = FLT_MAX; float sbMax = -FLT_MAX; float temp;
	for(int i = 0; i < 8; i++) {
		// projection of box vertex i on n
		temp = dotProduct(b->vw[i], n);
		if(temp > sbMax) sbMax = temp;
		if(temp < sbMin) sbMin = temp;
	}
	
	// projection of triangle vertex 1
	float tMin = dotProduct(v1, n);
	float tMax = tMin;
	// projection of triangle vertex 2
	temp = dotProduct(v2, n);
	if     (temp > tMax) tMax = temp;
	else if(temp < tMin) tMin = temp;
	// projection of triangle vertex 3
	temp = dotProduct(v3, n);
	if     (temp > tMax) tMax = temp;
	else if(temp < tMin) tMin = temp;

	return !(sbMax < tMin || sbMin > tMax);	
}

/* ============= Collision Resolution Functions =============  */
Vector3 defaultCallback(RigidBody *a, RigidBody *b, Vector3 collAxis, float amount) {

}
/* ============= Vector Utility Functions =============  */

float dotProduct(Vector3 v1, Vector3 v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

Vector3 vectorProduct(Vector3 v1, Vector3 v2) {
	return (Vector3) {
		.x = v1.y * v2.z - v1.z * v2.y,
		.y = v1.z * v2.x - v1.x * v2.z,
		.z = v1.x * v2.y - v1.y * v2.x
	};
}

Vector3 vectorProductNormalized(Vector3 v1, Vector3 v2) {
	return Vector3Normalize(vectorProduct(v1, v2));
}

