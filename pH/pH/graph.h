#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

#define GRAPH_HEADER_INCL

typedef struct graph 
{
    unsigned int numVertex;
    unsigned int numEdges;
    unsigned int* VI;
    unsigned int* EI;
} graph;

int read_csr (char*, graph*);

void printGraph (graph*);

void write_csr (char*, graph*);
void write_edgelist(char*, graph*);
void initGraph (graph*);

void createReverseCSR (graph*, graph*);

void freeMem(graph*);

