#include "physics.h"
#include "utils.h"
#include "raylib/src/raymath.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============= Struct Creation Functions =============  */

RigidBody *createRigidBody(BodyType type, Vector3 position, Vector3 size) {
	RigidBody *r = xmalloc(sizeof(*r));

	r->type = type;
	r->pos = position;
	r->box.minSize = (Vector3) { -size.x / 2, -size.y / 2, -size.z / 2 };
	r->box.maxSize = (Vector3) {  size.x / 2,  size.y / 2,  size.z / 2 };
	r->satBox = createSATBox(&(r->box), position);

	return r;
}

RigidBody *createRigidBodyFromMesh(BodyType type, Mesh *mesh, int meshCount, Vector3 position) {
	RigidBody *r = xmalloc(sizeof(*r));

	r->type = type;
	r->pos = position;
	r->mesh = mesh;
	r->meshCount = meshCount;

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

	r->box.minSize = (Vector3) { minX, minY, minZ };
	r->box.maxSize = (Vector3) { maxX, maxY, maxZ };

	return r;
}

SATBox *createSATBox(Box *b, Vector3 pos) {
	Vector3 aSmax = b->maxSize;
	Vector3 aSmin = b->minSize;
	SATBox *sb = xmalloc(sizeof(*sb));

	sb->v[0] = (Vector3) { pos.x + aSmin.x, pos.y + aSmin.y, pos.z + aSmax.z };
	sb->v[1] = (Vector3) { pos.x + aSmin.x, pos.y + aSmax.y, pos.z + aSmax.z };
	sb->v[2] = (Vector3) { pos.x + aSmax.x, pos.y + aSmax.y, pos.z + aSmax.z };
	sb->v[3] = (Vector3) { pos.x + aSmax.x, pos.y + aSmin.y, pos.z + aSmax.z };
	sb->v[4] = (Vector3) { pos.x + aSmin.x, pos.y + aSmin.y, pos.z + aSmin.z };
	sb->v[5] = (Vector3) { pos.x + aSmin.x, pos.y + aSmax.y, pos.z + aSmin.z };
	sb->v[6] = (Vector3) { pos.x + aSmax.x, pos.y + aSmax.y, pos.z + aSmin.z };
	sb->v[7] = (Vector3) { pos.x + aSmax.x, pos.y + aSmin.y, pos.z + aSmin.z };

	sb->s[0] = Vector3Normalize(Vector3Subtract(sb->v[1], sb->v[0]));
	sb->s[1] = Vector3Normalize(Vector3Subtract(sb->v[3], sb->v[0]));
	sb->s[2] = Vector3Normalize(Vector3Subtract(sb->v[0], sb->v[4]));

	sb->n[0] = vectorProductNormalized(sb->s[1], sb->s[0]);
	sb->n[1] = vectorProductNormalized(sb->s[0], sb->s[2]);
	sb->n[2] = vectorProductNormalized(sb->s[2], sb->s[1]);

	return sb;
}


void freeRigidBody(RigidBody *r) {
	if(r == NULL) {
		fprintf(stderr, "ERROR trying to free an invalid pointer\n");
		exit(1);
	}
	free(r);
}

/* ============= Check Collision Functions =============  */

int checkCollisionAABB(RigidBody *a, RigidBody *b) {
	if(a == NULL || b == NULL) {
		fprintf(stderr, "ERROR cannot check for collision a NULL pointer to a RigidBody!\n");
		exit(1);
	}
	
	float aMinX = a->pos.x + a->box.minSize.x;
	float aMaxX = a->pos.x + a->box.maxSize.x;
	float bMinX = b->pos.x + b->box.minSize.x;
	float bMaxX = b->pos.x + b->box.maxSize.x;
	float aMinY = a->pos.y + a->box.minSize.y;
	float aMaxY = a->pos.y + a->box.maxSize.y;
	float bMinY = b->pos.y + b->box.minSize.y;
	float bMaxY = b->pos.y + b->box.maxSize.y;
	float aMinZ = a->pos.z + a->box.minSize.z;
	float aMaxZ = a->pos.z + a->box.maxSize.z;
	float bMinZ = b->pos.z + b->box.minSize.z;
	float bMaxZ = b->pos.z + b->box.maxSize.z;

	return !(aMaxX < bMinX || aMinX > bMaxX || aMaxY < bMinY || aMinY > bMaxY || aMaxZ < bMinZ || aMinZ > bMaxZ);
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
	SATBox *sb = a->satBox;
	Mesh *m = b->mesh;
	// loop through all triangles
	for(int j = 0; j < b->meshCount; j++) {
		float *vs = m[j].vertices;
		for(int i = 0; i < m[j].vertexCount; i += 3) {
			Vector3 bP = b->pos;
			Vector3 v1 = (Vector3){ vs[i*3+0] + bP.x, vs[i*3+1] + bP.y, vs[i*3+2] + bP.z };
			Vector3 v2 = (Vector3){ vs[i*3+3] + bP.x, vs[i*3+4] + bP.y, vs[i*3+5] + bP.z };
			Vector3 v3 = (Vector3){ vs[i*3+6] + bP.x, vs[i*3+7] + bP.y, vs[i*3+8] + bP.z };
	
			// check collision on box axes
			int hasCollision = 1;
			for(int j = 0; j < 3; j++)
				if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, sb->n[j])) {
					hasCollision = 0;
					break;
			};
			if(!hasCollision) continue;

			// triangle sides
			Vector3 lt1 = Vector3Subtract(v2, v1);
			Vector3 lt2 = Vector3Subtract(v3, v2);
			Vector3 lt3 = Vector3Subtract(v1, v3);
			// calculate the normal to the triangle
			Vector3 vN = vectorProductNormalized(lt1, lt2);
			// check collision on triangle axis
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;

			// check collision on dot product between triangle and box sides
			vN = vectorProductNormalized(sb->s[0], lt1);
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;

			vN = vectorProductNormalized(sb->s[1], lt1);
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;
	
			vN = vectorProductNormalized(sb->s[2], lt1);
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;

			vN = vectorProductNormalized(sb->s[0], lt2);
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;

			vN = vectorProductNormalized(sb->s[1], lt2);
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;
	
			vN = vectorProductNormalized(sb->s[2], lt2);
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;

			vN = vectorProductNormalized(sb->s[0], lt3);
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;

			vN = vectorProductNormalized(sb->s[1], lt3);
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;
	
			vN = vectorProductNormalized(sb->s[2], lt3);
			if(!checkOverlappingBoxAndTriangleOnAxis(sb, v1, v2, v3, vN)) continue;

			return 1;
		}
	}

	return 0;
}

int checkOverlappingBoxAndTriangleOnAxis(SATBox *sb, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 n) {
	float sbMin = FLT_MAX; float sbMax = -FLT_MAX; float temp;
	for(int i = 0; i < 8; i++) {
		// projection of box vertex i on n
		temp = dotProduct(sb->v[i], n);
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

