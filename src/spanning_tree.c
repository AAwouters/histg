#include <histg_lib.h>
#include <stdlib.h>
#include <stdbool.h>

RunData *rd_new()
{
    RunData *rd = malloc(sizeof(RunData));

    rd->hists_this_run = 0;
    rd->hists_total = 0;
    rd->trees_this_run = 0;
    rd->trees_total = 0;

    return rd;
}

void rd_reset(RunData *rd)
{
    rd->hists_this_run = 0;
    rd->hists_total = 0;
    rd->trees_this_run = 0;
    rd->trees_total = 0;
}

void rd_start_run(RunData *rd)
{
    rd->hists_this_run = 0;
    rd->trees_this_run = 0;
}

void rd_finish_run(RunData *rd)
{
    rd->hists_total += rd->hists_this_run;
    rd->trees_total += rd->trees_this_run;
}

/*
    Edge
*/
void add_edge_to_graph(Graph *graph, Edge *edge)
{
    unsigned int origin_index = edge->origin;
    unsigned int destination_index = edge->destination;

    uint64_t origin_bitset = FIRST_BIT >> origin_index;
    uint64_t destination_bitset = FIRST_BIT >> destination_index;

    graph->adjacency_matrix[origin_index] |= destination_bitset;
    graph->adjacency_matrix[destination_index] |= origin_bitset;

    graph->edges += 1;
}

void remove_edge_from_graph(Graph *graph, Edge *edge)
{
    unsigned int origin_index = edge->origin;
    unsigned int destination_index = edge->destination;

    uint64_t origin_bitset = FIRST_BIT >> origin_index;
    uint64_t destination_bitset = FIRST_BIT >> destination_index;

    graph->adjacency_matrix[origin_index] &= ~destination_bitset;
    graph->adjacency_matrix[destination_index] &= ~origin_bitset;

    graph->edges -= 1;
}

/*
    Find all spanning trees
*/
bool tree_is_finished(Graph *tree)
{
    return tree->edges == tree->vertices - 1;
}

bool get_highest_edge(Graph *graph, Graph *tree, Edge *edge)
{
    for (unsigned int vertex1 = graph->vertices - 1; vertex1 > 0; vertex1--)
    {
        uint64_t adjacencies = graph->adjacency_matrix[vertex1];
        uint64_t used_edges = tree->adjacency_matrix[vertex1];
        uint64_t available_edges = adjacencies & ~used_edges;

        if (available_edges)
        {
            unsigned int vertex2 = last_bit_position(available_edges);
            edge->origin = vertex2;
            edge->destination = vertex1;
            return true;
        }
    }

    return false;
}

bool get_highest_neighbouring_edge(Graph *graph, Graph *tree, Edge *edge)
{
    for (unsigned int vertex1 = graph->vertices - 1; vertex1 > 0; vertex1--)
    {
        uint64_t adjacencies = graph->adjacency_matrix[vertex1];
        uint64_t used_edges = tree->adjacency_matrix[vertex1];
        uint64_t available_edges = adjacencies & ~used_edges;

        while (available_edges)
        {
            unsigned int vertex2 = last_bit_position(available_edges);

            // check that chosen edge has exactly one vertex already in the tree
            uint64_t v1_adjc = tree->adjacency_matrix[vertex1];
            uint64_t v2_adjc = tree->adjacency_matrix[vertex2];

            if ((v1_adjc && !v2_adjc) || (!v1_adjc && v2_adjc))
            {
                edge->origin = vertex2;
                edge->destination = vertex1;
                return true;
            }

            available_edges &= ~(FIRST_BIT >> vertex2);
        }
    }

    return false;
}

bool tree_is_empty(Graph *tree)
{
    return tree->edges == 0;
}

bool get_next_edge(Graph *graph, Graph *tree, Edge *edge)
{
    if (tree_is_empty(tree))
    {
        return get_highest_edge(graph, tree, edge);
    }
    else
    {
        return get_highest_neighbouring_edge(graph, tree, edge);
    }
}

