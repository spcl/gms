#ifndef ABSTRACTIONOPTIMIZING_MINEBENCH_CLIQUECOUNTING_AUXIL_KCLIST_ORIG_H
#define ABSTRACTIONOPTIMIZING_MINEBENCH_CLIQUECOUNTING_AUXIL_KCLIST_ORIG_H

// --------------------------------------------------------------------------
// Information:
// -----------
// This is a modified copy of Danisch et al.'s serial k-clique listing code.
//
// It is used for verification of our implementation.
//
// Sources:
// -------
// Repository: https://github.com/maxdan94/kClist
// Original file: https://github.com/maxdan94/kClist/blob/master/kClist.c
// Associated publication: https://doi.org/10.1145/3178876.3186125
//
// Modifications to the original source:
// ------------------------------------
// - Wrap the code in GMS::KClique namespace.
// - Change NLINKS from macro to constexpr int.
// - Change unsigned to signed integers.
// - Some additional comments.
// - Refactor some code into multiple functions.
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Original copyright notice:
// -------------------------
// MIT License
//
// Copyright (c) 2018 Maximilien Danisch
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// --------------------------------------------------------------------------

/*

Info:
Feel free to use these lines as you wish.
This program iterates over all k-cliques.
This is an improvement of the 1985 algorithm of Chiba And Nishizeki detailed in "Arboricity and subgraph listing".

Will print the number of k-cliques.
*/


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

