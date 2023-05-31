#define _GNU_SOURCE
#include <histg_lib.h>
#include <stdlib.h>
#include <adjlist.h>
#include <string.h>

typedef struct WVertex WVertex;

typedef struct WVertex
{
    int index;
    int label;
    int degree;
    WVertex **neighbours;
} WVertex;

typedef struct WEdge
{
    int label_a;
    int label_b;
    int number;
} WEdge;

int edge_number(int label_a, int label_b)
{
    return (label_a * (label_a - 1)) / 2 + label_b;
}

int total_edges(int nb_vertices)
{
    return nb_vertices * (nb_vertices - 1) / 2;
}

WEdge edge_new(int label_a, int label_b)
{
    WEdge edge;
    edge.label_a = label_a;
    edge.label_b = label_b;
    edge.number = edge_number(label_a, label_b);
    return edge;
}

typedef struct EdgeSetNode EdgeSetNode;

typedef struct EdgeSetNode
{
    WEdge edge;
    EdgeSetNode *next;
} EdgeSetNode;

typedef struct EdgeSet
{
    int label_a;
    int label_b;
    int number;
    EdgeSetNode *first;
    EdgeSetNode *last;
} EdgeSet;

EdgeSet es_new(int label_a, int label_b)
{
    EdgeSet edge_set;
    edge_set.label_a = label_a;
    edge_set.label_b = label_b;
    edge_set.number = edge_number(label_a, label_b);
    edge_set.first = NULL;
    edge_set.last = NULL;
    return edge_set;
}

void print_edge_set(EdgeSet *edge_set, bool print_fl)
{
    printf("Edgeset ( %d, %d):", edge_set->label_a, edge_set->label_b);

    if (print_fl)
    {
        printf(" f: ");
        if (edge_set->first == NULL)
        {
            printf("NULL");
        }
        else
        {
            printf("(%d, %d)", edge_set->first->edge.label_a, edge_set->first->edge.label_b);
        }

        printf(" l: ");
        if (edge_set->last == NULL)
        {
            printf("NULL");
        }
        else
        {
            printf("(%d, %d)", edge_set->last->edge.label_a, edge_set->last->edge.label_b);
        }
    }

    printf(" s:{");
    EdgeSetNode *node = edge_set->first;
    while (node != NULL)
    {
        WEdge edge = node->edge;
        printf(" (%d, %d)", edge.label_a, edge.label_b);

        node = node->next;
    }
    printf("}");
}

void es_add_edge(EdgeSet *es, WEdge edge)
{
    EdgeSetNode *node = malloc(sizeof(EdgeSetNode));
    node->edge = edge;
    node->next = NULL;

    EdgeSetNode *start_node = es->first;

    if (start_node == NULL)
    {
        es->first = node;
        es->last = node;
    }
    else
    {
        EdgeSetNode *last_node = es->last;

        last_node->next = node;
        es->last = node;
    }
}

void es_add_edge_set(EdgeSet *es, EdgeSet *es_to_add)
{
    EdgeSetNode *first_to_add = es_to_add->first;
    EdgeSetNode *last_to_add = es_to_add->last;
    EdgeSetNode *last_node = es->last;

    if (last_node == NULL)
    {
        es->first = first_to_add;
        es->last = last_to_add;
    }
    else
    {
        es->last->next = first_to_add;
        es->last = last_to_add;
    }
}

void es_clear(EdgeSet *es)
{
    es->first = NULL;
    es->last = NULL;
}

typedef struct EdgeSetListNode EdgeSetListNode;

typedef struct EdgeSetListNode
{
    EdgeSetListNode *previous;
    EdgeSetListNode *next;
    EdgeSet *edge_set;
} EdgeSetListNode;

typedef struct EdgeSetList
{
    int label;
    EdgeSetListNode *first;
    EdgeSetListNode *last;
    EdgeSetListNode *rank_node;
    EdgeSetListNode *max_node;
} EdgeSetList;

EdgeSetList esl_new(int label)
{
    EdgeSetList edge_set_list;
    edge_set_list.first = NULL;
    edge_set_list.last = NULL;
    edge_set_list.label = label;
    edge_set_list.rank_node = NULL;
    edge_set_list.max_node = NULL;
    return edge_set_list;
}

