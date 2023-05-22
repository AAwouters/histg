#define _GNU_SOURCE
#include <histg_lib.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

const uint64_t FIRST_BIT = 1ULL << 63;

unsigned int count_set_bits(uint64_t bitset)
{
    return __builtin_popcountll(bitset);
}

unsigned int vertex_degree(uint64_t adjacencies)
{
    return count_set_bits(adjacencies);
}

// returns nonsensical value when bitset == 0
unsigned int last_bit_position(uint64_t bitset)
{
    unsigned int from_right = __builtin_ffsll(bitset) - 1;
    return 63 - from_right;
}

// undefined when bitset == 0
unsigned int first_bit_position(uint64_t bitset)
{
    return __builtin_clzll(bitset);
}

Graph *empty_graph(unsigned int vertices)
{
    Graph *graph = malloc(sizeof(Graph));

    if (graph == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for graph\n");
        exit(EXIT_FAILURE);
    }

    graph->vertices = vertices;
    graph->edges = 0;
    graph->adjacency_matrix = calloc(vertices, sizeof(uint64_t));

    if (graph->adjacency_matrix == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for adjacency matrix\n");
        exit(EXIT_FAILURE);
    }

    return graph;
}

void graph_copy(const Graph *original, Graph *copy)
{
    copy->vertices = original->vertices;
    copy->edges = original->edges;
    copy->adjacency_matrix = calloc(copy->vertices, sizeof(uint64_t));

    if (copy->adjacency_matrix == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for graph copy\n");
        exit(EXIT_FAILURE);
    }

    memcpy(copy->adjacency_matrix, original->adjacency_matrix, copy->vertices * sizeof(uint64_t));
}

void free_graph(Graph *graph)
{
    free(graph->adjacency_matrix);
    free(graph);
}

// Removes the spaces from the supplied untrimmed char* and places the resulting string in trimmed
// trimmed and untrimmed may be the same pointer
void remove_spaces(char *trimmed, char *untrimmed)
{
    while (*untrimmed != '\0')
    {
        if (!isspace(*untrimmed))
        {
            *trimmed = *untrimmed;
            trimmed++;
        }
        untrimmed++;
    }
    *trimmed = '\0';
}

void add_spaces(char *string_with_spaces, char *string)
{
    while (*string != '\0')
    {
        *string_with_spaces = *string;
        string++;
        string_with_spaces++;
        *string_with_spaces = ' ';
        string_with_spaces++;
    }
    string_with_spaces--;
    *string_with_spaces = '\0';
}

void concatenate(char *first, char *second, char *combined)
{
    while (*first != '\0')
    {
        *combined++ = *first++;
    }

    while (*second != '\0')
    {
        *combined++ = *second++;
    }

    *combined = '\0';
}

// Fills the provided char buffer with the binary representation of the given uint64_t
// The char buffer size must at least be 65 to fit the string and terminator
void uint64_to_binary(uint64_t value, char *binary)
{
    int bits = 64;

    for (int i = bits; i > 0; --i)
    {
        binary[bits - i] = (value & (1ULL << (i - 1))) ? '1' : '0';
    }

    binary[bits] = '\0';
}

// Shortens string to specified length by inserting terminator at given length
// Make sure buffer is at least of size length
void trim_string(char *string, int length)
{
    string[length] = '\0';
}

void uint64_to_binary_capped(uint64_t value, int length, char *binary)
{
    if (binary == NULL)
    {
        return;
    }

    uint64_to_binary(value, binary);
    trim_string(binary, length);
}

uint64_t parse_adjacency_matrix_line(char *line, int length)
{
    uint64_t result = 0ULL;

    for (uint64_t i = 0ULL; i < length; i++)
    {
        if (line[i] == '1')
        {
            result |= FIRST_BIT >> i;
        }
    }

    return result;
}

Graph parse_adjacency_matrix_file(FILE *input)
{
    if (input == NULL)
        exit(EXIT_FAILURE);

    char *line = NULL;
    size_t length = 0;
    size_t read;

    Graph graph;
    graph.vertices = 0;
    graph.edges = 0;

    char *binary = malloc(65);

    int read_lines = 0;

    // Handle first line differently as we want to allocate buffers depending on the size of the graph
    // which we determine based on the first line
    if ((read = getline(&line, &length, input)) != -1)
    {
        remove_spaces(line, line);
        int trimmed_length = strlen(line);

        graph.vertices = trimmed_length;
        graph.adjacency_matrix = calloc(graph.vertices, sizeof(uint64_t));

        uint64_t adjacencies = parse_adjacency_matrix_line(line, trimmed_length);
        graph.edges += count_set_bits(adjacencies);
        graph.adjacency_matrix[read_lines] = adjacencies;
    }

    read_lines = 1;

    // Parse remaining lines
    while (((read = getline(&line, &length, input)) != -1) && (read_lines < graph.vertices))
    {
        remove_spaces(line, line);
        int trimmed_length = strlen(line);

        uint64_t adjacencies = parse_adjacency_matrix_line(line, trimmed_length);
        graph.edges += count_set_bits(adjacencies);
        graph.adjacency_matrix[read_lines] = adjacencies;
        read_lines++;
    }

    graph.edges >>= 1; // divide edges by two as they get counted double

    // free allocated buffers
    if (line)
        free(line);

    free(binary);

    return graph;
}