void spanning_trees(Graph *graph, Graph *tree, Output *output, bool stop_at_first_tree, unsigned long long int *nb_trees_accumulator);

void add_edge(Graph *graph, Graph *tree, Edge *edge, Output *output, bool stop_at_first_tree, unsigned long long int *nb_trees_accumulator)
{
    add_edge_to_graph(tree, edge);

    if (tree_is_finished(tree))
    {
        if (output)
        {
            print_graph_to_output(output, tree);
        }

        *nb_trees_accumulator += 1;
        return;
    }

    spanning_trees(graph, tree, output, stop_at_first_tree, nb_trees_accumulator);
}

void remove_edge(Graph *graph, Graph *tree, Edge *edge, Output *output, bool stop_at_first_tree, unsigned long long int *nb_trees_accumulator)
{
    remove_edge_from_graph(graph, edge);

    // Check that the vertices are still reachable
    if (vertex_degree(graph->adjacency_matrix[edge->origin]) == 0 || vertex_degree(graph->adjacency_matrix[edge->destination]) == 0)
    {
        return;
    }

    spanning_trees(graph, tree, output, stop_at_first_tree, nb_trees_accumulator);
}

void spanning_trees(Graph *graph, Graph *tree, Output *output, bool stop_at_first_tree, unsigned long long int *nb_trees_accumulator)
{
    if (stop_at_first_tree && *nb_trees_accumulator >= 1)
    {
        return;
    }

    Edge edge;
    if (get_next_edge(graph, tree, &edge))
    {
        add_edge(graph, tree, &edge, output, stop_at_first_tree, nb_trees_accumulator);
        remove_edge_from_graph(tree, &edge);
        remove_edge(graph, tree, &edge, output, stop_at_first_tree, nb_trees_accumulator);
        add_edge_to_graph(graph, &edge);
    }
}

unsigned long long int spanning_trees_setup(Graph *input_graph, Output *output, bool stop_at_first_tree)
{
    Graph *graph = malloc(sizeof(Graph));
    graph_copy(input_graph, graph);
    Graph *tree = empty_graph(input_graph->vertices);
    unsigned long long int nb_trees = 0;
    spanning_trees(graph, tree, output, stop_at_first_tree, &nb_trees);

    free_graph(graph);
    free_graph(tree);
    return nb_trees;
}

unsigned long long int find_spanning_trees(Graph *input_graph, Output *output, bool find_one)
{
    return spanning_trees_setup(input_graph, output, find_one);
}

/*
    HIST specific
*/
bool is_valid_hist(Graph *tree)
{
    for (int i = 0; i < tree->vertices; i++)
    {
        uint64_t adjacencies = tree->adjacency_matrix[i];
        if (vertex_degree(adjacencies) == 2)
        {
            return false;
        }
    }

    return true;
}

bool get_highest_neighbouring_edge_hist(Graph *graph, Graph *tree, Edge *edge, bool *both_vertices_in_tree)
{
    for (unsigned int vertex1 = graph->vertices - 1; vertex1 > 0; vertex1--)
    {
        uint64_t adjacencies = graph->adjacency_matrix[vertex1];
        uint64_t used_edges = tree->adjacency_matrix[vertex1];
        uint64_t available_edges = adjacencies & ~used_edges;

        while (available_edges)
        {
            unsigned int vertex2 = last_bit_position(available_edges);

            // check that chosen edge has exactly one vertex already in the tree
            uint64_t v1_adjc = tree->adjacency_matrix[vertex1];
            uint64_t v2_adjc = tree->adjacency_matrix[vertex2];

            if (v1_adjc || v2_adjc)
            {
                *both_vertices_in_tree = v1_adjc && v2_adjc;
                edge->origin = vertex2;
                edge->destination = vertex1;
                return true;
            }

            available_edges &= ~(FIRST_BIT >> vertex2);
        }
    }

    return false;
}