void esl_add_edgeset(EdgeSetList *edge_set_list, EdgeSet *edge_set)
{
    EdgeSetListNode *node = malloc(sizeof(EdgeSetListNode));
    node->edge_set = edge_set;
    node->previous = NULL;
    node->next = NULL;

    if (edge_set_list->last == NULL)
    {
        edge_set_list->first = node;
        edge_set_list->last = node;
    }
    else
    {
        EdgeSetListNode *end_node = edge_set_list->last;
        end_node->next = node;
        node->previous = end_node;
        edge_set_list->last = node;
    }

    if (edge_set_list->max_node == NULL || edge_set_list->max_node->edge_set->label_b < node->edge_set->label_b)
    {
        edge_set_list->max_node = node;
    }
}

void esl_remove_edgeset(EdgeSetList *esl, EdgeSet *edge_set)
{
    EdgeSetListNode *node = esl->first;
    while (node != NULL && node->edge_set != edge_set)
        node = node->next;

    if (node->previous == NULL)
    {
        esl->first = node->next;
        esl->first->previous = NULL;
    }
    else if (node->next == NULL)
    {
        esl->last = node->previous;
        esl->last->next = NULL;
    }
    else
    {
        node->previous->next = node->next;
        node->next->previous = node->previous;
    }

    free(node);
}

typedef union WStackElement
{
    EdgeSetNode *esn;
    EdgeSetListNode *esln;
} WStackElement;

typedef struct WStack
{
    unsigned int size;
    unsigned int capacity;
    WStackElement *stack;
} WStack;

WStack wstack_new(int nb_vertices)
{
    WStack stack;
    stack.size = 0;
    stack.capacity = total_edges(nb_vertices);
    stack.stack = calloc(stack.capacity, sizeof(WStackElement));
    return stack;
}

void stack_push(WStack *stack, WStackElement value)
{
    if (stack->capacity <= stack->size)
    {
        fprintf(stderr, "Stack size should never reach capacity.\n");
        exit(EXIT_FAILURE);
    }

    stack->stack[stack->size++] = value;
}

WStackElement stack_pop(WStack *stack)
{
    return stack->stack[--stack->size];
}

typedef struct WGraph
{
    // Static value with the number of vertices in the graph
    int nb_vertices;
    // Array with the vertices, stored statically for easy pointer access
    WVertex *vertices;
    // Array where the index of an element corresponds to its labeling
    WVertex **labeling;

    EdgeSet *edge_sets;

    EdgeSetList *edge_set_lists;
    // Number of completed contractions
    int contractions;

    EdgeSet **contracted_sets;

    WStack stack;
} WGraph;

void free_wgraph(WGraph *graph)
{
    // Free vertices
    for (int i = 0; i < graph->nb_vertices; i++)
    {
        free(graph->vertices[i].neighbours);
    }
    free(graph->vertices);

    // Free edge sets
    for (int i = 0; i < total_edges(graph->nb_vertices); i++)
    {
        EdgeSet edge_set = graph->edge_sets[i];
        EdgeSetNode *node = edge_set.first;

        while (node != NULL)
        {
            EdgeSetNode *old_node = node;
            node = node->next;
            free(old_node);
        }
    }
    free(graph->edge_sets);

    // Free edge set lists
    for (int i = 0; i < graph->nb_vertices; i++)
    {
        EdgeSetList esl = graph->edge_set_lists[i];
        EdgeSetListNode *node = esl.first;

        while (node != NULL)
        {
            EdgeSetListNode *old_node = node;
            node = node->next;
            free(old_node);
        }
    }
    free(graph->edge_set_lists);

    // Free leftover arrays
    free(graph->stack.stack);
    free(graph->contracted_sets);
    free(graph->labeling);
    free(graph);
}