int get_graph6_number_of_vertices(const char *graph6_line)
{
    if (strlen(graph6_line) == 0)
    {
        fprintf(stderr, "Graph6 input string is empty\n");
        exit(EXIT_FAILURE);
    }

    if ((graph6_line[0] < 63 || graph6_line[0] > 126) && graph6_line[0] != '>')
    {
        fprintf(stderr, "Invalid start character in graph6 string.\n");
        exit(EXIT_FAILURE);
    }

    int index = 0;

    if (graph6_line[index] == '>') // Skip >>Graph6<< header
    {
        index += 10;
    }

    if (graph6_line[index] < 126) // 0 <= n <= 62
    {
        return (int)graph6_line[index] - 63;
    }

    if (graph6_line[++index] < 126)
    {
        int number = 0;
        for (int i = 2; i >= 0; i--)
        {
            number |= (graph6_line[index++] - 63) << i * 6;
        }
        return number;
    }

    if (graph6_line[++index] < 126)
    {
        int number = 0;
        for (int i = 5; i >= 0; i--)
        {
            number |= (graph6_line[index++] - 63) << i * 6;
        }
        return number;
    }

    fprintf(stderr, "Graph6 format only works for graphs with up to 68719476735 vertices.\n");
    exit(EXIT_FAILURE);
}

#define unsafePrev(character, current) (__builtin_ctz(character) - (current) >= 0 ? -1 : (current)-__builtin_clz((character) << (32 - current)) - 1)
#define prev(character, current) ((character) ? unsafePrev(character, current) : -1)

void parse_graph6_line(char *graph6_line, Graph *graph)
{
    graph->edges = 0;

    int vertices = get_graph6_number_of_vertices(graph6_line);

    int start_index = 0;
    if (graph6_line[start_index] == '>') // Skip >>Graph6<< header
    {
        start_index += 10;
    }

    if (vertices <= 62)
    {
        start_index += 1;
    }
    else if (vertices <= 64)
    {
        start_index += 4;
    }
    else
    {
        fprintf(stderr, "Only graphs with up to 64 vertices are supported.\n");
        exit(EXIT_FAILURE);
    }

    graph->vertices = vertices;
    graph->adjacency_matrix = calloc(graph->vertices, sizeof(uint64_t));

    if (graph->adjacency_matrix == NULL)
    {
        fprintf(stderr, "Failed to allocate adjacency matrix");
        exit(EXIT_FAILURE);
    }

    int currentVertex = 1;
    int sum = 0;
    char final_char = '\n';
    for (int index = start_index; graph6_line[index] != '\n' && (final_char = graph6_line[index]) != '\0'; index++)
    {
        int i;
        for (i = prev(graph6_line[index] - 63, 6); i != -1; i = prev(graph6_line[index] - 63, i))
        {
            while (5 - i + (index - start_index) * 6 - sum >= 0)
            {
                sum += currentVertex;
                currentVertex++;
            }
            sum -= --currentVertex;
            int neighbour = 5 - i + (index - start_index) * 6 - sum;

            Edge edge;
            edge.origin = currentVertex;
            edge.destination = neighbour;

            add_edge_to_graph(graph, &edge);
        }
    }

    if (final_char == '\0')
    {
        fprintf(stderr, "Graph6 string should end with a newline character.\n");
        exit(EXIT_FAILURE);
    }
}

