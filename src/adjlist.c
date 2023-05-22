#include <adjlist.h>
#include <stdlib.h>

/*
 * Adjacency List Edge
 */
AdjListEdge ale_new(unsigned int origin, unsigned int destination)
{
    AdjListEdge edge;

    edge.origin = origin;
    edge.destination = destination;
    edge.removed = false;
    edge.selected = false;

    return edge;
}

/*
 * Adjacency List Edge Array
 */
AdjListEdgeArray *alea_with_capacity(unsigned int capacity)
{
    AdjListEdgeArray *alea = malloc(sizeof(AdjListEdgeArray));

    alea->size = 0;
    alea->capacity = capacity;
    alea->edges = malloc(capacity * sizeof(AdjListEdge));

    return alea;
}

AdjListEdgeArray *alea_new()
{
    return alea_with_capacity(4);
}

AdjListEdge *add_edge_alea(AdjListEdgeArray *alea, AdjListEdge edge)
{
    if (alea->capacity <= alea->size)
    {
        unsigned int new_capacity = alea->capacity * 2;
        alea->edges = realloc(alea->edges, new_capacity * sizeof(AdjListEdge));

        if (alea->edges == NULL)
        {
            fprintf(stderr, "Reallocation failed for add edge. Requested capacity: %u\n", new_capacity);
            exit(EXIT_FAILURE);
        }

        alea->capacity = new_capacity;
    }

    unsigned int edge_index = alea->size;
    alea->size++;
    alea->edges[edge_index] = edge;

    return &alea->edges[edge_index];
}

void free_alea(AdjListEdgeArray *alea)
{
    free(alea->edges);
    free(alea);
}

/*
 * Adjacency List Neighbour
 */
AdjListNeighbour aln_new(unsigned int neighbour, AdjListEdge *edge)
{
    AdjListNeighbour aln;

    aln.vertex = neighbour;
    aln.edge = edge;

    return aln;
}

/*
 * Adjacency List Neighbour Array
 * contains the neighbours for a single vertex
 */
AdjListNeighbourArray alna_with_capacity(unsigned int capacity)
{
    AdjListNeighbourArray alna;

    alna.size = 0;
    alna.capacity = capacity;
    alna.neighbours = malloc(capacity * sizeof(AdjListNeighbour));

    return alna;
}

AdjListNeighbourArray alna_new()
{
    return alna_with_capacity(4);
}

AdjListNeighbourArray *alna_array(unsigned int count)
{
    AdjListNeighbourArray *array = malloc(count * sizeof(AdjListNeighbourArray));

    for (int i = 0; i < count; i++)
    {
        array[i] = alna_new();
    }

    return array;
}

void add_neighbour_alna(AdjListNeighbourArray *alna, AdjListNeighbour neighbour)
{
    if (alna->capacity <= alna->size)
    {
        unsigned int new_capacity = alna->capacity * 2;
        alna->neighbours = realloc(alna->neighbours, new_capacity * sizeof(AdjListNeighbour));

        if (alna->neighbours == NULL)
        {
            fprintf(stderr, "Reallocation failed for add neighbour. requested capacity: %u\n", new_capacity);
            exit(EXIT_FAILURE);
        }

        alna->capacity = new_capacity;
    }

    alna->neighbours[alna->size] = neighbour;
    alna->size++;
}

void free_alna(AdjListNeighbourArray *alna, unsigned int array_count)
{
    for (int i = 0; i < array_count; i++)
    {
        free(alna[i].neighbours);
    }

    free(alna);
}

void add_neighbour_for_vertex_alna(AdjListNeighbourArray *alna, unsigned int vertex, AdjListNeighbour neighbour)
{
    AdjListNeighbourArray *alna_vertex = &alna[vertex];
    add_neighbour_alna(alna_vertex, neighbour);
}

/*
 * Adjacency List Graph
 */