void generate_labeling(WGraph *graph)
{
    graph->labeling = calloc(graph->nb_vertices, sizeof(WVertex *));

    int nb_labeled_vertices = 0;
    uint64_t labeled_vertices = 0;

    int largest_degree = 0;
    WVertex *largest_vertex = NULL;

    // Find largest vertex to start
    for (int i = 0; i < graph->nb_vertices; i++)
    {
        WVertex *vertex = &graph->vertices[i];

        if (vertex->degree > largest_degree)
        {
            largest_degree = vertex->degree;
            largest_vertex = vertex;
        }
    }

    // Add largest vertex as first labeled vertex
    graph->labeling[nb_labeled_vertices] = largest_vertex;
    largest_vertex->label = nb_labeled_vertices;
    nb_labeled_vertices++;
    labeled_vertices |= (FIRST_BIT >> largest_vertex->index);

    // Continue adding labelings from the neighbours of already labeled vertices
    for (; nb_labeled_vertices < graph->nb_vertices; nb_labeled_vertices++)
    {
        largest_degree = 0;
        largest_vertex = NULL;

        // Find vertex with largest degree neighbouring an already labeled vertex
        for (int i = 0; i < nb_labeled_vertices; i++)
        {
            WVertex *labeled_vertex = graph->labeling[i];

            for (int j = 0; j < labeled_vertex->degree; j++)
            {
                WVertex *neighbour = labeled_vertex->neighbours[j];

                // Don't check vertices that already have a labeling
                if ((FIRST_BIT >> neighbour->index) & labeled_vertices)
                    continue;

                if (neighbour->degree > largest_degree)
                {
                    largest_degree = neighbour->degree;
                    largest_vertex = neighbour;
                }
            }
        }

        // Add labeling to vertex with now largest degree
        graph->labeling[nb_labeled_vertices] = largest_vertex;
        largest_vertex->label = nb_labeled_vertices;
        labeled_vertices |= (FIRST_BIT >> largest_vertex->index);
    }
}

void print_labeling(WGraph *graph)
{
    printf("Labeling:\n");
    for (int i = 0; i < graph->nb_vertices; i++)
    {
        WVertex *vertex = graph->labeling[i];

        printf("Label:  %d, index:  %d\n", i, vertex->index);
    }
}

void initialize_edges(WGraph *graph)
{
    int nb_edges = total_edges(graph->nb_vertices);

    // Allocate arrays
    graph->edge_sets = calloc(nb_edges, sizeof(EdgeSet));
    graph->edge_set_lists = calloc(graph->nb_vertices, sizeof(EdgeSetList));

    // Initialize arrays
    for (int label_a = 1; label_a < graph->nb_vertices; label_a++)
    {
        for (int label_b = 0; label_b < label_a; label_b++)
        {
            EdgeSet new_set = es_new(label_a, label_b);
            graph->edge_sets[new_set.number] = new_set;
        }
    }

    for (int i = 0; i < graph->nb_vertices; i++)
    {
        graph->edge_set_lists[i] = esl_new(i);
    }

    // Initialize array data
    for (int a_label = 1; a_label < graph->nb_vertices; a_label++)
    {
        WVertex *a_vertex = graph->labeling[a_label];

        EdgeSetList *esl = &graph->edge_set_lists[a_label];

        for (int i = 0; i < a_vertex->degree; i++)
        {
            WVertex *b_vertex = a_vertex->neighbours[i];
            int b_label = b_vertex->label;

            if (a_label < b_label)
                continue;

            WEdge edge = edge_new(a_label, b_label);
            int edge_nb = edge.number;

            EdgeSet *edge_set = &graph->edge_sets[edge_nb];

            es_add_edge(edge_set, edge);
            esl_add_edgeset(esl, edge_set);
        }
    }
}

WGraph *construct_wgraph(Graph *input_graph)
{
    WGraph *wgraph = malloc(sizeof(WGraph));
    wgraph->nb_vertices = input_graph->vertices;
    wgraph->vertices = calloc(wgraph->nb_vertices, sizeof(WVertex));
    wgraph->contractions = 0;
    wgraph->contracted_sets = calloc(wgraph->nb_vertices, sizeof(EdgeSet *));

    // Set up WVertices
    for (int i = 0; i < input_graph->vertices; i++)
    {
        WVertex *w_vertex = &wgraph->vertices[i];
        w_vertex->index = i;

        uint64_t vertex_neighbours = input_graph->adjacency_matrix[i];

        w_vertex->degree = vertex_degree(vertex_neighbours);
        w_vertex->neighbours = calloc(w_vertex->degree, sizeof(WVertex *));

        // Set up neighbours for each WVertex
        int added_neighbours = 0;
        for (int j = 0; j < input_graph->vertices; j++)
        {
            uint64_t neighbour = FIRST_BIT >> j;
            if (neighbour & vertex_neighbours)
            {
                w_vertex->neighbours[added_neighbours] = &wgraph->vertices[j];
                added_neighbours++;
            }
        }
    }

    // Generate labeling
    generate_labeling(wgraph);
    // Initialize edges
    initialize_edges(wgraph);

    wgraph->stack = wstack_new(wgraph->nb_vertices);

    return wgraph;
}

