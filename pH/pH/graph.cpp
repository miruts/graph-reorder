#include "graph.h"

using namespace std;

int read_edge_list (char* filename, std::vector<unsigned int>& src, std::vector<unsigned int>& dst)
{
    unsigned int numEdgesRead = 0;

    FILE* fp = fopen (filename, "r");
    if (fp == NULL)
    {
        fputs("file error", stderr);
        return -1;
    }

    unsigned int srcVal = 0;
    unsigned int dstVal = 0;

    int numVertex = 0;

    while(!feof(fp))
    {
        if(fscanf(fp, "%d", &srcVal) <= 0)
            break;
        fscanf(fp, "%d", &dstVal);
        numVertex = (srcVal > numVertex) ? srcVal : numVertex;
        numVertex = (dstVal > numVertex) ? dstVal : numVertex;
		if (srcVal != dstVal) {
            src.push_back(srcVal);
            dst.push_back(dstVal);
            numEdgesRead++;
		}
    }

    fclose(fp);
    numVertex++;

    return numVertex;
}

unsigned int filter (std::vector<unsigned int>& src, std::vector<unsigned int>& dst, unsigned int numVertex, unsigned int numEdgesRead)
{
    bool* exists = new bool [numVertex]();
    for (unsigned int i=0; i<numEdgesRead; i++)
    {
        exists[src[i]]=true;
        exists[dst[i]]=true;
    }

    unsigned int* vertexMap = new unsigned int [numVertex]();
    unsigned int actualVertices = 0;
    for (unsigned int i=0; i<numVertex; i++)
    {
        if (exists[i])
            vertexMap[i] = actualVertices++;
    }
    
    for (unsigned int i=0; i<numEdgesRead; i++)
    {
        src[i] = vertexMap[src[i]];
        dst[i] = vertexMap[dst[i]];
    }
    
    delete[] exists;
    delete[] vertexMap;
    
    return actualVertices;
}

void csr_convert(std::vector<unsigned int>& src, std::vector<unsigned int>& dst, graph* G)
{
    G->VI = new unsigned int [G->numVertex+1]();
    G->EI = new unsigned int [G->numEdges]();  

    unsigned int* inDeg = new unsigned int [G->numVertex]();
    for (unsigned int i=0; i<G->numEdges; i++)
        inDeg[src[i]]++;
    
    for (unsigned int i=1; i<G->numVertex+1; i++)
        G->VI[i] = G->VI[i-1] + inDeg[i-1];

    for (unsigned int i=0; i<G->numVertex; i++)
        inDeg[i] = 0;

    for (unsigned int i=0; i<G->numEdges; i++)
    {
        G->EI[G->VI[src[i]] + inDeg[src[i]]] = dst[i];
        inDeg[src[i]]++;
    }

    delete[] inDeg;
}

int read_csr (char* filename, graph* G)
{
    std::vector<unsigned int> src;
    std::vector<unsigned int> dst; 

    int numVertex = read_edge_list (filename, src, dst);
    if (numVertex < 0)
        return -1;

    G->numEdges = src.size();

    G->numVertex = filter(src, dst, (unsigned int)numVertex, G->numEdges);


    csr_convert(src, dst, G);


    return 1;
}


void printGraph (graph* G)
{
    //for (unsigned int i=0; i<G->numVertex; i++)
    //    printf("%d ", i);
    //printf("\n");
    for (unsigned int i=0; i<G->numVertex; i++)
    {
        printf("%d -> ", i);
        unsigned int endId = (i>=G->numVertex-1) ? G->numEdges : G->VI[i+1];
        for (unsigned int j=G->VI[i]; j<endId; j++)
            printf("%d, ", G->EI[j]);
        printf("\n");
    }
    printf("\n");
    //for (unsigned int i=0; i<G->numEdges; i++)
    //    printf("%d ", i);
    //printf("\n");
//    for (unsigned int i=0; i<G->numEdges; i++)
//        printf("%d ", G->EI[i]);
    printf("\n");
}


void write_csr (char* filename, graph* G)
{
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        fputs("file error", stderr);
        return;
    }
    fwrite(&G->numVertex, sizeof(unsigned int), 1, fp); 
    fwrite(&G->numEdges, sizeof(unsigned int), 1, fp); 
    fwrite(G->VI, sizeof(unsigned int), G->numVertex, fp); 
    fwrite(G->EI, sizeof(unsigned int), G->numEdges, fp); 
    fclose(fp); 
}
void write_edgelist(char* filename, graph * G) {
	unsigned int * VI = G->VI;
	unsigned int * EI = G->EI;
	FILE * fp;
	fp = fopen(filename, "w");
	if (fp == NULL) {
		fputs("File error\n", stderr);
	}
	for(unsigned int i=0; i <G->numVertex-1; ++i) {
		for(unsigned int j=VI[i]; j <VI[i+1]; ++j) fprintf(fp, "%d %d\n", i, EI[j]);
	}
	for(unsigned int j=VI[G->numVertex-1]; j < G->numEdges; ++j) fprintf(fp, "%d %d\n", G->numVertex-1, EI[j]);
}
void createReverseCSR(graph* G1, graph* G2)
{
    G2->numVertex = G1->numVertex;
    G2->numEdges = G1->numEdges;
    G2->VI = new unsigned int[G1->numVertex]();
    G2->EI = new unsigned int[G1->numEdges]; 

    for (unsigned int i=0; i<G1->numEdges; i++)
    {
        if (G1->EI[i] < G1->numVertex-1)
            G2->VI[G1->EI[i]+1]++;
    }
    for (unsigned int i=1; i<G1->numVertex; i++)
        G2->VI[i] += G2->VI[i-1];
    unsigned int* tempId = new unsigned int [G1->numVertex]();
    for (unsigned int i=0; i<G1->numVertex; i++)
    {
        unsigned int maxId = (i==G1->numVertex-1) ? G1->numEdges : G1->VI[i+1];
        for (unsigned int j=G1->VI[i]; j<maxId; j++)
        {
            G2->EI[G2->VI[G1->EI[j]] + tempId[G1->EI[j]]] = i;
            tempId[G1->EI[j]]++;
        } 
    }
    delete[] tempId;
}


void freeMem (graph* G)
{
    delete[] G->VI;
    delete[] G->EI;
}

