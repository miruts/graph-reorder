#include "cachefunc.h"

void moveRight(unsigned int id, unsigned int* cost, unsigned int* nodeMap, unsigned int* nodeId, unsigned int* CI)
{
    unsigned int nodeCost = cost[nodeMap[id]];
    unsigned int temp = nodeMap[id];
    unsigned int replacedNode = nodeId[CI[nodeCost+1]-1];
    nodeMap[id] = CI[nodeCost+1]-1;
    nodeMap[replacedNode] = temp;
    nodeId[CI[nodeCost+1]-1] = id;
    nodeId[temp] = replacedNode;
    cost[CI[nodeCost+1]-1]++;
    CI[nodeCost+1]--;
}

void moveLeft(unsigned int id, unsigned int* cost, unsigned int* nodeMap, unsigned int* nodeId, unsigned int* CI, unsigned int minPtr)
{
    unsigned int nodeCost = cost[nodeMap[id]];
    unsigned int temp = nodeMap[id];
    if (CI[nodeCost] <= minPtr)
        CI[nodeCost] = minPtr+1;
    unsigned int replacedNode = nodeId[CI[nodeCost]];
    nodeMap[id] = CI[nodeCost];
    nodeId[CI[nodeCost]] = id;
    nodeMap[replacedNode] = temp;
    nodeId[temp] = replacedNode;
    cost[CI[nodeCost]]--;
    if (CI[nodeCost-1] <= minPtr)
        CI[nodeCost-1] = minPtr+1;
    CI[nodeCost]++;
}

unsigned int evictParents (unsigned int node, graph* G1, graph* G2, unsigned int* CI, unsigned int* cost, unsigned int* cachePresence, bool* isPlaced, unsigned int* nodeId, unsigned int* nodeMap, unsigned int* outDeg, unsigned int* updates)
{
    unsigned int numParentsEvicted = 0;
    unsigned int origId = nodeId[node];
    unsigned int parentStartId = G1->VI[origId];
    unsigned int parentEndId = (origId==(G1->numVertex-1)) ? G1->numEdges : G1->VI[origId+1];
    //for every parent
    for (unsigned int i=parentStartId; i<parentEndId; i++)
    {
        //find out if the parent should be evicted
        //if yes, evict and increase the cost of its children 
        unsigned int parentId = G1->EI[i];
        cachePresence[parentId]--;
        if (cachePresence[parentId] == 0)
        {
            //increase the cost of their children
            unsigned int childIdStart = G2->VI[parentId];
            unsigned int childIdEnd = (parentId==G2->numVertex-1) ? G2->numEdges : G2->VI[parentId+1];
            for (unsigned int j=childIdStart; j<childIdEnd; j++)
            {
                //update the new location after cost reduction
                unsigned int childId = G2->EI[j];
                if (!isPlaced[childId])
                    updates[childId]++;
                //if the child is not already placed in the order
                //process it, otherwise leave it
            }
            numParentsEvicted++;
        }
    }
    return numParentsEvicted;
} 


unsigned int loadParents (unsigned int nodePtr, graph* G1, graph* G2, unsigned int* CI, unsigned int* cost, unsigned int* cachePresence, bool* isPlaced, unsigned int* nodeId, unsigned int* nodeMap, unsigned int* outDeg, unsigned int*updates, unsigned int minPtr)
{
    unsigned int numParentsLoaded = 0;
    unsigned int origId = nodeId[nodePtr];
    unsigned int parentIdStart = G1->VI[origId];
    unsigned int parentIdEnd = (origId==(G1->numVertex-1)) ? G1->numEdges : G1->VI[origId+1];
    for (unsigned int i=parentIdStart; i<parentIdEnd; i++)
    {
        unsigned int parentId = G1->EI[i];
        cachePresence[parentId]++;

        //reduce the cost of their children
        unsigned int childIdStart = G2->VI[parentId];
        unsigned int childIdEnd = (parentId==G2->numVertex-1) ? G2->numEdges : G2->VI[parentId+1];
        if (cachePresence[parentId]==1) //this implies that element wasn't in cache before
        {
            for (unsigned int j=childIdStart; j<childIdEnd; j++)
            {
                //update the new location after cost reduction
                unsigned int childId = G2->EI[j];
                //if the child's processing order isn't yet fixed
                //update it, otherwise leave it
                if (!isPlaced[childId])
                {
                    if (updates[childId] > 0)
                        updates[childId]--;
                    else
                        moveLeft(childId, cost, nodeMap, nodeId, CI, minPtr);
                }
            }
            numParentsLoaded++;
        }
    }
    return numParentsLoaded;
}