Graph parse_graph6_file(FILE *input)
{
    if (input == NULL)
    {
        fprintf(stderr, "given input argument is NULL.\n");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t length = 0;
    size_t read;

    if ((read = getline(&line, &length, input)) != -1)
    {
        Graph graph;
        parse_graph6_line(line, &graph);

        if (line)
            free(line);

        return graph;
    }
    else
    {
        fprintf(stderr, "failed to read line from given input.\n");
        exit(EXIT_FAILURE);
    }
}

void print_graph_to_output(Output *output, Graph *graph)
{
    switch (output->format)
    {
    case (AdjacencyMatrix):
    {
        print_graph_to_output_as_adjacency_matrix(output->output_file, graph);
        break;
    }
    case (AdjacencyList):
    {
        print_graph_to_output_as_adjacency_list(output->output_file, graph);
        break;
    }
    case (Graph6):
    {
        print_graph_to_output_as_graph6(output->output_file, graph);
        break;
    }
    }
}

// Prints graph in adjacency matrix notation to given output
void print_graph_to_output_as_adjacency_matrix(FILE *output, Graph *graph)
{
    char *binary = malloc(65);
    char *spaced_binary = malloc(128);

    for (int i = 0; i < graph->vertices; i++)
    {
        uint64_to_binary(graph->adjacency_matrix[i], binary);
        add_spaces(spaced_binary, binary);
        trim_string(spaced_binary, 2 * graph->vertices - 1);
        fprintf(output, "%s\n", spaced_binary);
    }

    free(binary);
    free(spaced_binary);
}

void print_graph_to_output_as_adjacency_list(FILE *output, Graph *graph)
{
    // loop over all vertices
    for (int i = 0; i < graph->vertices; i++)
    {
        fprintf(output, "%i:", i + 1);

        uint64_t adjacencies = graph->adjacency_matrix[i];
        for (int j = 0; j < graph->vertices; j++)
        {
            if (adjacencies & (FIRST_BIT >> j))
            {
                fprintf(output, " %i", j + 1);
            }
        }

        fprintf(output, "\n");
    }
}

void graph6_r(char *bitvector, int vector_length, char *output, int *output_length)
{
    int characters = vector_length / 6;
    int bitvector_index = 0;

    for (int i = 0; i < characters; i++)
    {
        char element = 0;

        for (int j = 5; j >= 0; j--)
        {
            char bit = bitvector[bitvector_index++] == '1' ? 1 : 0;
            element |= (bit << j);
        }

        element += 63;
        output[i] = element;
    }

    output[characters] = '\0';
    *output_length = characters;
}

char *get_graph6_string(Graph *graph)
{
    unsigned int vertices = graph->vertices;
    char *n;

    if (vertices <= 62)
    {
        n = malloc(2);
        if (n == NULL)
        {
            fprintf(stderr, "Failed to allocate while calculating g6 string.\n");
            exit(EXIT_FAILURE);
        }

        n[0] = (char)vertices + 63;
        n[1] = '\0';
    }
    else if (vertices <= 258047)
    {
        fprintf(stderr, "attempted to get g6 string for graph with more than 62 vertices. This is not yet implemented/n");
        exit(EXIT_FAILURE);
        // n = malloc(5);
        // n[0] = 126;
        // // TODO
        // n[4] = '\0';
    }
    else
    {
        fprintf(stderr, "attempted to get g6 string for graph with more than 62 vertices. This is not yet implemented/n");
        exit(EXIT_FAILURE);
        // n = malloc(9);
        // n[0] = 126;
        // n[1] = 126;
        // // TODO
        // n[8] = '\0';
    }

    int edges = vertices * (vertices - 1) / 2;
    int edges_rounded = ceil(edges / 6.0) * 6;
    char *edge_bitvector = malloc(edges_rounded + 1);

    if (edge_bitvector == NULL)
    {
        fprintf(stderr, "Failed to allocate edge_bitvector\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < vertices; i++)
    {
        uint64_t adjacencies = graph->adjacency_matrix[i];

        for (int j = 0; j < i; j++)
        {
            unsigned int bitvector_index = (i * (i - 1)) / 2 + j;
            char bit = (adjacencies & (FIRST_BIT >> j)) ? '1' : '0';
            edge_bitvector[bitvector_index] = bit;
        }
    }

    for (int i = edges; i < edges_rounded; i++)
    {
        edge_bitvector[i] = '0';
    }

    edge_bitvector[edges_rounded] = '\0';

    char *edge_characters = malloc(edges_rounded / 6 + 1);

    if (edge_characters == NULL)
    {
        fprintf(stderr, "Failed to allocate edge_characters\n");
        exit(EXIT_FAILURE);
    }

    int edge_characters_length;
    graph6_r(edge_bitvector, edges_rounded, edge_characters, &edge_characters_length);

    char *graph6 = malloc(edges_rounded + 3);

    if (graph6 == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for graph6 string result\n");
        exit(EXIT_FAILURE);
    }

    concatenate(n, edge_characters, graph6);

    free(n);
    free(edge_bitvector);
    free(edge_characters);

    return graph6;
}

void print_graph_to_output_as_graph6(FILE *output, Graph *graph)
{
    char *graph6 = get_graph6_string(graph);
    fprintf(output, "%s\n", graph6);
    free(graph6);
}
