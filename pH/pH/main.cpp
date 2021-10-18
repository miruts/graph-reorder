#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include "sort.h"
#include "cachefunc.h"
#ifndef GRAPH_HEADER_INCL
#include "graph.h"
#endif

#define DEBUG 1
#undef DEBUG

unsigned int CACHE_SIZE = 100000;

using namespace std;

void dumpNewOrder (graph*, unsigned int*, char*);


int main(int argc, char** argv)
{

    if (argc == 4)
    {
        CACHE_SIZE = atoi(argv[1]);
    }
    else if (argc != 3)
    {
        printf("Usage : %s <cacheCapacity(optional)> <inputFile1> <outputFile>\n", argv[0]);
        exit(1);
    }

    // graph objects
    graph G1, G2;

    // read csr file
    if (read_csr(argv[argc-2], &G1)==-1)
        exit(1);

    unsigned int degThresh = sqrt(G1.numVertex);
//    unsigned int degThresh = G1.numVertex + 1;

    unsigned int* inDeg = new unsigned int [G1.numVertex];
    for (unsigned int i=0; i<G1.numVertex-1; i++)
        inDeg[i] = G1.VI[i+1] - G1.VI[i];
    inDeg[G1.numVertex-1] = G1.numEdges - G1.VI[G1.numVertex-1];

#ifdef DEBUG
    printf("file reading done. Num vertices = %d and numEdges = %d\n", G1.numVertex, G1.numEdges);
#endif

    // another csr that stores edges in reverse direction
    // to access children of a node
    createReverseCSR(&G1, &G2);

    unsigned int* outDeg = new unsigned int [G1.numVertex];
    for (unsigned int i=0; i<G2.numVertex-1; i++)
        outDeg[i] = G2.VI[i+1] - G2.VI[i];
    outDeg[G2.numVertex-1] = G2.numEdges - G2.VI[G2.numVertex-1];



    //initialize the cost array by in degree of each vertex
    unsigned int* cost = new unsigned int [G1.numVertex]();
    unsigned int maxCost = 0;
    for (unsigned int i=0; i<G1.numVertex; i++)
    {
        unsigned int endId = G1.VI[i+1];
        for (unsigned int j=G1.VI[i]; j<endId; j++)
        {
            if (outDeg[G1.EI[j]] <= degThresh)
                cost[i]++; 
        }
        if (cost[i] > maxCost)
            maxCost = cost[i];
    }

    unsigned int* updates = new unsigned int [G1.numVertex]();


    // nodeIds keep track of what was the original position of a node in the vertex array
    unsigned int* nodeId = new unsigned int [G1.numVertex];
    for (unsigned int i=0; i<G1.numVertex; i++)
        nodeId[i] = i;
   
    mergeSort(cost, nodeId, 0, G1.numVertex-1);


    for (unsigned int i=0; i<G1.numVertex; i++)
        cost[i] = maxCost;

    
    

#ifdef DEBUG
    printf("sorting done\n");
#endif




    clock_t start, end;
    start = clock();
    // CI -> array to keep pounsigned inters of start and end positions of a particular value in cost array
    // eg. cost array = [0, 0, 1, 1, 3, 4, 4, 7]
    // then CI = [0, 2, 4, 4, 5, 7, 7, 7]
    // this array will be used in updating cost array in const time so that it always remains sorted
    unsigned int* CI = new unsigned int [maxCost+1];
    unsigned int val = 0; 
    CI[0] = 0;
    unsigned int myId = 0;
    while(val < maxCost)
    {
        while(cost[myId] == val)
            myId++;
        while(cost[myId] != val)
            CI[++val] = myId;
        myId++;
    }
 
#ifdef DEBUG
    printf("cache model created\n");
#endif
        
    // nodeMap gives the new position of a vertex in the node array
    unsigned int* nodeMap = new unsigned int [G1.numVertex];
    for (unsigned int i=0; i<G1.numVertex; i++)
        nodeMap[nodeId[i]] = i;

#ifdef DEBUG
    printf("node mapping computed\n");
#endif

    //keeps track of number of nodes in cache
    //if greater than cache size, need to evict
    unsigned int currWinCost = 0;
    //keeps track of oldest node whose parents 
    //are still in cache
    unsigned int currWinStartId = 0;
    unsigned int* cachePresence = new unsigned int [G1.numVertex]();

    //array to tell if a node is already processed
    //cost of these nodes shouldn't be updated and they
    //shouldn't be bought back unsigned into processing
    bool* isPlaced = new bool [G1.numVertex]();
    //array to store the reordered nodeIds
    // place the lowest cost node first
    unsigned int minPtr = 0;
    


    while(minPtr < G1.numVertex)
    {        
        if (minPtr >= G1.numVertex-2)
            break; 
        unsigned int origId = nodeId[minPtr];
        unsigned int nodeCost = inDeg[origId] - (maxCost - cost[minPtr]);
        while(updates[origId] > 0)
        {
            updates[origId]--;
            moveRight(origId, cost, nodeMap, nodeId, CI, minPtr); 
            origId = nodeId[minPtr];
        }
        if (inDeg[origId] > CACHE_SIZE)
        {
        //special case, handle separately
            isPlaced[origId] = true;
            minPtr++;
            continue;
        }
        while((currWinCost + nodeCost > CACHE_SIZE) && (currWinStartId < minPtr-5))
        {
            //evict
            if (inDeg[nodeId[currWinStartId]] <= CACHE_SIZE)
                currWinCost -= evictParents(currWinStartId, &G1, &G2, CI, cost, cachePresence, isPlaced, nodeId, nodeMap, outDeg, updates, degThresh);
            origId = nodeId[minPtr];
            while(updates[origId] > 0)
            {
                updates[origId]--;
                moveRight(origId, cost, nodeMap, nodeId, CI, minPtr); 
                origId = nodeId[minPtr];
            }
            nodeCost = inDeg[origId] - (maxCost - cost[minPtr]);
            currWinStartId++;
            if (inDeg[origId] > CACHE_SIZE)
            {
            //special case, handle separately
                break;
            }
        } 
        if (inDeg[origId] > CACHE_SIZE)
        {
            isPlaced[origId] = true;
            minPtr++;
            continue;
        }
        while(updates[origId] > 0)
        {
            updates[origId]--;
            moveRight(origId, cost, nodeMap, nodeId, CI, minPtr); 
            origId = nodeId[minPtr];
        }
        if (nodeCost < 0)
            printf("deg = %d, node = %d, nodecost = %d, cost=%d, updates left = %d\n",inDeg[origId], minPtr, nodeCost, cost[minPtr], updates[origId]); 

        isPlaced[origId] = true;
        //put parents in cache
        currWinCost += loadParents(minPtr, &G1, &G2, CI, cost, cachePresence, isPlaced, nodeId, nodeMap, outDeg, updates, degThresh, minPtr);
        minPtr++;
    }
    //there is no reordering if only 2 vertices are left
    //who are already sorted on cost
    isPlaced[nodeId[G1.numVertex-2]] = true;
    isPlaced[nodeId[G1.numVertex-1]] = true;

    end = clock();

//    printf("Total time taken for reordering %s is %lf seconds\n", argv[1], (double)(end-start)/CLOCKS_PER_SEC);
    printf("%s, %lf \n", argv[argc-1], (double)(end-start)/CLOCKS_PER_SEC);

    for (unsigned int i=0; i<G1.numVertex; i++)
    {
        if (!isPlaced[i])
        {
            printf("ERROR -> failed to place %d\n", i);
            break;
        }
    }

#ifdef DEBUG
    printf("reordering computed\n");
#endif

	dumpNewOrder(&G1, nodeMap, (char *)"ph/new_order.el");

    //// apply the new order to existing graph 
    //// new reordered graph
    G2.VI[0] = 0;
    for (unsigned int i=1; i<G2.numVertex; i++)
    {
        unsigned int prevNodeId = nodeId[i-1];
        unsigned int parentStartId = G1.VI[prevNodeId]; 
        unsigned int parentEndId = G1.VI[prevNodeId+1];
        G2.VI[i] = G2.VI[i-1] + (parentEndId - parentStartId);
        unsigned int tempId = 0;
        for (unsigned int j=G2.VI[i-1]; j<G2.VI[i]; j++)
        {
            G2.EI[j] = nodeMap[G1.EI[parentStartId + tempId]];
            tempId++;
        }
    }
    
    unsigned int prevNodeId = nodeId[G2.numVertex-1];
    unsigned int parentStartId = G1.VI[prevNodeId];
    unsigned int tempId = 0;
    for (unsigned int i=G2.VI[G2.numVertex-1]; i<G2.numEdges; i++)
    {
       G2.EI[i] = nodeMap[G1.EI[parentStartId + tempId]];
       tempId++; 
    }

#ifdef DEBUG
    printf("reordering applied\n");
#endif


//	  write_edgelist(argv[argc-1], &G2);
    write_edgelist(argv[argc-1], &G2);
    delete[] updates;
    delete[] nodeId;
    delete[] cost;
    delete[] nodeMap;
    delete[] CI;
    delete[] cachePresence;
    delete[] isPlaced;
    freeMem(&G1);
    freeMem(&G2);

    return 0;
}

void dumpNewOrder (graph* G, unsigned int* nodeMap, char* fileName)
{
	FILE* fp = fopen(fileName, "w");
//	fwrite(nodeMap, sizeof(unsigned int), G->numVertex, fp);
	for (int i=0; i<G->numVertex; i++)
		fprintf(fp, "%d\n", nodeMap[i]);
	fclose(fp);
}
