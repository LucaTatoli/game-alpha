#include "sprite.h"
#include <stdio.h>
#include "raylib/src/raylib.h"

int main() {
	
	SetConfigFlags(FLAG_VSYNC_HINT);
	Vector2 resolution = (Vector2) { 1280, 720 };
	InitWindow(resolution.x, resolution.y, "prova");
	SetWindowState(FLAG_FULLSCREEN_MODE);

	Sprite *sprite = createSprite("res/glass_sheet.png", 42);
	AnimationConfig *config = parseAnimationConfig("configs.csv");
	AnimationFrames *frames = createAnimationFrames(config, sprite->size);
	AnimatedSprite *aSprite = createAnimatedSprite(sprite, frames, 4);
	freeAnimationConfig(config);

	Vector2 v = (Vector2) { .x = 0, .y = 0 };
	Vector2 pos = (Vector2) { .x = resolution.x / 2, .y = resolution.y / 2 };
	float deltaTime;

	RenderTexture2D canvas = LoadRenderTexture(resolution.x, resolution.y);
	Vector2 canvasPosition = (Vector2) { 0, 0 };
	Vector2 origin = (Vector2) { 0, 0 };
	Rectangle source = (Rectangle) { 0, 0, resolution.x, -resolution.y };
	Rectangle dest = (Rectangle) { 0, 0, resolution.x, resolution.y };

	Shader shader = LoadShader("res/shaders/light.vs", "res/shaders/test.fs");
	SetShaderValue(shader, GetShaderLocation(shader, "resolution"), &resolution, SHADER_UNIFORM_VEC2); 
	Color background = (Color) { 150, 75, 50 };
	while(!WindowShouldClose()) {
		BeginTextureMode(canvas);

		ClearBackground(background);
		deltaTime = GetFrameTime();
		drawAnimatedSprite(aSprite, pos, deltaTime); 

		EndTextureMode();
		
		BeginDrawing();
		BeginShaderMode(shader);
		DrawTexturePro(canvas.texture, source, dest, origin, 0, WHITE);
		EndShaderMode();
		EndDrawing();

		if(IsKeyPressed(KEY_UP)) {
			switchAnimationType(aSprite, IDLE_U_ANIM);
			v.x =  0;
			v.y =  0;
		}
		else if(IsKeyPressed(KEY_DOWN)) {
			switchAnimationType(aSprite, IDLE_D_ANIM);
			v.x =  0;
			v.y =  0;
		}
		else if(IsKeyPressed(KEY_LEFT)) {
			switchAnimationType(aSprite, IDLE_L_ANIM);
			v.x =  0;
			v.y =  0;
		}
		else if(IsKeyPressed(KEY_RIGHT)) {
			switchAnimationType(aSprite, IDLE_R_ANIM);
			v.x =  0;
			v.y =  0;
		}
		else if(IsKeyPressed(KEY_W)) {
			switchAnimationType(aSprite, WALK_U_ANIM);
			v.x =  0;
			v.y = -3;
		}
		else if(IsKeyPressed(KEY_S)) {
			switchAnimationType(aSprite, WALK_D_ANIM);
			v.x =  0;
			v.y =  3;
		}
		else if(IsKeyPressed(KEY_A)) {
			switchAnimationType(aSprite, WALK_L_ANIM);
			v.x = -3;
			v.y =  0;
		}
		else if(IsKeyPressed(KEY_D)) {
			switchAnimationType(aSprite, WALK_R_ANIM);
			v.x =  3;
			v.y =  0;
		}

		pos.x += v.x * deltaTime * 10;
		pos.y += v.y * deltaTime * 10;
	}
	
	freeAnimatedSprite(aSprite);
	freeSprite(sprite);
	freeAnimationFrames(frames);

	CloseWindow();
	return 0;
}