bool get_next_edge_hist(Graph *graph, Graph *tree, Edge *edge, bool *both_vertices_in_tree)
{
    if (tree_is_empty(tree))
    {
        *both_vertices_in_tree = false;
        return get_highest_edge(graph, tree, edge);
    }
    else
    {
        return get_highest_neighbouring_edge_hist(graph, tree, edge, both_vertices_in_tree);
    }
}

void hists(Graph *graph, Graph *tree, Output *output, bool stop_at_first_tree, RunData *run_data);

void add_edge_hist(Graph *graph, Graph *tree, Edge *edge, Output *output, bool stop_at_first_tree, RunData *run_data)
{
    add_edge_to_graph(tree, edge);

    if (tree_is_finished(tree))
    {
        if (is_valid_hist(tree))
        {
            if (output)
            {
                print_graph_to_output(output, tree);
            }

            run_data->hists_this_run += 1;
        }

        run_data->trees_this_run += 1;

        return;
    }

    unsigned int origin = edge->origin;
    unsigned int destination = edge->destination;

    unsigned int origin_tree_degree = vertex_degree(tree->adjacency_matrix[origin]);
    unsigned int origin_graph_degree = vertex_degree(graph->adjacency_matrix[origin]);
    unsigned int destination_tree_degree = vertex_degree(tree->adjacency_matrix[destination]);
    unsigned int destination_graph_degree = vertex_degree(graph->adjacency_matrix[destination]);

    // Check that no edges with degree 2 are guaranteed by this addition
    if ((origin_tree_degree == 2 && origin_graph_degree == 2) || (destination_tree_degree == 2 && destination_graph_degree == 2))
    {
        return;
    }

    hists(graph, tree, output, stop_at_first_tree, run_data);
}

void remove_edge_hist(Graph *graph, Graph *tree, Edge *edge, Output *output, bool stop_at_first_tree, RunData *run_data)
{
    remove_edge_from_graph(graph, edge);

    unsigned int origin = edge->origin;
    unsigned int destination = edge->destination;

    unsigned int origin_tree_degree = vertex_degree(tree->adjacency_matrix[origin]);
    unsigned int origin_graph_degree = vertex_degree(graph->adjacency_matrix[origin]);
    unsigned int destination_graph_degree = vertex_degree(graph->adjacency_matrix[destination]);
    unsigned int destination_tree_degree = vertex_degree(tree->adjacency_matrix[destination]);

    // Check that the vertices are still reachable
    if (origin_graph_degree == 0 || destination_graph_degree == 0)
    {
        return;
    }

    // Check that no edges with degree 2 are guaranteed by this removal
    if ((origin_tree_degree == 2 && origin_graph_degree == 2) || (destination_tree_degree == 2 && destination_graph_degree == 2))
    {
        return;
    }

    hists(graph, tree, output, stop_at_first_tree, run_data);
}

void hists(Graph *graph, Graph *tree, Output *output, bool stop_at_first_tree, RunData *run_data)
{
    if (stop_at_first_tree && run_data->hists_this_run >= 1)
    {
        return;
    }

    Edge edge;
    bool both_vertices_in_tree = false;
    if (get_next_edge_hist(graph, tree, &edge, &both_vertices_in_tree))
    {
        if (!both_vertices_in_tree)
        {
            add_edge_hist(graph, tree, &edge, output, stop_at_first_tree, run_data);
            remove_edge_from_graph(tree, &edge);
        }
        remove_edge_hist(graph, tree, &edge, output, stop_at_first_tree, run_data);
        add_edge_to_graph(graph, &edge);
    }
}

bool find_hists(Graph *input_graph, Output *output, bool find_one, RunData *run_data)
{
    Graph *graph = malloc(sizeof(Graph));
    graph_copy(input_graph, graph);
    Graph *tree = empty_graph(input_graph->vertices);

    rd_start_run(run_data);
    hists(graph, tree, output, find_one, run_data);
    rd_finish_run(run_data);

    free_graph(graph);
    free_graph(tree);
    return run_data->hists_this_run != 0;
}

