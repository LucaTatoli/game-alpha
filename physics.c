#include "physics.h"
#include "utils.h"
#include "raylib/src/raymath.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* =============== Structs Manipulation Functions ===============  */

RigidBody *createRigidBody(BodyType type, Vector3 position, Vector3 size) {
	if(world.bodyCount == world.maxBodies) {
		fprintf(stderr, "ERROR can't add more bodies to the world\n");
		exit(1);
	}
	RigidBody *r = xmalloc(sizeof(*r));

	r->type = type;
	r->pos = position;
	Vector3 minSize = (Vector3) { -size.x / 2, -size.y / 2, -size.z / 2 };
	Vector3 maxSize = (Vector3) {  size.x / 2,  size.y / 2,  size.z / 2 };
	r->box = createBox(minSize, maxSize, position);
	r->grounded = 1;

	world.bodies[world.bodyCount++] = r;
	return r;
}

RigidBody *createRigidBodyFromMesh(BodyType type, Mesh *mesh, int meshCount, Vector3 position) {
	if(world.bodyCount == world.maxBodies) {
		fprintf(stderr, "ERROR can't add more bodies to the world\n");
		exit(1);
	}
	RigidBody *r = xmalloc(sizeof(*r));

	r->type = type;
	r->mesh = mesh;
	r->meshCount = meshCount;
	r->pos = position;
	r->grounded = 1;

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
	
	world.bodies[world.bodyCount++] = r;

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

	float sizeX = maxSize.x - minSize.x;
	float sizeY = maxSize.y - minSize.y;
	float sizeZ = maxSize.z - minSize.z;
	b.center = (Vector3) {
		.x = minSize.x + sizeX / 2, 
		.y = minSize.y + sizeY / 2, 
		.z = minSize.z + sizeZ / 2, 
	};

	b.wCenter = (Vector3) {
		.x = b.center.x + pos.x,
		.y = b.center.y + pos.y,
		.z = b.center.z + pos.z,
	};

	return b;
}


void freeRigidBody(RigidBody *r) {
	if(r == NULL) {
		fprintf(stderr, "ERROR trying to free an invalid pointer\n");
		exit(1);
	}
	free(r);
	// TODO add real world bodies handling while freeing
	world.bodyCount--;
}

void updateRigidBodyPosition(RigidBody *r, Vector3 pos) {
	r->pos = pos;
	for(int i = 0; i < 8; i++) r->box.vw[i] = Vector3Add(r->box.v[i], pos);
}

/* ============= Check Collision Functions =============  */

CollisionInfo checkCollisionAABB(RigidBody *a, RigidBody *b) {
	if(a == NULL || b == NULL) {
		fprintf(stderr, "ERROR cannot check for collision a NULL pointer to a RigidBody!\n");
		exit(1);
	}
	
	Vector3 *av = a->box.vw;
	Vector3 *bv = b->box.vw;

	CollisionInfo i;
	/* TODO add real collision info */
	if(!(av[3].x < bv[0].x || av[0].x > bv[3].x || 
		 av[1].y < bv[0].y || av[0].y > bv[1].y ||
		 av[0].z < bv[4].z || av[4].z > bv[0].z)) {
		 i.baseLength = 1;
		 i.length = 1;
	} else {
		i.baseLength = -1;
		i.length = -1;
	}
	return i;
}

CollisionInfo checkCollision(RigidBody *a, RigidBody *b) {
	if(a == NULL || b == NULL) {
		fprintf(stderr, "ERROR cannot check for collision a NULL pointer to a RigidBody!\n");
		exit(1);
	}

	CollisionInfo i = checkCollisionAABB(a, b);
	if(i.baseLength < 0) return i;
	/* TODO check if rigid body are complex shape or AABB
	 * and use the appropriate collision detection algorithm */

	// check collision using SAT
	return collisionSATBoxAndComplexShape(a, b);
}

