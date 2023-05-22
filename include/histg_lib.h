#ifndef HISTG_LIB_H
#define HISTG_LIB_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

extern const uint64_t FIRST_BIT;
unsigned int count_set_bits(uint64_t bitset);
unsigned int last_bit_position(uint64_t bitset);
unsigned int first_bit_position(uint64_t bitset);
void uint64_to_binary_capped(uint64_t value, int length, char *binary);

typedef struct Graph
{
    unsigned int vertices;
    unsigned int edges;
    uint64_t *adjacency_matrix;
} Graph;

Graph *empty_graph(unsigned int vertices);
void graph_copy(const Graph *original, Graph *copy);

void free_graph(Graph *graph);

Graph parse_adjacency_matrix_file(FILE *input);
void parse_graph6_line(char *graph6_line, Graph *graph);
Graph parse_graph6_file(FILE *input);

typedef enum Format
{
    Graph6,
    AdjacencyMatrix,
    AdjacencyList,
} Format;

typedef struct Output
{
    FILE *output_file;
    Format format;
} Output;

typedef struct RunData
{
    unsigned long long int hists_this_run;
    unsigned long long int hists_total;
    unsigned long long int trees_this_run;
    unsigned long long int trees_total;
} RunData;

RunData *rd_new();
void rd_reset(RunData *rd);
void rd_start_run(RunData *rd);
void rd_finish_run(RunData *rd);

void print_graph_to_output(Output *output, Graph *graph);
void print_graph_to_output_as_adjacency_matrix(FILE *output, Graph *graph);
void print_graph_to_output_as_adjacency_list(FILE *output, Graph *graph);
char *get_graph6_string(Graph *graph);
void print_graph_to_output_as_graph6(FILE *output, Graph *graph);

unsigned int vertex_degree(uint64_t adjacencies);

typedef struct Timer
{
    clock_t start;
    clock_t end;
} Timer;

void start_timer(Timer *timer);
void end_timer(Timer *timer);
double elapsed_time_seconds(Timer *timer);
void print_elapsed_time_to_output(FILE *output, Timer *timer);

typedef struct Edge
{
    unsigned int origin;
    unsigned int destination;
} Edge;

void add_edge_to_graph(Graph *graph, Edge *edge);
void remove_edge_from_graph(Graph *graph, Edge *edge);

bool tree_is_finished(Graph *tree);
bool get_highest_edge(Graph *graph, Graph *tree, Edge *edge);

unsigned long long int find_spanning_trees(Graph *input_graph, Output *output, bool find_one);
bool find_hists(Graph *input_graph, Output *output, bool find_one, RunData *run_data);
bool find_all_hists_bf(Graph *input_graph, Output *Output, bool find_one, RunData *run_data);

typedef struct HideData
{
    uint64_t available_vertices;
    unsigned int nb_hidden_vertices;
} HideData;

HideData construct_hide_data(uint64_t hidden_vertices, unsigned int nb_vertices_in_graph);

bool find_hists_hd(Graph *input_graph, uint64_t hidden_vertices, Output *output, bool find_one, RunData *run_data);

bool is_hypohist_partials(Graph *input_graph, Output *output, RunData *run_data);
bool is_hypohist(Graph *input_graph, Output *output, bool only_partials, RunData *run_data);

#endif
