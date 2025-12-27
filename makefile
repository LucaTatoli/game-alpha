CC := $(shell command -v gcc || command -v cc)

ifeq ($(CC),)
	$(error No compiler found! Either gcc or cc is needed!)
endif

FLAGS := -Wall -pedantic
LIBS := raylib/src/libraylib.a -lm
CUSTOM_LIBS := obj/utils.o obj/sprite.o obj/physics.o

alpha: alpha.c sprite.o physics.o
	$(CC) -DISOMETRIC $(FLAGS) alpha.c $(LIBS) $(CUSTOM_LIBS) -o alpha

alpha_3rdp: alpha.c sprite.o physics.o
	$(CC) $(FLAGS) alpha.c $(LIBS) $(CUSTOM_LIBS) -o alpha

sprite.o: sprite.c utils.o
	$(CC) -c sprite.c obj/utils.o -o obj/sprite.o

utils.o: utils.c
	$(CC) -c utils.c -o obj/utils.o

physics.o: physics.c
	$(CC) -c physics.c -o obj/physics.o