CollisionInfo collisionSATBoxAndComplexShape(RigidBody *a, RigidBody *b) {
	Box *box = &(a->box);
	Mesh *m = b->mesh;
	CollisionInfo info;
	info.baseLength = -1;
	info.length = -1;
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
			float collLength  = checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, vN);
			Vector3 collAxis = vN;
			if(!collLength) goto noCollisionFound;
			
			
			// check collision on box axes and vector product between triangle sides and box axes
			float tmpLength;
			for(int j = 0; j < 3; j++) {
				
				tmpLength = checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, box->n[j]);
				if(!tmpLength) goto noCollisionFound;
				//if(tmpLength < collLength) {
				//	collLength = tmpLength;
				//	collAxis = box->n[j];
				//}
			
				Vector3 vp = vectorProductNormalized(box->n[j], lt1);
				if(Vector3Length(vp) > 0.00001f) {
					tmpLength = checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, vp);
					if(!tmpLength) goto noCollisionFound;
					//if(tmpLength < collLength) {
					//	collLength = tmpLength;
					//	collAxis = vp;
					//}
				}
			
				vp = vectorProductNormalized(box->n[j], lt2);
				if(Vector3Length(vp) > 0.00001f) {
					tmpLength = checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, vp);
					if(!tmpLength) goto noCollisionFound;
					//if(tmpLength < collLength) {
					//	collLength = tmpLength;
					//	collAxis = vp;
					//}
				}
			
				vp = vectorProductNormalized(box->n[j], lt3);
				if(Vector3Length(vp) > 0.00001f) {
					tmpLength = checkOverlappingBoxAndTriangleOnAxis(box, v1, v2, v3, vp);
					if(!tmpLength) goto noCollisionFound;
					//if(tmpLength < collLength) {
					//	collLength = tmpLength;
					//	collAxis = vp;
					//}
				}
			}

			
			Vector3 tCenter = Vector3Scale(Vector3Add(Vector3Add(v1, v2), v3), 1.0f/3.0f);
			Vector3 dir = Vector3Subtract(box->wCenter, tCenter);
			if(dotProduct(dir, collAxis) > 0 && (info.length == -1 || info.length > collLength)) {
				info.v1 = v1;
				info.v2 = v2;
				info.v3 = v3;
				info.length = collLength;
				info.direction = collAxis;
			}
		
			printf("==================================\n");
			printf("Colliding with triangle\n");
			printf("V1 (%f, %f, %f)\n", v1.x, v1.y, v1.z);
			printf("V2 (%f, %f, %f)\n", v2.x, v2.y, v2.z);
			printf("V3 (%f, %f, %f)\n", v3.x, v3.y, v3.z);
			printf("Collision direction\n");
			printf("N (%f, %f, %f)\n", collAxis.x, collAxis.y, collAxis.z);
			printf("Collision length\n");
			printf("L %f\n", collLength);
			printf("Center direction\n");
			printf("D (%f, %f, %f)\n", dir.x, dir.y, dir.z);
			printf("Dot Product\n");
			printf("%f\n", dotProduct(dir, collAxis));
			printf("Box center\n");
			printf("B (%f, %f, %f)\n", box->wCenter.x, box->wCenter.y, box->wCenter.z);
			printf("Tri center\n");
			printf("T (%f, %f, %f)\n", tCenter.x, tCenter.y, tCenter.z);

			
			noCollisionFound: 
			continue;
		}
	}

	return info;
}

float checkOverlappingBoxAndTriangleOnAxis(Box *b, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 n) {
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
	if(temp > tMax) tMax = temp;
	if(temp < tMin) tMin = temp;
	// projection of triangle vertex 3
	temp = dotProduct(v3, n);
	if(temp > tMax) tMax = temp;
	if(temp < tMin) tMin = temp;

	if (sbMax < tMin || sbMin > tMax) return 0;		
	
	float d1 = sbMax -  tMin;
	float d2 =  tMax - sbMin;

	return d1 > d2 ? d2 : d1;
}

