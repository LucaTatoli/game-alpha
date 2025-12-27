#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "stb_perlin.h"
#include "raylib/src/raylib.h"
#include "raylib/src/raymath.h"
#include "sprite.h"
#include "physics.h"

#define WINDOW_TITLE "Alpha"
#define CELL_SIZE 0.25f
#define TREES 30

typedef struct Tile {
	Vector3 position;
	Texture2D *texture;
	Model model;
} Tile;

typedef struct DebugBody {
	Vector3 position;
	Color color;
	Model model;
	Model drawModel;
} DebugBody;

typedef struct TileGrid {
	int rows;
	int cols;
	Tile *tiles[];
} TileGrid;

typedef struct Grid {
	int cellSize;
	int rows;
	int cols;
	Vector3 points[];
} Grid;

typedef struct Entity {
	Vector3 position;
	Vector3 size;
	Color color;
	Model model;
	RigidBody *body;
} Entity;

typedef struct Entity2D {
	Vector3 position;
	Vector2 size;
	Color color;
	RigidBody *body;
} Entity2D;

Vector3 zero_pos = (Vector3) { 0, 0, 0 };

DebugBody *createDebugBody(Color color, Vector3 pos, Vector2 size) {
	DebugBody *d = malloc(sizeof(*d));
	if(d == NULL) {
		printf("Error allocating memory for the debug body\n");
		exit(1);
	}
	d->position = pos;
	d->color = color;
	d->model = LoadModelFromMesh(GenMeshPlane(size.x, size.y, 1, 1));
	d->drawModel = LoadModelFromMesh(GenMeshPlane(size.x, size.y, 1, 1));

	Mesh mesh = d->drawModel.meshes[0];
	for(int i = 0; i < mesh.vertexCount; i++) {
		mesh.vertices[i * 3    ] += d->position.x;
		mesh.vertices[i * 3 + 1] += d->position.y + 0.3f;
		mesh.vertices[i * 3 + 2] += d->position.z;
	}

	UpdateMeshBuffer(mesh, 0, mesh.vertices, mesh.vertexCount * 3 * sizeof(float), 0);

	return d;
}

void updateDebugBodyPosition(DebugBody *d, Vector3 pos) {
	Mesh mesh = d->model.meshes[0];
	Mesh drawMesh = d->drawModel.meshes[0];
	for(int i = 0; i < mesh.vertexCount; i++) {
		drawMesh.vertices[i * 3    ] = mesh.vertices[i * 3    ] + pos.x;
		drawMesh.vertices[i * 3 + 1] = mesh.vertices[i * 3 + 1] + pos.y + 0.3f;
		drawMesh.vertices[i * 3 + 2] = mesh.vertices[i * 3 + 2] + pos.z;
	}

	UpdateMeshBuffer(drawMesh, 0, drawMesh.vertices, drawMesh.vertexCount * 3 * sizeof(float), 0);
}

void drawDebugBody(DebugBody *d) {
	DrawModel(d->drawModel, zero_pos, 1.0f, d->color);
}

Tile *createTile(Texture2D *texture, float posX, float posY, float posZ) {
	Tile *tile = malloc(sizeof(*tile));
	if(tile == NULL) {
		printf("Error allocating memory for the tile\n");
		exit(1);
	}
	tile->position = (Vector3) {
		.x = posX - CELL_SIZE / 2,
		.y = posY - 0.001f,
		.z = posZ - CELL_SIZE / 2
	};


	tile->texture = texture;
	tile->model = LoadModelFromMesh(GenMeshPlane(CELL_SIZE, CELL_SIZE, 1, 1));
	tile->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = *texture;

	Mesh mesh = tile->model.meshes[0];
	for(int i = 0; i < mesh.vertexCount; i++) {
		mesh.vertices[i * 3    ] += tile->position.x;
		mesh.vertices[i * 3 + 1] += tile->position.y;
		mesh.vertices[i * 3 + 2] += tile->position.z;
	}

	UpdateMeshBuffer(mesh, 0, mesh.vertices, mesh.vertexCount * 3 * sizeof(float), 0);

	return tile;
}