namespace GMS::KClique {

//maximum number of edges for memory allocation, will increase if needed
constexpr int NLINKS = 100000000;

typedef struct {
	int s;
	int t;
} edge;

typedef struct {
	int node;
	int deg;
} nodedeg ;


typedef struct {

	int n;//number of nodes
	int e;//number of edges
	edge *edges;//list of edges

	int *ns;//ns[l]: number of nodes in G_l
	int **d;//d[l]: degrees of G_l
	int *cd;//cumulative degree: (starts with 0) length=n+1
	int *adj;//truncated list of neighbors
	int *rank;//ranking of the nodes according to degeneracy ordering
	//int *map;//oldID newID correspondance

	char *lab;//lab[i] label of node i
	int **sub;//sub[l]: nodes in G_l

} specialsparse;


void freespecialsparse(specialsparse *g, int k)
{
	int i;
	free(g->ns);
	for (i=2;i<k+1;i++)
    {
		free(g->d[i]);
		free(g->sub[i]);
	}
	free(g->d);
	free(g->sub);
	free(g->cd);
	free(g->adj);
	free(g);
}

//Compute the maximum of three int integers.
inline int max3( int a, int b, int c)
{
	a=(a>b) ? a : b;
	return (a>c) ? a : c;
}

specialsparse* readedgelist(char* edgelist)
{
	int e1 = NLINKS;
	specialsparse* g = (specialsparse*)malloc(sizeof(specialsparse));
	FILE* file;

	g->n=0;
	g->e=0;
	file=fopen(edgelist,"r");
	g->edges=(edge*)malloc(e1*sizeof(edge));
	while (fscanf(file,"%i %i", &(g->edges[g->e].s), &(g->edges[g->e].t))==2) {//Add one edge
		g->n=max3(g->n,g->edges[g->e].s,g->edges[g->e].t);
		g->e++;
		if (g->e==e1) {
			e1+=NLINKS;
			g->edges=(edge*)realloc(g->edges,e1*sizeof(edge));
		}
	}
	fclose(file);
	g->n++;

	g->edges=(edge*)realloc(g->edges,g->e*sizeof(edge));

	return g;
}



void relabel(specialsparse *g){
	int i, source, target, tmp;

	for (i=0;i<g->e;i++) {
		source=g->rank[g->edges[i].s];
		target=g->rank[g->edges[i].t];
		if (source<target){
			tmp=source;
			source=target;
			target=tmp;
		}
		g->edges[i].s=source;
		g->edges[i].t=target;
	}

}

///// CORE ordering /////////////////////

typedef struct {
	int key;
	int value;
} keyvalue;

typedef struct {
	int n_max;	// max number of nodes.
	int n;	// number of nodes.
	int *pt;	// pointers to nodes.
	keyvalue *kv; // nodes.
} bheap;


bheap *construct(int n_max){
	int i;
	bheap *heap=(bheap*)malloc(sizeof(bheap));

	heap->n_max=n_max;
	heap->n=0;
	heap->pt=(int*)malloc(n_max*sizeof(int));
	for (i=0;i<n_max;i++) heap->pt[i]=-1;
	heap->kv=(keyvalue*) malloc(n_max*sizeof(keyvalue));
	return heap;
}

void swap(bheap *heap,int i, int j) {
	keyvalue kv_tmp=heap->kv[i];
	int pt_tmp=heap->pt[kv_tmp.key];
	heap->pt[heap->kv[i].key]=heap->pt[heap->kv[j].key];
	heap->kv[i]=heap->kv[j];
	heap->pt[heap->kv[j].key]=pt_tmp;
	heap->kv[j]=kv_tmp;
}

void bubble_up(bheap *heap,int i) {
	int j=(i-1)/2;
	while (i>0) {
		if (heap->kv[j].value>heap->kv[i].value) {
			swap(heap,i,j);
			i=j;
			j=(i-1)/2;
		}
		else break;
	}
}

void bubble_down(bheap *heap) {
	int i=0,j1=1,j2=2,j;
	while (j1<heap->n) {
		j=( (j2<heap->n) && (heap->kv[j2].value<heap->kv[j1].value) ) ? j2 : j1 ;
		if (heap->kv[j].value < heap->kv[i].value) {
			swap(heap,i,j);
			i=j;
			j1=2*i+1;
			j2=j1+1;
			continue;
		}
		break;
	}
}

void insert(bheap *heap,keyvalue kv){
	heap->pt[kv.key]=(heap->n)++;
	heap->kv[heap->n-1]=kv;
	bubble_up(heap,heap->n-1);
}

/**
 * @brief Decrease the value associated with key by one
 * and resort heap
 * 
 * @param heap 
 * @param key key of value that is decreased
 */
void update(bheap *heap,int key){
	int i=heap->pt[key];
	if (i!=-1){
		((heap->kv[i]).value)--;
		bubble_up(heap,i);
	}
}

keyvalue popmin(bheap *heap){
	keyvalue min=heap->kv[0];
	heap->pt[min.key]=-1;
	heap->kv[0]=heap->kv[--(heap->n)];
	heap->pt[heap->kv[0].key]=0;
	bubble_down(heap);
	return min;
}

/**
 * @brief Building the heap with (key, value) = (node,degree) for each node
 * 
 * @param n 
 * @param v 
 * @return bheap* 
 */
bheap* mkheap(int n,int *v){
	int i;
	keyvalue kv;
	bheap* heap=construct(n);
	for (i=0;i<n;i++){
		kv.key=i;
		kv.value=v[i];
		insert(heap,kv);
	}
	return heap;
}

void freeheap(bheap *heap){
	free(heap->pt);
	free(heap->kv);
	free(heap);
}

/**
 * @brief Computing core ordering (aka linkage by Matula & Beck)
 * 
 * @param g 
 */
void ord_core(specialsparse* g){
	int i,j,r=0,n=g->n;
	keyvalue kv;
	bheap *heap;

	int* d0   = (int*)calloc(g->n,sizeof(int));
	int* cd0  = (int*)malloc((g->n+1)*sizeof(int));
	int* adj0 = (int*)malloc(2*g->e*sizeof(int));
	// count degree of each node
	// undirected graph: each edge appears once -> regular degree
	// of node is counted
	for (i=0;i<g->e;i++) {
		d0[g->edges[i].s]++;
		d0[g->edges[i].t]++;
	}
	//cumulative degrees
	cd0[0]=0;
	for (i=1;i<g->n+1;i++) {
		cd0[i]=cd0[i-1]+d0[i-1];
		d0[i-1]=0;
	}
	// build adjacency lists
	// walk through all edges
	//
	// count again in- and outdegrees of nodes together
	for (i=0;i<g->e;i++) {
		// cum. degree of start point + counted degree so far of start point
		// is position in adjacency list
		adj0[ cd0[g->edges[i].s] + d0[ g->edges[i].s ]++ ]=g->edges[i].t;
		adj0[ cd0[g->edges[i].t] + d0[ g->edges[i].t ]++ ]=g->edges[i].s;
	}

	// heap out of degree of nodes
	heap=mkheap(n,d0);

	g->rank=(int*)malloc(g->n*sizeof(int));
	// order nodes according to degree
	// matula beck -> smallest last
	for (i=0;i<g->n;i++){
		kv=popmin(heap);
		// insert current min at end of array
		// end shrinks to front
		// save the rank of the current node
		g->rank[kv.key]=n-(++r);
		for (j=cd0[kv.key];j<cd0[kv.key+1];j++){
			// reduce the level of each node adjacent to the current node
			// by one and resort heap
			update(heap,adj0[j]);
		}
	}
	freeheap(heap);
	free(d0);
	free(cd0);
	free(adj0);
}

//////////////////////////
//Building the special graph structure
void mkspecial(specialsparse *g, int k){
	int i,ns,max;
	int *d,*sub;
	char *lab;

	d=(int*)calloc(g->n,sizeof(int));

	for (i=0;i<g->e;i++) {
		d[g->edges[i].s]++;
	}

	g->cd=(int*)malloc((g->n+1)*sizeof(int));
	ns=0;
	g->cd[0]=0;
	max=0;
	sub=(int*)malloc(g->n*sizeof(int));
	lab=(char*)malloc(g->n*sizeof(char));
	for (i=1;i<g->n+1;i++) {
		g->cd[i]=g->cd[i-1]+d[i-1];
		max=(max>d[i-1])?max:d[i-1];
		sub[ns++]=i-1;
		d[i-1]=0;
		lab[i-1]=k;
	}
	printf("max degree = %u\n",max);

	g->adj=(int*)malloc(g->e*sizeof(int));

	for (i=0;i<g->e;i++) {
		g->adj[ g->cd[g->edges[i].s] + d[ g->edges[i].s ]++ ]=g->edges[i].t;
	}
	free(g->edges);

	g->ns=(int*)malloc((k+1)*sizeof(int));
	g->ns[k]=ns;

	g->d=(int**)malloc((k+1)*sizeof(int*));
	g->sub=(int**)malloc((k+1)*sizeof(int*));
	for (i=2;i<k;i++){
		g->d[i]=(int*)malloc(g->n*sizeof(int));
		g->sub[i]=(int*)malloc(max*sizeof(int));
	}
	g->d[k]=d;
	g->sub[k]=sub;

	g->lab=lab;
}

void doCounting(specialsparse* g, int long long* n)
{
    int i,j,end,u;
    for(i=0; i<g->ns[2]; i++){//list all edges
        u=g->sub[2][i];
        //(*n)+=g->d[2][u];
        end=g->cd[u]+g->d[2][u];
        for (j=g->cd[u];j<end;j++) {
            (*n)++;//listing here!!!  // NOTE THAT WE COULD DO (*n)+=g->d[2][u] to be much faster (for counting only); !!!!!!!!!!!!!!!!!!
        }
    }
}

void buildSubGraph(specialsparse* g, int l, int i)
{
    int j, u, v, end;
    u=g->sub[l][i];
    //printf("%u %u\n",i,u);
    g->ns[l-1]=0;
    end=g->cd[u]+g->d[l][u];
    for (j=g->cd[u];j<end;j++){//relabeling nodes and forming U'.
        v=g->adj[j];
        if (g->lab[v]==l){
            g->lab[v]=l-1;
            g->sub[l-1][g->ns[l-1]++]=v;
            g->d[l-1][v]=0;//new degrees
        }
    }
}

void orderAndCount(specialsparse* g, int l)
{
    int j, v, w, end, k;
    for (j=0;j<g->ns[l-1];j++){//reodering adjacency list and computing new degrees
        v=g->sub[l-1][j];
        end=g->cd[v]+g->d[l][v];
        for (k=g->cd[v];k<end;k++){
            w=g->adj[k];
            if (g->lab[w]==l-1){
                g->d[l-1][v]++;
            }
            else{
                g->adj[k--]=g->adj[--end];
                g->adj[end]=w;
            }
        }
    }
}

void restoreLabels(specialsparse* g, int l)
{
    int j, v;
    for (j=0;j<g->ns[l-1];j++){//restoring labels
        v=g->sub[l-1][j];
        g->lab[v]=l;
	}
}


void kclique(int l, specialsparse *g, long long *n) {
	int i;

	if(l==2){
		doCounting(g, n);
		return;
	}

	for(i=0; i<g->ns[l]; i++){
		buildSubGraph(g, l, i);
        orderAndCount(g, l);

		kclique(l-1, g, n);

        restoreLabels(g, l);
	}
}

}

#endif