AdjListGraph *alg_from_graph_and_hidden(Graph *graph, uint64_t hidden_vertices)
{
    AdjListGraph *alg = malloc(sizeof(AdjListGraph));

    HideData hd = construct_hide_data(hidden_vertices, graph->vertices);

    // Initialize values
    alg->vertices = graph->vertices;
    alg->available_vertices = hd.available_vertices;
    alg->nb_available_vertices = graph->vertices - hd.nb_hidden_vertices;
    alg->edges = alea_with_capacity(graph->edges);
    alg->neighbours = alna_array(graph->vertices);
    alg->d_nb_tree_edges = 0;
    alg->d_graph_degrees = calloc(alg->vertices, sizeof(unsigned int));
    alg->d_tree_degrees = calloc(alg->vertices, sizeof(unsigned int));
    alg->extendable_vertices = 0;

    // Calculate degrees for all vertices
    for (int vertex = 0; vertex < graph->vertices; vertex++)
    {
        uint64_t vertex_bit = FIRST_BIT >> vertex;
        bool available = vertex_bit & hd.available_vertices;

        uint64_t available_neighbours = available ? graph->adjacency_matrix[vertex] & hd.available_vertices : 0;
        unsigned int degree = vertex_degree(available_neighbours);

        alg->d_graph_degrees[vertex] = degree;
    }

    // Determine and store all edges & neighbours
    for (unsigned int origin = 0; origin < alg->vertices - 1; origin++)
    {
        uint64_t origin_bit = FIRST_BIT >> origin;

        if (!(origin_bit & hd.available_vertices))
            continue;

        uint64_t origin_adjacencies = graph->adjacency_matrix[origin] & hd.available_vertices;

        for (unsigned int destination = origin + 1; destination < alg->vertices; destination++)
        {
            uint64_t destination_bit = FIRST_BIT >> destination;

            if (destination_bit & origin_adjacencies)
            {
                AdjListEdge edge = ale_new(origin, destination);
                AdjListEdge *edge_ptr = add_edge_alea(alg->edges, edge);

                AdjListNeighbour origins_neighbour = aln_new(destination, edge_ptr);
                AdjListNeighbour destinations_neighbour = aln_new(origin, edge_ptr);

                add_neighbour_for_vertex_alna(alg->neighbours, origin, origins_neighbour);
                add_neighbour_for_vertex_alna(alg->neighbours, destination, destinations_neighbour);
            }
        }
    }

    return alg;
}

void free_alg(AdjListGraph *graph)
{
    free_alna(graph->neighbours, graph->vertices);
    free_alea(graph->edges);
    free(graph->d_graph_degrees);
    free(graph->d_tree_degrees);
    free(graph);
}

Graph *get_tree(AdjListGraph *alg)
{
    Graph *graph = empty_graph(alg->vertices);

    AdjListEdgeArray *edge_array = alg->edges;

    for (int i = 0; i < edge_array->size; i++)
    {
        AdjListEdge alg_edge = edge_array->edges[i];
        if (alg_edge.selected)
        {
            Edge edge;
            edge.origin = alg_edge.origin;
            edge.destination = alg_edge.destination;

            add_edge_to_graph(graph, &edge);
        }
    }

    return graph;
}

/*
 * Adjacency List hist algorithm
 */
void update_extendable_vertices_alg(AdjListGraph *graph, AdjListEdge *edge)
{
    unsigned int *graph_degrees = graph->d_graph_degrees;
    unsigned int *tree_degrees = graph->d_tree_degrees;

    unsigned int origin = edge->origin;
    uint64_t origin_bit = FIRST_BIT >> origin;
    if (tree_degrees[origin] > 0 && graph_degrees[origin] > tree_degrees[origin])
    {
        graph->extendable_vertices |= origin_bit;
    }
    else
    {
        graph->extendable_vertices &= ~origin_bit;
    }

    unsigned int destination = edge->destination;
    uint64_t destination_bit = FIRST_BIT >> destination;
    if (tree_degrees[destination] > 0 && graph_degrees[destination] > tree_degrees[destination])
    {
        graph->extendable_vertices |= destination_bit;
    }
    else
    {
        graph->extendable_vertices &= ~destination_bit;
    }
}

