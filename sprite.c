#include "sprite.h"
#include "utils.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

Sprite *createSprite(char *path, int size) {
	Sprite *s = (Sprite*)xmalloc(sizeof(*s));
	Texture2D tex = LoadTexture(path);
	s->texture = tex;
	s->size = size;
	s->referenceCount = 1;
	return s;
}

void retainSprite(Sprite *s) {
	if(s == NULL || s->referenceCount <= 0) {
		fprintf(stderr, "ERROR trying to retain an invalid pointer\n");
		exit(1);
	}
	s->referenceCount++;
}

void releaseSprite(Sprite *s) {
	if(s == NULL || s->referenceCount <= 0) {
		fprintf(stderr, "ERROR trying to release an invalid pointer\n");
		exit(1);
	}
	s->referenceCount--;
	if(s->referenceCount == 0)
		freeSprite(s);
}

void freeSprite(Sprite *s) {
	if(s == NULL) {
		fprintf(stderr, "ERROR trying to free an invalid pointer\n");
		exit(1);
	}
	UnloadTexture(s->texture);
	free(s);
}

AnimatedSprite *createAnimatedSprite(Sprite *s, AnimationFrames *frames, int fps) {
	AnimatedSprite *a = (AnimatedSprite*)xmalloc(sizeof(*a));
	retainSprite(s);
	a->sprite = s;
	retainAnimationFrames(frames);
	a->animationFrames = frames;
	a->fps = fps;
	a->frameTimer = 0;
	a->currFrame = 0;
	a->currAnimation = IDLE_D_ANIM;
	return a;
}

void freeAnimatedSprite(AnimatedSprite *a) {
	if(a == NULL) {
		fprintf(stderr, "ERROR trying to free an invalid pointer\n");
		exit(1);
	}
	releaseSprite(a->sprite);
	releaseAnimationFrames(a->animationFrames);
	free(a);
}

AnimationFrames *createAnimationFrames(AnimationConfig *config, int size) {
	if(config == NULL) {
		fprintf(stderr, "ERROR creating an animation frames requires a valid configuration\n");
		exit(1);
	}
	AnimationFrames *a = (AnimationFrames*)xmalloc(sizeof(AnimationFrames) * ANIMATION_COUNT);

	for(int f = 0; f < ANIMATION_COUNT; f++) {
		a[f].frames = (Rectangle*)xmalloc(sizeof(Rectangle) * config[f].count);

		for(int i = 0; i < config[f].count; i++) {
			a[f].frames[i] = (Rectangle) {
				.x      = config[f].x[i],
				.y      = config[f].y[i],
				.width  = size,
				.height = size
			};
		}
		a[f].referenceCount = 1;
		a[f].count = config[f].count;
	}

	return a;
}

void retainAnimationFrames(AnimationFrames *a) {
	if(a == NULL || a->referenceCount <= 0) {
		fprintf(stderr, "ERROR trying to retain an invalid pointer\n");
		exit(1);
	}
	a->referenceCount++;
}

void releaseAnimationFrames(AnimationFrames *a) {
	if(a == NULL || a->referenceCount <= 0) {
		fprintf(stderr, "ERROR trying to release an invalid pointer\n");
		exit(1);
	}
	a->referenceCount--;
	if(a->referenceCount == 0)
		freeAnimationFrames(a);
}

void freeAnimationFrames(AnimationFrames *a) {
	if(a == NULL) {
		fprintf(stderr, "ERROR trying to free an invalid pointer\n");
		exit(1);
	}
	for(int i = 0; i < ANIMATION_COUNT; i++) {
		free(a[i].frames);
	}
	free(a);
}

int parseInteger(char *buff, size_t start, size_t end) {
	char *frames = (char*)xmalloc(sizeof(char)*(end - start)+1);
	memcpy(frames, buff + start, end - start);
	frames[end - start] = 0; // null terminator to avoid bugs with atoi
	int value = atoi(frames);
	free(frames);
	return value;
}