void print_edge_sets(WGraph *graph)
{
    for (int edge_nb = 0; edge_nb < total_edges(graph->nb_vertices); edge_nb++)
    {
        EdgeSet set = graph->edge_sets[edge_nb];

        print_edge_set(&set, false);
    }
}

void print_edge_set_list_node(EdgeSetListNode *node)
{
    EdgeSet *edge_set = node->edge_set;
    printf("  ");
    print_edge_set(edge_set, false);
}

void print_edge_set_list(EdgeSetList *list)
{
    EdgeSetListNode *node = list->first;

    printf("ESL  %d:\n", node->edge_set->label_a);

    while (node != NULL)
    {
        print_edge_set_list_node(node);
        node = node->next;
    }

    printf("\n");
}

void print_edge_set_lists(WGraph *graph)
{
    for (int label_a = 1; label_a < graph->nb_vertices; label_a++)
    {
        EdgeSetList *list = &graph->edge_set_lists[label_a];
        print_edge_set_list(list);
    }
}

void count_trees(WGraph *graph, unsigned long long int *nb_trees)
{
    int total_trees = 1;

    for (int i = graph->nb_vertices - 1; i > 0; i--)
    {
        EdgeSet *set = graph->contracted_sets[i];

        EdgeSetNode *node = set->first;
        int edges = 0;
        while (node != NULL)
        {
            edges += 1;
            node = node->next;
        }

        total_trees *= edges;
    }

    *nb_trees += total_trees;
}

void count_trees_produced_parts(WGraph *graph, Graph *tree, int i, unsigned long long int *nb_trees)
{
    if (i == 0)
    {
        *nb_trees += 1;
        return;
    }

    EdgeSet *set = graph->contracted_sets[i];
    EdgeSetNode *node = set->first;

    while (node != NULL)
    {
        WEdge wedge = node->edge;
        Edge edge;
        edge.origin = graph->labeling[wedge.label_a]->index;
        edge.destination = graph->labeling[wedge.label_b]->index;

        add_edge_to_graph(tree, &edge);
        count_trees_produced_parts(graph, tree, i - 1, nb_trees);
        remove_edge_from_graph(tree, &edge);

        node = node->next;
    }
}

void count_trees_produced(WGraph *graph, unsigned long long int *nb_trees)
{
    Graph *tree = empty_graph(graph->nb_vertices);
    count_trees_produced_parts(graph, tree, graph->nb_vertices - 1, nb_trees);
    free_graph(tree);
}