float checkOverlappingBoxBaseAndTriangleOnAxis(Box *b, Vector3 v1, Vector3 v2, Vector3 v3, Vector3 n) {
	// check if base and triangle are on the same y level
	float boxY = b->vw[0].y;
	float triY = v1.y;
	if(v2.y < triY) triY = v2.y;
	if(v3.y < triY) triY = v3.y;
	float threshold = triY - boxY;
	//printf("Threshold: %f\n", threshold);
	if(threshold > 0.1f) return 0;
	// projection of base vertex 1
	float sbMin = dotProduct(b->vw[0], n); 
	float sbMax = sbMin; 
	// projection of base vertex 2
	float temp = dotProduct(b->vw[3], n);
	if(temp > sbMax) sbMax = temp;
	else if(temp < sbMin) sbMin = temp;
	// projection of base vertex 3
	temp = dotProduct(b->vw[4], n);
	if(temp > sbMax) sbMax = temp;
	else if(temp < sbMin) sbMin = temp;
	// projection of base vertex 4
	temp = dotProduct(b->vw[7], n);
	if(temp > sbMax) sbMax = temp;
	else if(temp < sbMin) sbMin = temp;
	
	
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

	float d1 = sbMax -  tMin;
	float d2 =  tMax - sbMin;

	return d1 > d2 ? d2 : d1;
}

/* ============= Collision Resolution Functions =============  */

void handleCollision(RigidBody *a, RigidBody *b, CollisionInfo i, float frameTime) {
	// TODO handle PHANTOM collisions
	if(a->type == PHANTOM || b->type == PHANTOM) return;
	if(a->type == RIGID_FIXED && b->type == RIGID_FIXED) return;
	
	if(a->type == RIGID && b->type == RIGID) {
		Vector3 dir = Vector3Scale(i.direction, i.length / 2);
		a->pos = Vector3Add(a->pos, dir);
		b->pos = Vector3Subtract(b->pos, dir);
	}

	Vector3 offset = Vector3Scale(i.direction, i.length);
	if(b->type == RIGID_FIXED) {
		// axis are already normalized, so I don't need to divide by the axis length
		float slopeAngle = dotProduct(i.direction, upVector);
		
		//if(slopeAngle > maxSlope) printf("Slope is walkable!\n"); 
		//else				 printf("Can't walk up this slope!\n");

		a->pos = Vector3Add(a->pos, offset);
		for(int i = 0; i < 8; i++) a->box.vw[i] = Vector3Add(a->box.v[i], a->pos);
		a->box.wCenter = Vector3Add(a->box.center, a->pos);
	}
	else { 
		float slopeDir = dotProduct(b->vel, i.baseDirection);
		if(slopeDir < 0) b->pos = Vector3Add(b->pos, offset); 
		else {
			Vector3 normalizedVel = Vector3Normalize(b->vel);
			b->pos = Vector3Add(b->pos, Vector3Scale(normalizedVel, -i.baseLength));
		}
	}
}

/* ============= State Update Functions =============  */

void updateWorld(float frameTime) {
	// update bodies position
	for(int b = 0; b < world.bodyCount; b++) {
		RigidBody *r = world.bodies[b];
		
		if(r->type == RIGID_FIXED || r->type == PHANTOM) continue;

		if(!r->grounded && r->pos.y >= 0.15f)
			r->vel.y -= world.gravity;
		
		Vector3 vel = Vector3Scale(r->vel, frameTime);
		r->pos = Vector3Add(r->pos, vel);
		for(int i = 0; i < 8; i++) r->box.vw[i] = Vector3Add(r->box.v[i], r->pos);
		r->box.wCenter = Vector3Add(r->box.center, r->pos);
	}

	// check collisions
	for(int i = 0; i < world.bodyCount-1; i++) {
		RigidBody *a = world.bodies[i];
		a->grounded = 0;
		
		if(a->type == RIGID_FIXED || a->type == PHANTOM) continue;
		
		for(int y = i+1; y < world.bodyCount; y++) {
			RigidBody *b = world.bodies[y];
			CollisionInfo info = checkCollision(a,b);
			if(info.length > 0) {
				handleCollision(a, b, info, frameTime);
				a->grounded = 1;
			}
		}
	}

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

float vectorDistance(Vector3 v1, Vector3 v2) {
	float x = v1.x - v2.x;
	float y = v1.y - v2.y;
	float z = v1.z - v2.z;

	return sqrt(x*x + y*y + z*z);
}

/* ============= Other Utility Functions =============  */

float map(float value, float min, float max, float nMin, float nMax) {
  return nMin + (value - min) * (nMax - nMin) / (max - min);
}
