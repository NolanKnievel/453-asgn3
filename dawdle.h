#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef DAWDLEFACTOR
#define DAWDLEFACTOR 1000
#endif


void dawdle();