bool is_hist(Graph *potential_hist)
{
    if (potential_hist->edges != potential_hist->vertices - 1)
    {
        printf("Number of edges is incorrect. This should never happen.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < potential_hist->vertices; i++)
    {
        uint64_t adjacencies = potential_hist->adjacency_matrix[i];
        if (vertex_degree(adjacencies) == 2)
            return false;
    }

    return true;
}

void count_hists_parts(WGraph *graph, Graph *potential_hist, int i, unsigned long long int *nb_hists)
{
    if (i == 0)
    {
        if (is_hist(potential_hist))
            *nb_hists += 1;
        return;
    }

    EdgeSet *set = graph->contracted_sets[i];
    EdgeSetNode *node = set->first;

    while (node != NULL)
    {
        WEdge wedge = node->edge;
        Edge edge;
        edge.origin = graph->labeling[wedge.label_a]->index;
        edge.destination = graph->labeling[wedge.label_b]->index;

        add_edge_to_graph(potential_hist, &edge);
        count_hists_parts(graph, potential_hist, i - 1, nb_hists);
        remove_edge_from_graph(potential_hist, &edge);

        node = node->next;
    }
}

void count_hists(WGraph *graph, unsigned long long int *nb_hists)
{
    Graph *potential_hist = empty_graph(graph->nb_vertices);
    count_hists_parts(graph, potential_hist, graph->nb_vertices - 1, nb_hists);
    free_graph(potential_hist);
}

void print_contracted_sets(WGraph *graph)
{
    printf("Contracted sets: ");

    for (int i = graph->nb_vertices - 1; i > 0; i--)
    {
        EdgeSet *set = graph->contracted_sets[i];

        EdgeSetNode *node = set->first;
        while (node != NULL)
        {
            node = node->next;
        }

        print_edge_set(set, false);
        printf("; ");
    }

    printf("\n");
}

void print_contracted_sets_g6_parts(WGraph *graph, Graph *tree, int i)
{
    if (i == 0)
    {
        print_graph_to_output_as_graph6(stdout, tree);
        return;
    }

    EdgeSet *set = graph->contracted_sets[i];
    EdgeSetNode *node = set->first;

    while (node != NULL)
    {
        WEdge wedge = node->edge;
        Edge edge;
        edge.origin = graph->labeling[wedge.label_a]->index;
        edge.destination = graph->labeling[wedge.label_b]->index;

        add_edge_to_graph(tree, &edge);
        print_contracted_sets_g6_parts(graph, tree, i - 1);
        remove_edge_from_graph(tree, &edge);

        node = node->next;
    }
}

void print_contracted_sets_g6(WGraph *graph)
{
    Graph *tree = empty_graph(graph->nb_vertices);
    print_contracted_sets_g6_parts(graph, tree, graph->nb_vertices - 1);
    free(tree);
}

void esl_rearrange(EdgeSetList *esl, EdgeSetListNode *rnk)
{
    // Pop rnk out of the double linked list

    // rnk is the only element in the list, no point in rearranging
    if (rnk->previous == NULL && rnk->next == NULL)
        return;

    // rnk is last in the list
    if (rnk->next == NULL) // && rnk->previous != NULL
    {
        EdgeSetListNode *previous = rnk->previous;
        esl->last = previous;
        previous->next = NULL;
    }
    // rnk is first in the list
    else if (rnk->previous == NULL) // && rnk->next != NULL
    {
        EdgeSetListNode *next = rnk->next;
        esl->first = next;
        next->previous = NULL;
    }
    else // rnk->next != NULL && rnk-> previous != NULL
    {
        EdgeSetListNode *next = rnk->next;
        EdgeSetListNode *previous = rnk->previous;
        next->previous = previous;
        previous->next = next;
    }

    // Place rnk in the correct position

    // rnk is largest so pop it in the back
    if (rnk == esl->max_node)
    {
        rnk->previous = esl->last;
        esl->last = rnk;
        rnk->previous->next = rnk;
        rnk->next = NULL;
    }
    // Place rmk before the previously processed set
    else
    {
        rnk->previous = esl->rank_node->previous;
        if (rnk->previous != NULL)
            rnk->previous->next = rnk;
        else
            esl->first = rnk;

        rnk->next = esl->rank_node;
        rnk->next->previous = rnk;
    }

    // rnk now becomes the last processed node
    esl->rank_node = rnk;
}

void scan_contract(WGraph *graph, EdgeSetList *eenk, EdgeSetListNode *rnk)
{
    int rnk_val = rnk->edge_set->label_b;

    EdgeSetListNode *enki = eenk->first;
    while (enki != NULL && enki != rnk)
    {
        int i = enki->edge_set->label_b;
        EdgeSet *ernki = &graph->edge_sets[edge_number(rnk_val, i)];

        // Set not empty
        if (ernki->last != NULL)
        {
            WStackElement val;
            val.esn = ernki->last;
            stack_push(&graph->stack, val);
            es_add_edge_set(ernki, enki->edge_set);
        }
        // Set empty
        else
        {
            EdgeSetList *eernk = &graph->edge_set_lists[rnk_val];
            EdgeSetListNode *max_value_node = eernk->max_node;

            es_add_edge_set(ernki, enki->edge_set);
            esl_add_edgeset(eernk, ernki);

            WStackElement val;
            val.esln = max_value_node;
            stack_push(&graph->stack, val);
        }

        enki = enki->next;
    }
}

void scan_restore(WGraph *graph, EdgeSetListNode **rnk_ptr)
{
    EdgeSetListNode *rnk = *rnk_ptr;

    int rnk_val = rnk->edge_set->label_b;
    EdgeSetListNode *enki = rnk->previous;

    int max_i = -1;
    EdgeSetListNode *max_i_node = NULL;

    while (enki != NULL)
    {
        int i = enki->edge_set->label_b;
        EdgeSet *enki_set = enki->edge_set;
        EdgeSet *ernki_set = &graph->edge_sets[edge_number(rnk_val, i)];

        EdgeSetNode *enki_first = enki_set->first;
        EdgeSetNode *ernki_first = ernki_set->first;

        if (enki_first != ernki_first)
        {
            WStackElement val = stack_pop(&graph->stack);
            EdgeSetNode *last = val.esn;
            if (last == NULL)
            {
                fprintf(stderr, "Last should not be NULL.\n");
                exit(EXIT_FAILURE);
            }
            ernki_set->last = last;
            last->next = NULL;
        }
        else
        {
            EdgeSetList *eernk = &graph->edge_set_lists[rnk_val];
            esl_remove_edgeset(eernk, ernki_set);
            WStackElement val = stack_pop(&graph->stack);
            EdgeSetListNode *eernk_max = val.esln;
            eernk->max_node = eernk_max;
            es_clear(ernki_set);
        }

        if (i < rnk_val && i > max_i)
        {
            max_i = i;
            max_i_node = enki;
        }

        enki = enki->previous;
    }

    *rnk_ptr = max_i_node;
}

void contract(WGraph *graph, unsigned long long int *nb_trees, bool find_hists, bool produce_trees)
{
    // Next vertex to contract is vertex n-k
    int nk = graph->nb_vertices - graph->contractions - 1; // n - k

    // If we're at vertex 1, there's only one possible contraction left (1 to 0),
    // so we stop and we output
    if (nk == 1)
    {
        graph->contracted_sets[nk] = &graph->edge_sets[0];
        if (find_hists)
            count_hists(graph, nb_trees);
        else if (produce_trees)
            count_trees_produced(graph, nb_trees);
        else
            count_trees(graph, nb_trees);
        return;
    }

    // Otherwise we contract on every edge indicent to 'n-k'

    // Edges indicent to vertex 'n-k' are contained in edgesetlist EE(n-k)
    EdgeSetList *eenk = &graph->edge_set_lists[nk]; // EE(n - k)

    // Keep track of the current edgeset we're contracting
    EdgeSetListNode *rnk_node = eenk->max_node;

    while (rnk_node != NULL)
    {
        int rnk = rnk_node->edge_set->label_b; // r_n-k

        esl_rearrange(eenk, rnk_node);
        scan_contract(graph, eenk, rnk_node);

        EdgeSet *contracted_set = &graph->edge_sets[edge_number(nk, rnk)];
        graph->contracted_sets[nk] = contracted_set;

        graph->contractions++;
        contract(graph, nb_trees, find_hists, produce_trees);
        graph->contractions--;

        scan_restore(graph, &rnk_node);
    }
}

unsigned long long int winter(Graph *graph, bool find_hists, bool produce_trees)
{
    WGraph *wgraph = construct_wgraph(graph);
    unsigned long long int nb_trees = 0;
    contract(wgraph, &nb_trees, find_hists, produce_trees);
    free_wgraph(wgraph);
    return nb_trees;
}

int main(int argc, char *argv[])
{
    char *line = NULL;
    size_t length = 0;
    size_t read;

    bool find_hists = false;
    bool produce_trees = false;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "hist") == 0)
            find_hists = true;

        if (strcmp(argv[i], "out") == 0)
            produce_trees = true;
    }

    if (find_hists)
        produce_trees = false;

    RunData *histg_data = rd_new();

    Timer timer;
    double histg_time = 0;
    double winter_time = 0;

    unsigned long long int nb_graphs = 0;
    unsigned long long int nb_trees = 0;

    while ((read = getline(&line, &length, stdin)) != -1)
    {
        Graph *graph = malloc(sizeof(Graph));
        parse_graph6_line(line, graph);

        nb_graphs++;

        // Time winter's algorithm
        start_timer(&timer);
        unsigned long long int winter_nb = winter(graph, find_hists, produce_trees);
        end_timer(&timer);
        winter_time += elapsed_time_seconds(&timer);

        // Time histg implementation
        start_timer(&timer);
        unsigned long long int histg_nb;
        if (find_hists)
        {
            find_hists_alg(graph, 0, NULL, false, histg_data);
            histg_nb = histg_data->hists_this_run;
        }
        else
        {
            histg_nb = find_spanning_trees(graph, NULL, false);
        }

        end_timer(&timer);
        histg_time += elapsed_time_seconds(&timer);

        if (histg_nb != winter_nb)
        {
            fprintf(stderr, "Number of found trees does not match. Graph: \n");
            fprintf(stderr, "Winter: %llu, histg: %llu\n", winter_nb, histg_nb);
            print_graph_to_output_as_graph6(stderr, graph);
            exit(EXIT_FAILURE);
        }

        nb_trees += winter_nb;

        free_graph(graph);
    }

    printf("Graphs: %llu", nb_graphs);
    printf(find_hists ? ", hists: " : ", trees: ");
    printf("%llu,", nb_trees);
    printf(" winter time: %lf, histg time: %lf\n", winter_time, histg_time);

    free(line);
    free(histg_data);
}
