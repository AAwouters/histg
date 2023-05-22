#ifndef ADJLIST_H
#define ADJLIST_H

#include <histg_lib.h>

typedef struct AdjListEdge
{
    unsigned int origin;
    unsigned int destination;
    bool removed;
    bool selected;
} AdjListEdge;

AdjListEdge ale_new(unsigned int origin, unsigned int destination);

typedef struct AdjListEdgeArray
{
    unsigned int size;
    unsigned int capacity;
    AdjListEdge *edges;
} AdjListEdgeArray;

AdjListEdgeArray *alea_with_capacity(unsigned int capacity);
AdjListEdgeArray *alea_new();
AdjListEdge *add_edge_alea(AdjListEdgeArray *alea, AdjListEdge edge);
void free_alea(AdjListEdgeArray *alea);

typedef struct AdjListNeighbour
{
    unsigned int vertex;
    AdjListEdge *edge;
} AdjListNeighbour;

AdjListNeighbour aln_new(unsigned int vertex, AdjListEdge *edge);

typedef struct AdjListNeighbourArray
{
    unsigned int size;
    unsigned int capacity;
    AdjListNeighbour *neighbours;
} AdjListNeighbourArray;

AdjListNeighbourArray alna_with_capacity(unsigned int capacity);
AdjListNeighbourArray alna_new();
void add_neighbour_alna(AdjListNeighbourArray *alna, AdjListNeighbour neighbour);
void free_alna(AdjListNeighbourArray *alna, unsigned int array_count);

typedef struct AdjListGraph
{
    // Static value storing the number of vertices in the graph, not considering any hidden vertices
    unsigned int vertices;
    // Static bitset storing the available vertices
    uint64_t available_vertices;
    // Static value storing the number of available vertices in the graph
    unsigned int nb_available_vertices;
    // Static array storing all the edges belonging to this graph/tree-combo
    // Each edge has flags storing whether it is: removed/selected
    AdjListEdgeArray *edges;
    // Static array of AdjListNeighbourArrays to store the neighbours for each vertex
    // Length == vertices
    AdjListNeighbourArray *neighbours;
    // Dynamic value storing the number of selected edges
    unsigned int d_nb_tree_edges;
    // Dynamic array storing the degrees for the vertices in the graph
    unsigned int *d_graph_degrees;
    // Dynamic array storing the degrees for the vertices in the tree
    unsigned int *d_tree_degrees;
    // Dynamic bitset storing the vertices where the tree can be extended
    uint64_t extendable_vertices;
} AdjListGraph;

AdjListGraph *alg_from_graph_and_hidden(Graph *graph, uint64_t hidden_vertices);
void free_alg(AdjListGraph *graph);
Graph *get_tree(AdjListGraph *alg);

void add_edge_to_graph_alg(AdjListGraph *graph, AdjListEdge *edge);
void add_edge_to_tree_alg(AdjListGraph *graph, AdjListEdge *edge);
void remove_edge_from_graph_alg(AdjListGraph *graph, AdjListEdge *edge);
void remove_edge_from_tree_alg(AdjListGraph *graph, AdjListEdge *edge);

bool tree_is_finished_alg(AdjListGraph *graph);
bool is_valid_hist_alg(AdjListGraph *graph);

bool get_next_edge_alg(AdjListGraph *graph, AdjListEdge **out_edge, bool *out_both_in_tree);

void add_edge_alg(AdjListGraph *graph, AdjListEdge *edge, Output *output, bool find_one, RunData *run_data);
void remove_edge_alg(AdjListGraph *graph, AdjListEdge *edge, Output *output, bool find_one, RunData *run_data);
void hists_alg(AdjListGraph *graph, Output *output, bool find_one, RunData *run_data);

bool is_hypohist_partials_alg(Graph *input_graph, Output *output, RunData *run_data);
bool is_hypohist_alg(Graph *input_graph, Output *output, bool only_partials, RunData *run_data);
bool find_hists_alg(Graph *input_graph, uint64_t hidden_vertices, Output *output, bool find_one, RunData *run_data);

#endif
