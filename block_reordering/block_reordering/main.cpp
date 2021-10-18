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
//#undef DEBUG


using namespace std;

//defaults
unsigned int CLSize = 20;
unsigned int CACHE_SIZE = 100000;


void merge(unsigned int*, unsigned int*, unsigned int, unsigned int, unsigned int);

void mergeSort (unsigned int*, unsigned int*, unsigned int, unsigned int); 

int main(int argc, char** argv)
{
    
    if (argc == 5)
    {
        CLSize = atoi(argv[1]);
        CACHE_SIZE = atoi(argv[2]);
    }
    else if (argc != 3)
    {
        printf("Usage : %s <cacheLineSize>(optional) <cacheCapacity>(optional) <inputFile1> <outputFile> \n", argv[0]);
        exit(1);
    }

    // graph objects
    graph G1, G2;

    // read csr file
    if (read_csr(argv[argc-2], &G1)==-1)
        exit(1);

#ifdef DEBUG
    printf("file reading done. Num vertices = %d and numEdges = %d\n", G1.numVertex, G1.numEdges);
#endif


    unsigned int degThresh = (G1.numVertex) + 1;
//    printGraph(&G1);
    
    graph compG;
    compG.numVertex = (G1.numVertex-1)/CLSize + 1;
    compG.numEdges = G1.numEdges; 
    compG.VI = new unsigned int [compG.numVertex]();
    compG.EI = new unsigned int [compG.numEdges]();
    compG.numEdges = 0; 

    for (unsigned int i=0; i<compG.numVertex; i++)
    {
        unsigned int start = G1.VI[i*CLSize];
        unsigned int end = (i==(compG.numVertex-1)) ? G1.numEdges : G1.VI[(i+1)*CLSize];
        mergeSortWOkey(G1.EI, start, end-1); 
        
        if (i < compG.numVertex-1)
            compG.VI[i+1] = compG.VI[i];
        if (start < end)
        {
            compG.EI[compG.numEdges++] = G1.EI[start];
            if (i < compG.numVertex-1)
                compG.VI[i+1]++;
        }
        for (unsigned int j=start+1; j<end; j++)
        {
            if ((G1.EI[j]) != (G1.EI[j-1]))
            {
                compG.EI[compG.numEdges++] = G1.EI[j];
                if (i < compG.numVertex-1)
                    compG.VI[i+1]++;
            }
        }
    }

    
//    freeMem(&G1);
#ifdef DEBUG
    printf("new number of vertices are %d\n", compG.numVertex);
    printf("new number of edges are %d\n", compG.numEdges);
#endif




    unsigned int* inDeg = new unsigned int [compG.numVertex];
    for (unsigned int i=0; i<compG.numVertex-1; i++)
        inDeg[i] = compG.VI[i+1] - compG.VI[i];
    inDeg[compG.numVertex-1] = compG.numEdges - compG.VI[compG.numVertex-1];


    // another csr that stores edges in reverse direction
    // to access children of a node
	cout << "G1 Number of vertices: " << G1.numVertex << endl;
    createReverseCSR(&compG, &G2, G1.numVertex);
    unsigned int* outDeg = new unsigned int [G2.numVertex];
    for (unsigned int i=0; i<G2.numVertex-1; i++)
        outDeg[i] = G2.VI[i+1] - G2.VI[i];
    outDeg[G2.numVertex-1] = G2.numEdges - G2.VI[G2.numVertex-1];


    //initialize the cost array by in degree of each vertex
    unsigned int* cost = new unsigned int [compG.numVertex]();
    unsigned int maxCost = 0;
    for (unsigned int i=0; i<compG.numVertex; i++)
    {
        unsigned int endId = (i==compG.numVertex-1) ? compG.numEdges : compG.VI[i+1];
        cost[i] = endId - compG.VI[i];
        if (cost[i] > maxCost)
            maxCost = cost[i];
    }

    unsigned int* updates = new unsigned int [G1.numVertex]();


    // nodeIds keep track of what was the original position of a node in the vertex array
    unsigned int* nodeId = new unsigned int [compG.numVertex];
    for (unsigned int i=0; i<compG.numVertex; i++)
        nodeId[i] = i;

#ifdef DEBUG
    printf("sorting begin\n");
#endif
   
    mergeSort(cost, nodeId, 0, compG.numVertex-1);

    for (unsigned int i=0; i<compG.numVertex; i++)
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
    unsigned int* nodeMap = new unsigned int [compG.numVertex];
    for (unsigned int i=0; i<compG.numVertex; i++)
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
    unsigned int* cachePresence = new unsigned int [G2.numVertex]();

    //array to tell if a node is already processed
    //cost of these nodes shouldn't be updated and they
    //shouldn't be bought back unsigned into processing
    bool* isPlaced = new bool [compG.numVertex]();
    //array to store the reordered nodeIds
    // place the lowest cost node first
    unsigned int minPtr = 0;
    


    while(minPtr < compG.numVertex)
    {
        unsigned int origId = nodeId[minPtr];
        unsigned int nodeCost = inDeg[origId] - (maxCost - cost[minPtr]);
        while(updates[origId] > 0)
        {
            updates[origId]--;
            moveRight(origId, cost, nodeMap, nodeId, CI); 
            origId = nodeId[minPtr];
        }
        if (inDeg[origId] > CACHE_SIZE)
        {
        //special case, handle separately
            isPlaced[origId] = true;
            minPtr++;
            continue;
        }
        while((currWinCost + nodeCost > CACHE_SIZE) && (currWinStartId < minPtr-3))
        {
            //evict
            if (inDeg[nodeId[currWinStartId]] <= CACHE_SIZE)
                currWinCost -= evictParents(currWinStartId, &compG, &G2, CI, cost, cachePresence, isPlaced, nodeId, nodeMap, outDeg, updates);
            origId = nodeId[minPtr];
            while(updates[origId] > 0)
            {
                updates[origId]--;
                moveRight(origId, cost, nodeMap, nodeId, CI); 
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
            moveRight(origId, cost, nodeMap, nodeId, CI); 
            origId = nodeId[minPtr];
        }
        if (nodeCost < 0)
            printf("deg = %d, node = %d, nodecost = %d, cost=%d, updates left = %d\n",inDeg[origId], minPtr, nodeCost, cost[minPtr], updates[origId]); 
            
        //special case, handle separately
        isPlaced[origId] = true;
        //put parents in cache
        currWinCost += loadParents(minPtr, &compG, &G2, CI, cost, cachePresence, isPlaced, nodeId, nodeMap, outDeg, updates, minPtr);
        minPtr++;
    }
    //there is no reordering if only 2 vertices are left
    //who are already sorted on cost
    isPlaced[nodeId[compG.numVertex-2]] = true;
    isPlaced[nodeId[compG.numVertex-1]] = true;

    end = clock();

//    printf("Total time taken for reordering %s is %lf seconds\n", argv[argc-1], (double)(end-start)/CLOCKS_PER_SEC);
    printf("%s, %lf \n", argv[argc-1], (double)(end-start)/CLOCKS_PER_SEC);

    for (unsigned int i=0; i<compG.numVertex; i++)
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

    //// apply the new order to existing graph 
    //// new reordered graph
    freeMem(&G2);

    freeMem(&G1);
    freeMem(&compG);
    delete[] cost;
    delete[] nodeMap;
    delete[] CI;
    delete[] updates;
    delete[] cachePresence;
    delete[] isPlaced;

    if (read_csr(argv[argc-2], &G1)==-1)
        exit(1);

	cout << "G1 number of vertices: " << G1.numVertex << endl;
    unsigned int* newNodeId = new unsigned int [G1.numVertex];
    unsigned int* newNodeMap = new unsigned int [G1.numVertex];
    isPlaced = new bool [G1.numVertex]();

    unsigned int nodeExpCount = 0;
    for (unsigned int i=0; i<compG.numVertex; i++)
    {
        unsigned int clId = nodeId[i]; 
        unsigned int baseNode = clId*CLSize;
        unsigned int numNodes = (clId == (compG.numVertex-1)) ? G1.numVertex - baseNode : CLSize;
        for (unsigned int j=0; j<numNodes; j++)
            newNodeId[nodeExpCount++] = baseNode + j;
    }
    for (unsigned int i=0; i<G1.numVertex; i++)
    {
        newNodeMap[newNodeId[i]] = i;
    }

    G2.numVertex = G1.numVertex;
	cout << "G2 number of vertex: " << G2.numVertex << endl;
    G2.numEdges = G1.numEdges;
    G2.VI = new unsigned int[G2.numVertex];
    G2.EI = new unsigned int[G2.numEdges];
    G2.VI[0] = 0;
    for (unsigned int i=1; i<G2.numVertex; i++)
    {
        unsigned int prevNodeId = newNodeId[i-1];
        unsigned int parentStartId = G1.VI[prevNodeId]; 
        unsigned int parentEndId = (prevNodeId == G1.numVertex-1) ? G1.numEdges : G1.VI[prevNodeId+1];
        G2.VI[i] = G2.VI[i-1] + (parentEndId - parentStartId);
        unsigned int tempId = 0;
        for (unsigned int j=G2.VI[i-1]; j<G2.VI[i]; j++)
        {
            G2.EI[j] = newNodeMap[G1.EI[parentStartId + tempId]];
            tempId++;
        }
    }
    
    unsigned int prevNodeId = newNodeId[G2.numVertex-1];
    unsigned int parentStartId = G1.VI[prevNodeId];
    unsigned int tempId = 0;
    for (unsigned int i=G2.VI[G2.numVertex-1]; i<G2.numEdges; i++)
    {
       G2.EI[i] = newNodeMap[G1.EI[parentStartId + tempId]];
       tempId++; 
    }

#ifdef DEBUG
    printf("reordering applied\n");
#endif


//    write_csr(argv[argc-1], &G2);
	  write_edgelist(argv[argc-1],&G2);
//    FILE* fMap = fopen("fMap.bin", "wb");
//    fwrite(newNodeId, sizeof(unsigned int), G2.numVertex, fMap);
//    fclose(fMap);
	  FILE* fmap = fopen("new_order.el", "w");
	 for(int i=0; i < G2.numVertex; i++) fprintf(fmap, "%d\n", newNodeMap[i]);
	 fclose(fmap);

    freeMem(&G1);
    freeMem(&G2);
    delete[] nodeId;
    delete[] isPlaced;
    delete[] newNodeId;
    delete[] newNodeMap;

    return 0;
}