void add_edge_to_graph_alg(AdjListGraph *graph, AdjListEdge *edge)
{
    edge->removed = false;
    graph->d_graph_degrees[edge->origin] += 1;
    graph->d_graph_degrees[edge->destination] += 1;
    update_extendable_vertices_alg(graph, edge);
}

void add_edge_to_tree_alg(AdjListGraph *graph, AdjListEdge *edge)
{
    edge->selected = true;
    graph->d_nb_tree_edges += 1;
    graph->d_tree_degrees[edge->origin] += 1;
    graph->d_tree_degrees[edge->destination] += 1;
    update_extendable_vertices_alg(graph, edge);
}

void remove_edge_from_graph_alg(AdjListGraph *graph, AdjListEdge *edge)
{
    edge->removed = true;
    graph->d_graph_degrees[edge->origin] -= 1;
    graph->d_graph_degrees[edge->destination] -= 1;
    update_extendable_vertices_alg(graph, edge);
}

void remove_edge_from_tree_alg(AdjListGraph *graph, AdjListEdge *edge)
{
    edge->selected = false;
    graph->d_nb_tree_edges -= 1;
    graph->d_tree_degrees[edge->origin] -= 1;
    graph->d_tree_degrees[edge->destination] -= 1;
    update_extendable_vertices_alg(graph, edge);
}

bool tree_is_finished_alg(AdjListGraph *graph)
{
    return graph->d_nb_tree_edges == graph->nb_available_vertices - 1;
}

bool is_valid_hist_alg(AdjListGraph *graph)
{
    unsigned int *tree_degrees = graph->d_tree_degrees;

    for (int vertex = 0; vertex < graph->vertices; vertex++)
    {
        uint64_t vertex_bit = FIRST_BIT >> vertex;

        if (vertex_bit & graph->available_vertices)
        {
            if (tree_degrees[vertex] == 2)
                return false;
        }
    }

    return true;
}

bool get_smallest_vertex_for_set_alg(AdjListGraph *graph, uint64_t bitset, unsigned int *out_vertex)
{
    if (!bitset)
        return false;

    unsigned int smallest_vertex = 65;
    unsigned int smallest_degree = 65;

    while (bitset)
    {
        unsigned int vertex = first_bit_position(bitset);
        unsigned int degree = graph->d_graph_degrees[vertex];

        if (degree < smallest_degree)
        {
            smallest_vertex = vertex;
            smallest_degree = degree;
        }

        bitset &= ~(FIRST_BIT >> vertex);
    }

    *out_vertex = smallest_vertex;
    return true;
}

bool get_smallest_neighbour_alg(AdjListGraph *graph, unsigned int origin, AdjListNeighbour *out_neighbour)
{
    AdjListNeighbourArray n_array = graph->neighbours[origin];
    bool found_neighbour = false;

    unsigned int smallest_degree = 65;

    for (int i = 0; i < n_array.size; i++)
    {
        AdjListNeighbour neighbour = n_array.neighbours[i];
        AdjListEdge *edge = neighbour.edge;

        if (edge->removed || edge->selected)
            continue;

        unsigned int destination = neighbour.vertex;
        unsigned int degree = graph->d_graph_degrees[destination];

        if (degree < smallest_degree)
        {
            smallest_degree = degree;
            *out_neighbour = neighbour;
            found_neighbour = true;
        }
    }

    return found_neighbour;
}

bool get_next_edge_alg(AdjListGraph *graph, AdjListEdge **out_edge, bool *out_both_in_tree)
{
    uint64_t available_origins = graph->d_nb_tree_edges == 0 ? graph->available_vertices : graph->extendable_vertices;

    unsigned int origin;
    if (get_smallest_vertex_for_set_alg(graph, available_origins, &origin))
    {
        AdjListNeighbour destination;
        if (get_smallest_neighbour_alg(graph, origin, &destination))
        {
            // only have to check destination as origin should be in tree because of extendable_vertices
            *out_both_in_tree = graph->d_tree_degrees[destination.vertex] > 0;
            *out_edge = destination.edge;
            return true;
        }
    }

    return false;
}