void hists_bf(Graph *graph, Graph *tree, Output *output, bool stop_at_first_tree, RunData *run_data);

void add_edge_hist_bf(Graph *graph, Graph *tree, Edge *edge, Output *output, bool stop_at_first_tree, RunData *run_data)
{
    add_edge_to_graph(tree, edge);
    hists_bf(graph, tree, output, stop_at_first_tree, run_data);
}

void remove_edge_hist_bf(Graph *graph, Graph *tree, Edge *edge, Output *output, bool stop_at_first_tree, RunData *run_data)
{
    remove_edge_from_graph(graph, edge);

    // Check that the vertices are still reachable
    if (vertex_degree(graph->adjacency_matrix[edge->origin]) == 0 || vertex_degree(graph->adjacency_matrix[edge->destination]) == 0)
    {
        return;
    }

    hists_bf(graph, tree, output, stop_at_first_tree, run_data);
}

void hists_bf(Graph *graph, Graph *tree, Output *output, bool stop_at_first_tree, RunData *run_data)
{
    if (stop_at_first_tree && run_data->hists_this_run >= 1)
    {
        return;
    }

    if (tree_is_finished(tree))
    {
        if (is_valid_hist(tree))
        {
            if (output)
            {
                print_graph_to_output(output, tree);
            }

            run_data->hists_this_run += 1;
        }

        run_data->trees_this_run += 1;
        return;
    }

    Edge edge;
    if (get_next_edge(graph, tree, &edge))
    {
        add_edge_hist_bf(graph, tree, &edge, output, stop_at_first_tree, run_data);
        remove_edge_from_graph(tree, &edge);
        remove_edge_hist_bf(graph, tree, &edge, output, stop_at_first_tree, run_data);
        add_edge_to_graph(graph, &edge);
    }
}

bool find_all_hists_bf(Graph *input_graph, Output *output, bool find_one, RunData *run_data)
{
    Graph *graph = malloc(sizeof(Graph));
    graph_copy(input_graph, graph);
    Graph *tree = empty_graph(input_graph->vertices);

    rd_start_run(run_data);
    hists_bf(graph, tree, output, find_one, run_data);
    rd_finish_run(run_data);

    free_graph(graph);
    free_graph(tree);
    return run_data->hists_this_run != 0;
}

/*
 * Hypohist
 */
HideData construct_hide_data(uint64_t hidden_vertices, unsigned int nb_vertices_in_graph)
{
    HideData hide_data;
    hide_data.nb_hidden_vertices = count_set_bits(hidden_vertices);

    unsigned int size = 64 - nb_vertices_in_graph;
    uint64_t inverse_mask = (((uint64_t)(size < 64)) << (size & 63)) - 1u;
    uint64_t mask = ~inverse_mask;

    hide_data.available_vertices = mask & ~hidden_vertices;

    return hide_data;
}

bool tree_is_finished_hd(Graph *tree, HideData *hide_data)
{
    unsigned int vertices = tree->vertices;
    unsigned int target_edges = vertices - hide_data->nb_hidden_vertices - 1;

    if (tree->edges != target_edges)
        return false;

    uint64_t available_vertices = hide_data->available_vertices;

    for (int i = 0; i < vertices; i++)
    {
        uint64_t vertex = FIRST_BIT >> i;
        uint64_t adjacencies = tree->adjacency_matrix[i];

        if ((vertex & available_vertices) && !adjacencies)
            return false;
    }

    return true;
}

bool is_valid_hist_hd(Graph *tree, HideData *hide_data)
{
    uint64_t available_vertices = hide_data->available_vertices;

    for (int i = 0; i < tree->vertices; i++)
    {
        uint64_t vertex = FIRST_BIT >> i;
        uint64_t adjacencies = tree->adjacency_matrix[i];
        if ((vertex & available_vertices) && (vertex_degree(adjacencies) == 2))
        {
            return false;
        }
    }

    return true;
}

