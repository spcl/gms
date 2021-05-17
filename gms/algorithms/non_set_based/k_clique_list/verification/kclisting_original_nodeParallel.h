// --------------------------------------------------------------------------
// Information:
// -----------
// This is a modified copy of Danisch et al.'s node-parallel k-clique listing code.
//
// It is used for verification of our implementation.
//
// Sources:
// -------
// Repository: https://github.com/maxdan94/kClist
// Original file: https://github.com/maxdan94/kClist/blob/master/kClistNodeParallel.c
// Associated publication: https://doi.org/10.1145/3178876.3186125
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

// --------------------------------------------------------------------------
// Info: This is a C++ port of Danisch et al.'s node-parallel k-clique listing code (DOI: 10.1145/3178876.3186125)
// It serves as verification.
//
// Original source: https://github.com/maxdan94/kClist
// --------------------------------------------------------------------------

/*
Info:
Feel free to use these lines as you wish.
This program iterates over all k-cliques.
This is an improvement of the 1985 algorithm of Chiba And Nishizeki detailed in "Arboricity and subgraph listing".

To compile:
"gcc kClistNodeParallel.c -O9 -o kClistNodeParallel -fopenmp".

To execute:
"./kClistNodeParallel p k edgelist.txt".
"edgelist.txt" should contain the graph: one edge on each line separated by a space.
k is the size of the k-cliques
p is the number of threads
Will print the number of k-cliques.
*/

#ifndef KCLIST_ORIG_SPLIT_NP_H
#define KCLIST_ORIG_SPLIT_NP_H


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#include <gms/common/types.h>

namespace GMS::KClique::NP
{
    //maximum number of edges for memory allocation, will increase if needed
    constexpr int NLINKS = 100000000;

    typedef struct {
        NodeId s;
        NodeId t;
    } edge;

    typedef struct {
        NodeId node;
        NodeId deg;
    } nodedeg ;

    typedef struct {
        NodeId n;//number of nodes
        NodeId e;//number of edges
        edge *edges;//list of edges
        NodeId *rank;//ranking of the nodes according to degeneracy ordering
        //NodeId *map;//oldID newvID correspondance NOT USED IN THIS VERSION
    } edgelist;

    typedef struct {
        NodeId n;
        NodeId *cd;//cumulative degree: (starts with 0) length=n+1
        NodeId *adj;//truncated list of neighbors
        long long core;//core value of the graph
    } graph;

    typedef struct {
        NodeId *n;//n[l]: number of nodes in G_l
        NodeId **d;//d[l]: degrees of G_l
        NodeId *adj;//truncated list of neighbors
        NodeId *lab;//lab[i] label of node i
        NodeId **nodes;//sub[l]: nodes in G_l
        long long core;
    } subgraph;

    void free_edgelist(edgelist *el){
        free(el->edges);
        free(el->rank);
        free(el);
    }

    void free_graph(graph *g){
        free(g->cd);
        free(g->adj);
        free(g);
    }

    void free_subgraph(subgraph *sg, NodeId k){
        NodeId i;
        free(sg->n);
        for (i=2;i<k;i++){
            free(sg->d[i]);
            free(sg->nodes[i]);
        }
        free(sg->d);
        free(sg->nodes);
        free(sg->lab);
        free(sg->adj);
        free(sg);
    }


    //Compute the maximum of three NodeIdegers.
    inline NodeId max3(NodeId a,NodeId b,NodeId c){
        a=(a>b) ? a : b;
        return (a>c) ? a : c;
    }

    edgelist* readedgelist(char* input){
        NodeId e1=NLINKS;
        edgelist *el=(edgelist*)malloc(sizeof(edgelist));
        FILE *file;

        el->n=0;
        el->e=0;
        file=fopen(input,"r");
        el->edges=(edge*)malloc(e1*sizeof(edge));
        while (fscanf(file,"%u %u", &(el->edges[el->e].s), &(el->edges[el->e].t))==2) {//Add one edge
            el->n=max3(el->n,el->edges[el->e].s,el->edges[el->e].t);
            el->e++;
            if (el->e==e1) {
                e1+=NLINKS;
                el->edges=(edge*)realloc(el->edges,e1*sizeof(edge));
            }
        }
        fclose(file);
        el->n++;

        el->edges=(edge*)realloc(el->edges,el->e*sizeof(edge));

        return el;
    }

    void relabel(edgelist *el){
        NodeId i, source, target, tmp;

        for (i=0;i<el->e;i++) {
            source=el->rank[el->edges[i].s];
            target=el->rank[el->edges[i].t];
            if (source<target){
                tmp=source;
                source=target;
                target=tmp;
            }
            el->edges[i].s=source;
            el->edges[i].t=target;
        }

    }

