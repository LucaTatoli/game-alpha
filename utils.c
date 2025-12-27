#include "utils.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

void *xmalloc(size_t size) {
    void *p = malloc(size);
    if(p == NULL) {
        perror("ERROR enable to allocate more memory");
        exit(1);
    }
    return p;
}

void *xrealloc(void *p, size_t size) {
    void *new_p = realloc(p, size);
    if(new_p == NULL) {
        perror("ERROR enable to allocate more memory");
        exit(1);
    }
	return new_p;
}