bool get_highest_edge_hd(Graph *graph, Graph *tree, HideData *hide_data, Edge *edge)
{
    uint64_t hidden_vertices = ~hide_data->available_vertices;

    for (unsigned int largest_vertex = graph->vertices - 1; largest_vertex > 0; largest_vertex--)
    {
        uint64_t vertex = FIRST_BIT >> largest_vertex;

        if (vertex & hidden_vertices)
        {
            continue;
        }

        uint64_t adjacencies = graph->adjacency_matrix[largest_vertex];
        uint64_t used_edges = tree->adjacency_matrix[largest_vertex];
        uint64_t available_edges = adjacencies & ~used_edges & ~hidden_vertices;

        if (available_edges)
        {
            unsigned int second_largest_vertex = last_bit_position(available_edges);
            edge->origin = second_largest_vertex;
            edge->destination = largest_vertex;
            return true;
        }
    }

    return false;
}

bool get_highest_neighbouring_edge_hist_hd(Graph *graph, Graph *tree, HideData *hide_data, Edge *edge, bool *both_vertices_in_tree)
{
    uint64_t hidden_vertices = ~hide_data->available_vertices;

    for (unsigned int vertex1 = graph->vertices - 1; vertex1 > 0; vertex1--)
    {
        uint64_t vertex = FIRST_BIT >> vertex1;

        if (vertex & hidden_vertices)
        {
            continue;
        }

        uint64_t adjacencies = graph->adjacency_matrix[vertex1];
        uint64_t used_edges = tree->adjacency_matrix[vertex1];
        uint64_t available_edges = adjacencies & ~used_edges & ~hidden_vertices;

        while (available_edges)
        {
            unsigned int vertex2 = last_bit_position(available_edges);

            // check that chosen edge has exactly one vertex already in the tree
            uint64_t v1_adjc = tree->adjacency_matrix[vertex1];
            uint64_t v2_adjc = tree->adjacency_matrix[vertex2];

            if (v1_adjc || v2_adjc)
            {
                *both_vertices_in_tree = v1_adjc && v2_adjc;
                edge->origin = vertex2;
                edge->destination = vertex1;
                return true;
            }

            available_edges &= ~(FIRST_BIT >> vertex2);
        }
    }

    return false;
}

bool get_next_edge_hist_hd(Graph *graph, Graph *tree, Edge *edge, HideData *hide_data, bool *both_vertices_in_tree)
{
    if (tree_is_empty(tree))
    {
        *both_vertices_in_tree = false;
        return get_highest_edge_hd(graph, tree, hide_data, edge);
    }
    else
    {
        return get_highest_neighbouring_edge_hist_hd(graph, tree, hide_data, edge, both_vertices_in_tree);
    }
}
void hists_hd(Graph *graph, Graph *tree, HideData *hide_data, Output *output, bool stop_at_first_tree, RunData *run_data);

void add_edge_hist_hd(Graph *graph, Graph *tree, Edge *edge, HideData *hide_data, Output *output, bool stop_at_first_tree, RunData *run_data)
{
    add_edge_to_graph(tree, edge);

    uint64_t available_vertices = hide_data->available_vertices;

    unsigned int origin = edge->origin;
    unsigned int destination = edge->destination;

    unsigned int origin_tree_degree = vertex_degree(tree->adjacency_matrix[origin] & available_vertices);
    unsigned int origin_graph_degree = vertex_degree(graph->adjacency_matrix[origin] & available_vertices);
    unsigned int destination_tree_degree = vertex_degree(tree->adjacency_matrix[destination] & available_vertices);
    unsigned int destination_graph_degree = vertex_degree(graph->adjacency_matrix[destination] & available_vertices);

    // Check that no edges with degree 2 are guaranteed by this addition
    if ((origin_tree_degree == 2 && origin_graph_degree == 2) || (destination_tree_degree == 2 && destination_graph_degree == 2))
    {
        return;
    }

    hists_hd(graph, tree, hide_data, output, stop_at_first_tree, run_data);
}

