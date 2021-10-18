#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "graph.h"

unsigned int evictParents (unsigned int, graph*, graph*, unsigned int*, unsigned int*, unsigned int*, bool*, unsigned int*, unsigned int*, unsigned int*, unsigned int*, unsigned int);
unsigned int loadParents (unsigned int, graph*, graph*, unsigned int*, unsigned int*, unsigned int*, bool*, unsigned int*, unsigned int*, unsigned int*, unsigned int*, unsigned int, unsigned int);
void moveRight(unsigned int, unsigned int*, unsigned int*, unsigned int*, unsigned int*, unsigned int);
void moveLeft(unsigned int, unsigned int*, unsigned int*, unsigned int*, unsigned int*, unsigned int);