bool hist_impossible(AdjListGraph *graph, AdjListEdge *edge)
{
    unsigned int orig = edge->origin;
    unsigned int dest = edge->destination;

    unsigned int *graph_d = graph->d_graph_degrees;
    unsigned int *tree_d = graph->d_tree_degrees;

    bool zero_degree = graph_d[orig] == 0 || graph_d[dest] == 0;
    bool orig_two_guaranteed = graph_d[orig] == 2 && tree_d[orig] == 2;
    bool dest_two_guaranteed = graph_d[dest] == 2 && tree_d[dest] == 2;

    return zero_degree || orig_two_guaranteed || dest_two_guaranteed;
}

void add_edge_alg(AdjListGraph *graph, AdjListEdge *edge, Output *output, bool find_one, RunData *run_data)
{
    add_edge_to_tree_alg(graph, edge);

    if (hist_impossible(graph, edge))
        return;

    hists_alg(graph, output, find_one, run_data);
}

void remove_edge_alg(AdjListGraph *graph, AdjListEdge *edge, Output *output, bool find_one, RunData *run_data)
{
    remove_edge_from_graph_alg(graph, edge);

    if (hist_impossible(graph, edge))
        return;

    hists_alg(graph, output, find_one, run_data);
}

void hists_alg(AdjListGraph *graph, Output *output, bool find_one, RunData *run_data)
{
    if (find_one && run_data->hists_this_run >= 1)
        return;

    if (tree_is_finished_alg(graph))
    {
        if (is_valid_hist_alg(graph))
        {
            if (output)
            {
                Graph *tree = get_tree(graph);
                print_graph_to_output(output, tree);
                free_graph(tree);
            }

            run_data->hists_this_run += 1;
        }

        run_data->trees_this_run += 1;

        return;
    }

    AdjListEdge *edge;
    bool both_in_tree = false;
    if (get_next_edge_alg(graph, &edge, &both_in_tree))
    {
        if (!both_in_tree)
        {
            add_edge_alg(graph, edge, output, find_one, run_data);
            remove_edge_from_tree_alg(graph, edge);
        }
        remove_edge_alg(graph, edge, output, find_one, run_data);
        add_edge_to_graph_alg(graph, edge);
    }
}

bool find_hists_alg(Graph *input_graph, uint64_t hidden_vertices, Output *output, bool find_one, RunData *run_data)
{
    if (run_data == NULL)
    {
        fprintf(stderr, "No RunData struct provided.\n");
        exit(EXIT_FAILURE);
    }

    AdjListGraph *graph = alg_from_graph_and_hidden(input_graph, hidden_vertices);

    rd_start_run(run_data);
    hists_alg(graph, output, find_one, run_data);
    rd_finish_run(run_data);

    free_alg(graph);
    return run_data->hists_this_run != 0;
}

bool is_hypohist_partials_alg(Graph *input_graph, Output *output, RunData *run_data)
{
    if (run_data == NULL)
    {
        fprintf(stderr, "No RunData struct provided.\n");
        exit(EXIT_FAILURE);
    }

    for (unsigned int vertex = 0; vertex < input_graph->vertices; vertex++)
    {
        uint64_t hidden_vertex = FIRST_BIT >> vertex;
        if (!find_hists_alg(input_graph, hidden_vertex, output, true, run_data))
            return false;
    }

    return true;
}

bool is_hypohist_alg(Graph *input_graph, Output *output, bool only_partials, RunData *run_data)
{
    if (run_data == NULL)
    {
        fprintf(stderr, "No RunData struct provided.\n");
        exit(EXIT_FAILURE);
    }

    if (only_partials)
        return is_hypohist_partials_alg(input_graph, output, run_data);
    else
        return !find_hists_alg(input_graph, 0, output, true, run_data) && is_hypohist_partials_alg(input_graph, output, run_data);
}