void remove_edge_hist_hd(Graph *graph, Graph *tree, Edge *edge, HideData *hide_data, Output *output, bool stop_at_first_tree, RunData *run_data)
{
    remove_edge_from_graph(graph, edge);

    uint64_t available_vertices = hide_data->available_vertices;

    unsigned int origin = edge->origin;
    unsigned int destination = edge->destination;

    unsigned int origin_tree_degree = vertex_degree(tree->adjacency_matrix[origin] & available_vertices);
    unsigned int origin_graph_degree = vertex_degree(graph->adjacency_matrix[origin] & available_vertices);
    unsigned int destination_graph_degree = vertex_degree(graph->adjacency_matrix[destination] & available_vertices);
    unsigned int destination_tree_degree = vertex_degree(tree->adjacency_matrix[destination] & available_vertices);

    // Check that the vertices are still reachable
    if (origin_graph_degree == 0 || destination_graph_degree == 0)
    {
        return;
    }

    // Check that no edges with degree 2 are guaranteed by this removal
    if ((origin_tree_degree == 2 && origin_graph_degree == 2) || (destination_tree_degree == 2 && destination_graph_degree == 2))
    {
        return;
    }

    hists_hd(graph, tree, hide_data, output, stop_at_first_tree, run_data);
}

void hists_hd(Graph *graph, Graph *tree, HideData *hide_data, Output *output, bool stop_at_first_tree, RunData *run_data)
{
    if (stop_at_first_tree && run_data->hists_this_run >= 1)
    {
        return;
    }

    if (tree_is_finished_hd(tree, hide_data))
    {
        if (is_valid_hist_hd(tree, hide_data))
        {
            if (output)
            {
                print_graph_to_output(output, tree);
            }

            run_data->hists_this_run += 1;
        }

        run_data->trees_this_run += 1;

        return;
    }

    Edge edge;
    bool both_vertices_in_tree = false;
    if (get_next_edge_hist_hd(graph, tree, &edge, hide_data, &both_vertices_in_tree))
    {
        if (!both_vertices_in_tree)
        {
            add_edge_hist_hd(graph, tree, &edge, hide_data, output, stop_at_first_tree, run_data);
            remove_edge_from_graph(tree, &edge);
        }
        remove_edge_hist_hd(graph, tree, &edge, hide_data, output, stop_at_first_tree, run_data);
        add_edge_to_graph(graph, &edge);
    }
}

bool find_hists_hd(Graph *input_graph, uint64_t hidden_vertices, Output *output, bool find_one, RunData *run_data)
{
    if (run_data == NULL)
    {
        fprintf(stderr, "No RunData struct provided.\n");
        exit(EXIT_FAILURE);
    }

    Graph *graph = malloc(sizeof(Graph));
    graph_copy(input_graph, graph);
    Graph *tree = empty_graph(input_graph->vertices);

    HideData hide_data = construct_hide_data(hidden_vertices, input_graph->vertices);

    hists_hd(graph, tree, &hide_data, output, find_one, run_data);

    free_graph(graph);
    free_graph(tree);
    return run_data->hists_this_run != 0;
}

bool is_hypohist_partials(Graph *input_graph, Output *output, RunData *run_data)
{
    if (run_data == NULL)
    {
        fprintf(stderr, "No RunData struct provided.\n");
        exit(EXIT_FAILURE);
    }

    for (unsigned int vertex = 0; vertex < input_graph->vertices; vertex++)
    {
        uint64_t hidden_vertex = FIRST_BIT >> vertex;
        if (!find_hists_hd(input_graph, hidden_vertex, output, true, run_data))
        {
            return false;
        }
    }

    return true;
}

bool is_hypohist(Graph *input_graph, Output *output, bool only_partials, RunData *run_data)
{
    if (run_data == NULL)
    {
        fprintf(stderr, "No RunData struct provided.\n");
        exit(EXIT_FAILURE);
    }

    if (only_partials)
        return is_hypohist_partials(input_graph, output, run_data);
    else
        return !find_hists(input_graph, NULL, true, run_data) && is_hypohist_partials(input_graph, output, run_data);
}