    ///// CORE ordering /////////////////////

    typedef struct {
        NodeId key;
        NodeId value;
    } keyvalue;

    typedef struct {
        NodeId n_max;	// max number of nodes.
        NodeId n;	// number of nodes.
        NodeId *pt;	// pointers to nodes.
        keyvalue *kv; // nodes.
    } bheap;


    bheap *construct(NodeId n_max){
        NodeId i;
        bheap *heap=(bheap*)malloc(sizeof(bheap));

        heap->n_max=n_max;
        heap->n=0;
        heap->pt=(NodeId*)malloc(n_max*sizeof(NodeId));
        for (i=0;i<n_max;i++) heap->pt[i]=-1;
        heap->kv=(keyvalue*)malloc(n_max*sizeof(keyvalue));
        return heap;
    }

    void swap(bheap *heap,NodeId i, NodeId j) {
        keyvalue kv_tmp=heap->kv[i];
        NodeId pt_tmp=heap->pt[kv_tmp.key];
        heap->pt[heap->kv[i].key]=heap->pt[heap->kv[j].key];
        heap->kv[i]=heap->kv[j];
        heap->pt[heap->kv[j].key]=pt_tmp;
        heap->kv[j]=kv_tmp;
    }

    void bubble_up(bheap *heap,NodeId i) {
        NodeId j=(i-1)/2;
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
        NodeId i=0,j1=1,j2=2,j;
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

    void update(bheap *heap,NodeId key){
        NodeId i=heap->pt[key];
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

    //Building the heap structure with (key,value)=(node,degree) for each node
    bheap* mkheap(NodeId n,NodeId *v){
        NodeId i;
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

    //computing degeneracy ordering and core value
    void ord_core(edgelist* el){
        NodeId i,j,r=0,n=el->n,e=el->e;
        keyvalue kv;
        bheap *heap;

        NodeId *d0=(NodeId*)calloc(el->n,sizeof(NodeId));
        NodeId *cd0=(NodeId*)malloc((el->n+1)*sizeof(NodeId));
        NodeId *adj0=(NodeId*)malloc(2*el->e*sizeof(NodeId));
        for (i=0;i<e;i++) {
            d0[el->edges[i].s]++;
            d0[el->edges[i].t]++;
        }
        cd0[0]=0;
        for (i=1;i<n+1;i++) {
            cd0[i]=cd0[i-1]+d0[i-1];
            d0[i-1]=0;
        }
        for (i=0;i<e;i++) {
            adj0[ cd0[el->edges[i].s] + d0[ el->edges[i].s ]++ ]=el->edges[i].t;
            adj0[ cd0[el->edges[i].t] + d0[ el->edges[i].t ]++ ]=el->edges[i].s;
        }

        heap=mkheap(n,d0);

        el->rank=(NodeId*)malloc(n*sizeof(NodeId));
        for (i=0;i<n;i++){
            kv=popmin(heap);
            el->rank[kv.key]=n-(++r);
            for (j=cd0[kv.key];j<cd0[kv.key+1];j++){
                update(heap,adj0[j]);
            }
        }
        freeheap(heap);
        free(d0);
        free(cd0);
        free(adj0);
    }

    //////////////////////////
    //Building the special graph
    graph* mkgraph(edgelist *el){
        NodeId i,max;
        NodeId *d;
        graph* g=(graph*)malloc(sizeof(graph));

        d=(NodeId*)calloc(el->n,sizeof(NodeId));

        for (i=0;i<el->e;i++) {
            d[el->edges[i].s]++;
        }

        g->cd=(NodeId*)malloc((el->n+1)*sizeof(NodeId));
        g->cd[0]=0;
        max=0;
        for (i=1;i<el->n+1;i++) {
            g->cd[i]=g->cd[i-1]+d[i-1];
            max=(max>d[i-1])?max:d[i-1];
            d[i-1]=0;
        }
        printf("core value (max truncated degree) = %u\n",max);

        g->adj=(NodeId*)malloc(el->e*sizeof(NodeId));

        for (i=0;i<el->e;i++) {
            g->adj[ g->cd[el->edges[i].s] + d[ el->edges[i].s ]++ ]=el->edges[i].t;
        }

        free(d);
        g->core=max;
        g->n=el->n;
        return g;
    }


    subgraph* allocsub(graph *g,NodeId k){
        NodeId i;
        subgraph* sg=(subgraph*)malloc(sizeof(subgraph));
        sg->n=(NodeId*)calloc(k,sizeof(NodeId));
        sg->d=(NodeId**)malloc(k*sizeof(NodeId*));
        sg->nodes=(NodeId**)malloc(k*sizeof(NodeId*));
        for (i=2;i<k;i++){
            sg->d[i]=(NodeId*)malloc(g->core*sizeof(NodeId));
            sg->nodes[i]=(NodeId*)malloc(g->core*sizeof(NodeId));
        }
        sg->lab=(NodeId*)calloc(g->core,sizeof(NodeId));
        sg->adj=(NodeId*)malloc(g->core*g->core*sizeof(NodeId));
        sg->core=g->core;
        return sg;
    }

    void mksub(graph* g,NodeId u,subgraph* sg,NodeId k,
        NodeId *old, NodeId *newv){
        NodeId i,j,l,v,w;


        for (i=0;i<sg->n[k-1];i++){
            sg->lab[i]=0;
        }

        j=0;
        for (i=g->cd[u];i<g->cd[u+1];i++){
            v=g->adj[i];
            newv[v]=j;
            old[j]=v;
            sg->lab[j]=k-1;
            sg->nodes[k-1][j]=j;
            sg->d[k-1][j]=0;//newv degrees
            j++;
        }

        sg->n[k-1]=j;

        for (i=0;i<sg->n[k-1];i++){//reodering adjacency list and computing newv degrees
            v=old[i];
            for (l=g->cd[v];l<g->cd[v+1];l++){
                w=g->adj[l];
                j=newv[w];
                if (j!=-1){
                    sg->adj[sg->core*i+sg->d[k-1][i]++]=j;
                }
            }
        }

        for (i=g->cd[u];i<g->cd[u+1];i++){
            newv[g->adj[i]]=-1;
        }
    }

    void doCounting(subgraph* sg, unsigned long long* n)
    {
        NodeId i, j, end, u;
        for(i=0; i<sg->n[2]; i++){//list all edges
            u=sg->nodes[2][i];
            end=u*sg->core+sg->d[2][u];
            for (j=u*sg->core;j<end;j++) {
                (*n)++;//listing here!!!  // NOTE THAT WE COULD DO (*n)+=g->d[2][u] to be much faster (for counting only); !!!!!!!!!!!!!!!!!!
            }
        }
    }

    void buildSubGraph(subgraph* sg, int l, NodeId i)
    {
        NodeId j, u, v, end;
        u=sg->nodes[l][i];
        //printf("%u %u\n",i,u);
        sg->n[l-1]=0;
        end=u*sg->core+sg->d[l][u];
        for (j=u*sg->core;j<end;j++){//relabeling nodes and forming U'.
            v=sg->adj[j];
            if (sg->lab[v]==l){
                sg->lab[v]=l-1;
                sg->nodes[l-1][sg->n[l-1]++]=v;
                sg->d[l-1][v]=0;//newv degrees
            }
        }
    }

    void orderAndCount(subgraph* sg, int l)
    {
        NodeId j, v, w, end, k;
        for (j=0;j<sg->n[l-1];j++){//reodering adjacency list and computing newv degrees
            v=sg->nodes[l-1][j];
            end=sg->core*v+sg->d[l][v];
            for (k=sg->core*v;k<end;k++){
                w=sg->adj[k];
                if (sg->lab[w]==l-1){
                    sg->d[l-1][v]++;
                }
                else{
                    sg->adj[k--]=sg->adj[--end];
                    sg->adj[end]=w;
                }
            }
        }
    }

    void restoreLabels(subgraph* sg, int l)
    {
        NodeId j, v;
        for (j=0;j<sg->n[l-1];j++){//restoring labels
            v=sg->nodes[l-1][j];
            sg->lab[v]=l;
        }
    }

    void kclique_thread(int l, subgraph *sg, unsigned long long *n) {
        long long i,j,k,end;
        NodeId u, v, w;

        if(l==2){
            doCounting(sg, n);
            return;
        }

        for(i=0; i<sg->n[l]; i++){
            buildSubGraph(sg, l, i);
            orderAndCount(sg, l);

            kclique_thread(l-1, sg, n);

            restoreLabels(sg, l);
        }
    }

    unsigned long long kclique_main(int k, graph *g) {
        NodeId u;
        unsigned long long n=0;
        subgraph *sg;
        NodeId *v_old, *v_new;
        #pragma omp parallel private(sg, v_old, v_new, u) reduction(+:n)
        {
            sg=allocsub(g,k);

            v_new=(NodeId*)malloc(g->n*sizeof(NodeId));
            v_old=(NodeId*)malloc(g->core*sizeof(NodeId));
            for (u=0;u<g->n;u++){
                v_new[u]=-1;
            }

            #pragma omp for schedule(dynamic, 1) nowait
            for(u=0; u<g->n; u++){
                mksub(g,u,sg,k, v_old, v_new);
                kclique_thread(k-1, sg, &n);
            }

            free(v_new);
            free(v_old);
        }
        return n;
    }
}

#endif