void parseColumn(AnimationConfig *a, char *buff, int column, size_t start, size_t end) {

	// the first column is always the animation type
	if     (column == 0) a->type = parseInteger(buff, start, end);

	// the second column is always the frames count
	else if(column == 1) {
		int frames = parseInteger(buff, start, end);
		a->count = frames;
		a->x = (int*)xmalloc(sizeof(int) * frames);
		a->y = (int*)xmalloc(sizeof(int) * frames);	
	}

	// the other columns are the coordinates of the frames inside the texture
	// from the column 2 each two successive columns are the (x, y) coordinates of the frame
	// N where N equals (column - 2) / 2 rounded down
	else if(column % 2 == 0) {
		int index = (column - 2) / 2;
		a->x[index] = parseInteger(buff, start, end);
	}

	else {
		int index = (column - 2) / 2;
		a->y[index] = parseInteger(buff, start, end);
	}

}


AnimationConfig *parseAnimationConfig(char *path) {
	FILE *f = fopen(path, "rb");
	if(f == NULL) {
		perror("ERROR unable to open the file");
		exit(1);
	}

	size_t buffSize = 1024;
	char *buff = (char*)xmalloc(buffSize);
	size_t readOffset = 0;
	size_t bytesRead = 0;
	
	do {
		bytesRead = fread(buff + readOffset, 1, buffSize - readOffset, f);
		readOffset += bytesRead;
		if(ferror(f)) {
			perror("ERROR unable to read the file!");
			free(buff);
			fclose(f);
			exit(1);
		} else if(readOffset == buffSize) {
			buffSize *= 2;
			buff = (char*)xrealloc(buff, buffSize);
		}
	} while(bytesRead > 0);

	buff[readOffset] = 0;
	fclose(f);

	AnimationConfig *configs = xmalloc(sizeof(AnimationConfig) * ANIMATION_COUNT);

	size_t cursorStart = 0;
	int column = 0;
	int row = 0;
	for(size_t i = 0; i <= readOffset; i++) {
		if(buff[i] == ',' || buff[i] == '\n' || buff[i] == 0) {
			parseColumn(&configs[row], buff, column, cursorStart, i);
			cursorStart = i+1;
			
			if(buff[i] == '\n') {
				column = 0;
				row++;
			} 

			else column++;
		}
	}
	
	free(buff);

	return configs;
}

Rectangle *pickCurrentFrame(AnimatedSprite *a, float deltaTime) {
	a->frameTimer += deltaTime;
	if(a->frameTimer > 1.0f / a->fps) {
		a->currFrame = (a->currFrame + 1) % a->animationFrames[a->currAnimation].count;
		a->frameTimer = 0;
	}

	return &a->animationFrames[a->currAnimation].frames[a->currFrame];
}

void drawAnimatedSprite(AnimatedSprite *a, Vector2 position, float deltaTime) {
	Rectangle *frameToDraw = pickCurrentFrame(a, deltaTime);
	DrawTextureRec(a->sprite->texture, *frameToDraw, position, WHITE);
}

Vector3 up = (Vector3) { 0.0f, 1.0f, 0.0f };

void drawAnimatedSpriteBillboard(AnimatedSprite *a, Camera camera, Vector3 position, Vector2 size, float deltaTime) {
	Rectangle *frameToDraw = pickCurrentFrame(a, deltaTime);
	//DrawBillboardRec(camera, a->sprite->texture, *frameToDraw, position, size, WHITE);
	Vector2 origin = (Vector2) { .x = size.x / 2, .y = size.y / 2 };
	DrawBillboardPro(camera, a->sprite->texture, *frameToDraw, position, up, size, origin, 0, WHITE);
}

void switchAnimationType(AnimatedSprite *a, AnimationType type) {
	if(a->currAnimation == type) return; // do not reset the animation if switching into the same
	a->currAnimation = type;
	a->frameTimer = 0;
	a->currFrame = 0;
}

void freeAnimationConfig(AnimationConfig *c) {
	for(int i = 0; i < ANIMATION_COUNT; i++) {
		free(c[i].x);
		free(c[i].y);
	}
	free(c);
}

