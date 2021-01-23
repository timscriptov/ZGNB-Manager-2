#ifndef __ARRAY_H
#define __ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct Array Array;

Array *arrayCreate();

void arrayFree(Array *array);

int arrayAdd(Array *array, void *pointer);

void *arrayGet(Array *array, int index);

void *arrayRemove(Array *array, int index);

void *arraySet(Array *array, int index, void *pointer);

int arraySetSize(Array *array, int size);

int arraySize(Array *array);

const void **arrayUnwrap(Array *array);

#ifdef __cplusplus
}
#endif

#endif