TileGrid *createTileGrid(int cols, int rows) {
	TileGrid *tileGrid = malloc(sizeof(*tileGrid)+sizeof(Tile*)*rows*cols);
	tileGrid->cols = cols;
	tileGrid->rows = rows;
}

void drawTileGrid(TileGrid *grid) {
	for(int x = 0; x < grid->cols * grid->rows; x++) {
		Tile *tile = grid->tiles[x];
		DrawModel(tile->model, zero_pos, 1.0f, WHITE);
	}
}

void setTileGridShader(TileGrid *grid, Shader *shader) {
	int tilesCount = grid->rows * grid->cols;
	for(int i = 0; i < tilesCount; i++) 
		grid->tiles[i]->model.materials[0].shader = *shader;
}

Grid *createGrid(int cols, int rows, float cellSize) {
	Grid *grid = malloc(sizeof(*grid)+sizeof(Vector3)*rows*cols);
	if(grid == NULL)
		exit(1);
	
	grid->cellSize = cellSize;
	grid->rows = rows;
	grid->cols = cols;

	for(int y = 0; y < rows; y++) {
		for(int x = 0; x < cols; x++) {
			int index = x + y * cols;
			float sizeY = rows * cellSize;
			float sizeX = cols * cellSize;
			grid->points[index].z =  + (sizeY / 2) - sizeY * (1.0f / rows) * y;
			grid->points[index].x =  - (sizeX / 2) + sizeX * (1.0f / cols) * x;
			grid->points[index].y =  0;
		}
	}
	return grid;
}

void drawGrid(Grid *g, Color c) {
	for(int y = 0; y < g->rows; y++) {
		for(int x = 0; x < g->cols; x++) {
			Vector3 *curr    = &g->points[x     +  y      * g->cols];
			Vector3 *nextCol = &g->points[x + 1 +  y      * g->cols];
			Vector3 *nextRow = &g->points[x     + (y + 1) * g->cols];
			if(x < g->cols -1)
				DrawLine3D(*curr, *nextCol, c);
			if(y < g->rows - 1)
				DrawLine3D(*curr, *nextRow, c);
		}
	}
}

void setTileInTileGrid(TileGrid *grid, Tile *tile, int x, int y) {
	grid->tiles[x+y*grid->cols] = tile;
}

Camera3D camera = {0};
#if ISOMETRIC
	const Vector3 cameraDirection = (Vector3) { 0.0f, -0.6f, -1.0f };
#else
	const Vector3 cameraDirection = (Vector3) { 0.0f, 0.0f, -1.0f };
#endif
float cameraSpeed = 0.6f;
float sprintSpeed = 1.2f;
	
Entity2D player = {
	.position = (Vector3) { 0.0f, 0.15f, -0.8f },
	.size     = (Vector2) { 0.3f, 0.3f }//,  0.1f }
};

RigidBody *bridgeBody;
RigidBody *bridgeBody2;
RigidBody *treeBody[TREES];

