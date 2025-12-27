#ifndef SPRITE_H
#define SPRITE_H

#include "raylib/src/raylib.h"


#define ANIMATION_COUNT 8

/* This enum represents the indexes for all the possible animations for an animated sprite.
 * Each aniamted sprite has an array of AnimationFrames, each element is a specific animation.  
 * The position in the array is fixed, using this enum as index to access the animations */
typedef enum {
	IDLE_U_ANIM,
	IDLE_R_ANIM,
	IDLE_D_ANIM,
	IDLE_L_ANIM,
	WALK_U_ANIM,
	WALK_R_ANIM,
	WALK_D_ANIM,
	WALK_L_ANIM 
} AnimationType;

typedef struct Sprite {
	Texture2D texture;
	int size;
	int referenceCount;
} Sprite;

/* This struct stores the positions of the frames of an AnimatedSprite animation, keeping
 * track of how many frames this animation has */
typedef struct AnimationFrames {
	Rectangle *frames;
	int count;
	int referenceCount;
} AnimationFrames;

typedef struct AnimatedSprite {
	Sprite *sprite;
	int fps;
	float frameTimer;
	int currFrame;
	AnimationFrames *animationFrames;
	AnimationType currAnimation;
} AnimatedSprite;

/* This struct represents the configuration for a single animation, stored after parsing
 * the configuration file. 
 * 'type' labels the animation (e.g. 0: IDLE_U_ANIM)
 * 'size' is the sprite size
 * 'count' is the number of animation frames
 * 'x' and 'y' are arrays of animation starting coordinates */
typedef struct AnimationConfig {
	int type;
	int count;
	int *x;
	int *y;
} AnimationConfig;

/* Creates a Sprite pointer using the texture located at 'path' with 'size' size */
Sprite *createSprite(char *path, int size);

/* Creates an AnimatedSprite using the 's' as Sprite and 'frames' as animation frames. The caller
 * has to set 'fps' at which the animation will run */
AnimatedSprite *createAnimatedSprite(Sprite *s, AnimationFrames *frames, int fps);

/* Creates the animations frames using the configuration provided inside 'configs' */
AnimationFrames *createAnimationFrames(AnimationConfig *configs, int size);

/* Parses the animation config file at 'path' and returns an array of AnimationConfig containing all 
 * the data relative to the animation */
AnimationConfig* parseAnimationConfig(char *path);

/* Frees the memory pointed by 'sprite' */
void freeSprite(Sprite *sprite);

/* Increments the referenceCount associated with 'sprite', many entities may have the same sprite */
void retainSprite(Sprite *sprite);

/* Decrements the referenceCount associated with 'sprite', many entities may have the same sprite */
void releaseSprite(Sprite *sprite);

/* Frees the memory pointed by 'sprite' */
void freeAnimatedSprite(AnimatedSprite *sprite);

/* Frees the memory pointed by 'a' */
void freeAnimationFrames(AnimationFrames *a);

/* Increments the referenceCount associated with 'a', many AnimatedSprite may share the same AnimationFrames */
void retainAnimationFrames(AnimationFrames *a);

/* Decrements the referenceCount associated with 'a', many AnimatedSprite may share the same AnimationFrames */
void releaseAnimationFrames(AnimationFrames *a);

/* Frees the memory pointed by 'c' */
void freeAnimationConfig(AnimationConfig *c);

Rectangle *pickCurrentFrame(AnimatedSprite *a, float deltaTime);

/* Draws the correct animation frame for the animated sprite */
void drawAnimatedSprite(AnimatedSprite *a, Vector2 position, float deltaTime);

/* Draws the correct animation frame for the animated sprite as a billboard */
void drawAnimatedSpriteBillboard(AnimatedSprite *a, Camera camera, Vector3 position, Vector2 size, float deltaTime);

/* Changes the animation type of the AnimatedSprite 'a', this has to be called each time because frameTimer and currFrame
 * need to be refreshed every time the animation type is switched */
void switchAnimationType(AnimatedSprite *a, AnimationType type);

#endif