void handleInputs(AnimatedSprite *a) {
	float speed = cameraSpeed * GetFrameTime();

	Vector3 speedV = (Vector3) { .x = .0f, .y = .0f, .z = .0f };
	if(IsKeyDown(KEY_LEFT_SHIFT)) {
		speed = sprintSpeed * GetFrameTime();
		a->frameTimer += GetFrameTime();
	}
	if(IsKeyDown(KEY_W)) {
		speedV.z  = -speed;
		switchAnimationType(a, WALK_U_ANIM);
	}
	else if(IsKeyDown(KEY_S)) {
		speedV.z  = speed;
		switchAnimationType(a, WALK_D_ANIM);
	}
	else if(IsKeyDown(KEY_A)) {
		speedV.x  = -speed;
		switchAnimationType(a, WALK_L_ANIM);
	}
	else if(IsKeyDown(KEY_D)) {
		speedV.x  = speed;
		switchAnimationType(a, WALK_R_ANIM);
	}
	else if(a->currAnimation > IDLE_L_ANIM) {
		switchAnimationType(a, a->currAnimation - 4); // each walk animation is idle animation + 4
	}

	camera.position  = Vector3Add(camera.position, speedV);
	player.position  = Vector3Add(player.position, speedV);
	player.body->pos = Vector3Add(player.body->pos, speedV);
	for(int i = 0; i < TREES; i++) {
		int collision = checkCollisionAABB(player.body, treeBody[i]);
		if(collision == 1 && checkCollision(player.body, treeBody[i]))  {
			camera.position  = Vector3Subtract(camera.position, speedV);
			player.position  = Vector3Subtract(player.position, speedV);
			player.body->pos = Vector3Subtract(player.body->pos, speedV);
		}
	}
	int collision = checkCollisionAABB(player.body, bridgeBody);
	if(collision == 1 && checkCollision(player.body, bridgeBody))  {
		camera.position  = Vector3Subtract(camera.position, speedV);
		player.position  = Vector3Subtract(player.position, speedV);
		player.body->pos = Vector3Subtract(player.body->pos, speedV);
	}
	
	collision = checkCollisionAABB(player.body, bridgeBody2);
	if(collision == 1 && checkCollision(player.body, bridgeBody2))  {
		camera.position  = Vector3Subtract(camera.position, speedV);
		player.position  = Vector3Subtract(player.position, speedV);
		player.body->pos = Vector3Subtract(player.body->pos, speedV);
	}

	camera.target = Vector3Add(cameraDirection, camera.position);
	
	if(IsKeyPressed(KEY_F1)) ToggleFullscreen();
}

Entity createEntity(Texture2D texture, Vector3 pos, Vector3 size) {
		float randValue = GetRandomValue(1, 3)*0.1f;
		Entity e = (Entity) {
			.position = pos,
			.size = size
		};
		e.color = WHITE;
		Mesh mesh = GenMeshCube(e.size.x, e.size.y, e.size.z);
		e.body = createRigidBodyFromMesh(RIGID, &mesh, 1, pos);
		e.model = LoadModelFromMesh(mesh);
		e.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
		return e;
}

int main(void)
{
	SetConfigFlags(FLAG_MSAA_4X_HINT);// | FLAG_VSYNC_HINT);
	SetRandomSeed(10);

	int width = 720;
	int height = 1280;
    InitWindow(width, height, WINDOW_TITLE);
	HideCursor();
	int monitor = GetCurrentMonitor();
	width = GetMonitorWidth(monitor);
	height = GetMonitorHeight(monitor);
	SetWindowSize(width, height);
	Vector2 resolution = (Vector2) { width, height };

    //InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
	ClearWindowState(FLAG_VSYNC_HINT);
	
	int rows = 100;
	int cols = 100;
	
	Vector3 pSize = (Vector3){ .x = player.size.x * 0.7f, .y = player.size.y, .z = player.size.x * 0.7f };
	player.body = createRigidBody(RIGID, player.position, pSize);

	Grid *grid = createGrid(cols, rows, CELL_SIZE);
	TileGrid *tileGrid = createTileGrid(cols, rows);
	Image tileImage1 = LoadImage("res/snowygrass1.png");
	ImageMipmaps(&tileImage1);
	Texture2D tileTexture1 = LoadTextureFromImage(tileImage1);
	UnloadImage(tileImage1);
	Image tileImage2 = LoadImage("res/snowygrass2.png");
	ImageMipmaps(&tileImage2);
	Texture2D tileTexture2 = LoadTextureFromImage(tileImage2);
	UnloadImage(tileImage2);
	Image tileImage3 = LoadImage("res/snowygrass3.png");
	ImageMipmaps(&tileImage3);
	Texture2D tileTexture3 = LoadTextureFromImage(tileImage3);
	UnloadImage(tileImage3);
	Image tileImage4 = LoadImage("res/snowygrass4.png");
	ImageMipmaps(&tileImage4);
	Texture2D tileTexture4 = LoadTextureFromImage(tileImage4);
	UnloadImage(tileImage4);
	for(int y = 0; y < tileGrid->rows; y++) {
		for(int x = 0; x < tileGrid->cols; x++) {
			float sizeY = rows * CELL_SIZE;
			float sizeX = cols * CELL_SIZE;
			float posZ =  + (sizeY / 2) - sizeY * (1.0f / rows) * y;
			float posX =  - (sizeX / 2) + sizeX * (1.0f / cols) * x;
			int textureIndex = GetRandomValue(1, 4);
			Tile *tile;
			switch(textureIndex) {
				case 1: tile = createTile(&tileTexture1, posX, 0, posZ); break;
				case 2: tile = createTile(&tileTexture2, posX, 0, posZ); break;
				case 3: tile = createTile(&tileTexture3, posX, 0, posZ); break;
				case 4: tile = createTile(&tileTexture4, posX, 0, posZ); break;
			}
			setTileInTileGrid(tileGrid, tile, x, y);
		}
	}

	#if ISOMETRIC
	 	camera.position = (Vector3){
    		0.0f,
		    2.0f,
	        1.5f
		};
	#else
		camera.position = (Vector3){
	    	0.0f,
		    0.5f,
	        0.5f
		};
	#endif

	camera.target = cameraDirection;

	camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	Shader lightFSShader = LoadShader("res/shaders/light.vs", "res/shaders/light.fs");
	Shader lightNoTexShader = LoadShader("res/shaders/light.vs", "res/shaders/lightNoTex.fs");
	Shader leavesShader = LoadShader("res/shaders/light.vs", "res/shaders/transparency.fs");

	Vector4 color = (Vector4) { 1.0f, 0.0f, 0.0f, 1.0f };

	//Vector3 lightColor      = (Vector3) { 0, 0, 0 };
	Vector3 lightColor      = (Vector3) { 0.3f, 0.3f, 0.3f };
	Vector3 ambient         = (Vector3) { 0.5f, 0.5f, 0.5f };
	//Vector3 localLightColor = (Vector3) { 0, 0, 0 };
	Vector3 localLightColor = (Vector3) { 0.3f, 0.3f, 0.3f };

	Sprite *sprite = createSprite("res/glass_sheet.png", 42);
	AnimationConfig *config = parseAnimationConfig("configs.csv");
	AnimationFrames *frames = createAnimationFrames(config, sprite->size);
	AnimatedSprite *aSprite = createAnimatedSprite(sprite, frames, 4);
	freeAnimationConfig(config);

	int count = rows*cols/3;
	Vector3 billboardsPositions[count];
	for(int i = 0; i < count; i++) {
		float x = (GetRandomValue(0, cols) - cols / 2) * CELL_SIZE;
		float z = (GetRandomValue(0, rows) - rows / 2) * CELL_SIZE + CELL_SIZE / 2;
		billboardsPositions[i] = (Vector3) {
			.x = x,
			.y = CELL_SIZE / 2,
			.z = z
		};
	}

	Texture2D leavesTexture = LoadTexture("res/snowyleaves.png");

	SetTextureFilter(tileTexture1, TEXTURE_FILTER_ANISOTROPIC_16X);
	SetTextureFilter(tileTexture2, TEXTURE_FILTER_ANISOTROPIC_16X);
	SetTextureFilter(tileTexture3, TEXTURE_FILTER_ANISOTROPIC_16X);
	SetTextureFilter(tileTexture4, TEXTURE_FILTER_ANISOTROPIC_16X);
	SetTextureFilter(leavesTexture, TEXTURE_FILTER_ANISOTROPIC_16X);
	//Color background = (Color) {99, 155, 255, 255};
	Color background = (Color) {floor(255 * lightColor.x), floor(255 * lightColor.y), floor(255 * lightColor.z), 255};
	setTileGridShader(tileGrid, &lightFSShader);
	//RenderTexture2D renderTexture = LoadRenderTexture(width, height);

	// Debug objects
	//DebugBody *playerDebug = createDebugBody(WHITE, player.position, player.size);
	// End debug objects

	RenderTexture2D canvas = LoadRenderTexture(resolution.x, resolution.y);

	Vector2 origin = (Vector2) { 0, 0 };
	Rectangle source = (Rectangle) { 0, 0, resolution.x, -resolution.y };
	Rectangle dest   = (Rectangle) { 0, 0, resolution.x,  resolution.y };

	Shader canvasShader = LoadShader("res/shaders/light.vs", "res/shaders/test.fs");
	SetShaderValue(canvasShader, GetShaderLocation(canvasShader, "resolution"), &resolution, SHADER_UNIFORM_VEC2);
	
	// loading obj model
	Model bridge = LoadModel("res/objs/bridge.obj");
	Material *bridgeMaterial = LoadMaterials("res/objs/bridge.mtl", &bridge.materialCount);
	bridge.materials[0] = bridgeMaterial[0];
	bridge.materials[0].shader = lightFSShader;
	Texture bridgeTexture = LoadTexture("res/objs/stone.jpg");
	bridge.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = bridgeTexture;
	Vector3 bridgePos = (Vector3){ 0, 0.88f, -4.0f };
	bridgeBody = createRigidBodyFromMesh(RIGID, bridge.meshes, bridge.meshCount, bridgePos);
	
	Model bridge2 = LoadModel("res/objs/bridge2.obj");
	Material *bridgeMaterial2 = LoadMaterials("res/objs/bridge2.mtl", &bridge2.materialCount);
	bridge2.materials[0] = bridgeMaterial2[0];
	bridge2.materials[0].shader = lightFSShader;
	bridge2.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = bridgeTexture;
	Vector3 bridgePos2 = (Vector3){ -2.0f, 1.38f, -9.0f };
	bridgeBody2 = createRigidBodyFromMesh(RIGID, bridge2.meshes, bridge2.meshCount, bridgePos2);
	
	Model tree= LoadModel("res/objs/tree.obj");
	printf("Model loaded\n");
	for(int i = 0; i < tree.materialCount; i++) tree.materials[i].shader = lightFSShader;
	Vector3 treePos[TREES];
	for(int i = 0; i < TREES; i++) {
		treePos[i] = (Vector3) { 
			GetRandomValue(-cols / 2, cols/2) * CELL_SIZE,	
			0,
			GetRandomValue(-cols / 2, cols/2) * CELL_SIZE	
		};
		treeBody[i] = createRigidBodyFromMesh(RIGID, tree.meshes, tree.meshCount, treePos[i]);
		treePos[i].y = 0 - treeBody[i]->box.minSize.y;
		treeBody[i]->pos.y = treePos[i].y;

		if(checkCollisionAABB(bridgeBody, treeBody[i]) || checkCollisionAABB(bridgeBody2, treeBody[i])) {
			freeRigidBody(treeBody[i]);
			i--;
		}
		for(int j = 0; j < i; j++) {
			if(checkCollisionAABB(treeBody[i], treeBody[j])) {
				freeRigidBody(treeBody[i]);
				i--;
				break;
				
			}
		}
	}
	// Christmas snowflakes
	int flakes = 5000;
	Entity *snow = malloc(sizeof(*snow)*flakes);
	Vector3 *dirs = malloc(sizeof(*dirs)*flakes) ;
	Texture2D snowTexture = LoadTexture("res/snow.png");
	#if ISOMETRIC
	float snowInitialY = camera.position.y + 0.25f;
	#else
	float snowInitialY = camera.position.y * 5;
	#endif
	for(int i = 0; i < flakes; i++) {
		float x = GetRandomValue(-cols / 2, cols  / 2) * CELL_SIZE;
		float y = snowInitialY + GetRandomValue(0, 50) / 10.0f;
		float z = GetRandomValue(-rows / 2, rows / 2) * CELL_SIZE;
		Vector3 pos = (Vector3) { x, y, z };
		Vector3 size = (Vector3) { 0.005f, 0.005f, 0.005f };
		snow[i] = createEntity(snowTexture, pos, size);
		snow[i].model.materials[0].shader = lightFSShader;
		dirs[i] = (Vector3) { GetRandomValue(1, 10) / 50.0f, -GetRandomValue(5, 10) / 10.0f, GetRandomValue(1, 10) / 50.0f};
	}

    while (!WindowShouldClose())
    {
        BeginTextureMode(canvas);
		ClearBackground(background);
		BeginMode3D(camera);
		//drawGrid(grid, BLACK);
		drawTileGrid(tileGrid);
		BeginShaderMode(lightFSShader);
		SetShaderValue(lightFSShader, GetShaderLocation(lightFSShader, "colDiffuse"), &color, SHADER_UNIFORM_VEC4);
		SetShaderValue(lightFSShader, GetShaderLocation(lightFSShader, "localLight"), &player.position, SHADER_UNIFORM_VEC3);
		SetShaderValue(lightFSShader, GetShaderLocation(lightFSShader, "localLightColor"), &localLightColor, SHADER_UNIFORM_VEC3);
		SetShaderValue(lightFSShader, GetShaderLocation(lightFSShader, "lightColor"), &lightColor, SHADER_UNIFORM_VEC3);
		SetShaderValue(lightFSShader, GetShaderLocation(lightFSShader, "ambient"), &ambient, SHADER_UNIFORM_VEC3);

		float time = GetTime() + 1.0f / GetRandomValue(4, 10);
		SetShaderValue(lightFSShader, GetShaderLocation(lightFSShader, "time"), &time, SHADER_UNIFORM_FLOAT);

		Vector3 rotAxis = (Vector3){ 0.0f, 1.0f, 0.0f };
		Vector3 scale = (Vector3) { 1.0f, 1.0f, 1.0f };
		// DrawingBridge
		
		DrawModelEx(bridge, bridgePos, rotAxis, 0,  scale, WHITE);
		DrawModelEx(bridge2, bridgePos2, rotAxis, 0,  scale, WHITE);
		
		for(int i = 0; i < TREES; i++) {
			DrawModelEx(tree, treePos[i], rotAxis, 0, scale, WHITE);
			Box b = treeBody[i]->box;
			/*DrawCubeV(treePos[i],
					 Vector3Subtract(b.maxSize, b.minSize),
					 RED);*/
		}

		// Draw snowflakes
		for(int i = 0; i < flakes; i++) DrawModel(snow[i].model, snow[i].position, 1.0f, WHITE);
		EndShaderMode();
		BeginShaderMode(leavesShader);
		Vector3 billboardLightPosition = (Vector3) { player.position.x, player.position.y, player.position.z };
		SetShaderValue(leavesShader, GetShaderLocation(leavesShader, "localLight"), &billboardLightPosition, SHADER_UNIFORM_VEC3);
		SetShaderValue(leavesShader, GetShaderLocation(leavesShader, "time"), &time, SHADER_UNIFORM_FLOAT);
		SetShaderValue(leavesShader, GetShaderLocation(leavesShader, "localLightColor"), &localLightColor, SHADER_UNIFORM_VEC3);
		SetShaderValue(leavesShader, GetShaderLocation(leavesShader, "lightColor"), &lightColor, SHADER_UNIFORM_VEC3);
		SetShaderValue(leavesShader, GetShaderLocation(leavesShader, "ambient"), &ambient, SHADER_UNIFORM_VEC3);
		drawAnimatedSpriteBillboard(aSprite, camera, player.position, player.size, GetFrameTime());
		
		for(int i = 0; i < count; i++) 
			DrawBillboardPro(camera, 
			 				 leavesTexture,
							 (Rectangle) { 0, 0, leavesTexture.width, leavesTexture.height },
							 billboardsPositions[i],
							 (Vector3) { 0, 1, 0 },
							 (Vector2) { CELL_SIZE, CELL_SIZE },
							 (Vector2) { CELL_SIZE / 2, CELL_SIZE / 2 },
							 0.0f,
							 WHITE);


		EndShaderMode();
		EndMode3D();
	
		DrawPixel(10, 10, RED);
		EndTextureMode();
		BeginDrawing();
		BeginShaderMode(canvasShader);
		DrawTexturePro(canvas.texture, source, dest, origin, 0, WHITE);
		EndShaderMode();
        EndDrawing();

		handleInputs(aSprite);

		// update snowflakes
		float frameTime = GetFrameTime();
		for(int i = 0; i < flakes; i++) {
			snow[i].position.x += dirs[i].x * frameTime;	
			snow[i].position.y += dirs[i].y * frameTime;	
			snow[i].position.z += dirs[i].z * frameTime;	
			if(snow[i].position.y < 0) snow[i].position.y = snowInitialY;
		}
		
    }

	UnloadShader(lightFSShader);
    CloseWindow();

    return 0;
}
