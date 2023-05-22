#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <math.h>
#include <string.h>

#include <histg_lib.h>
#include <kirchhoff.h>
#include <adjlist.h>

const char *argp_program_version = "histg 0.1.0";
const char *argp_program_bug_address = "<awouters.andreas@gmail.com>";

// Program documentation
static char doc[] = "histg -- a program for finding homeomorphically irreducible spanning trees";
static char args_doc[] = "There are no mandatory arguments";
int arg_count = 0;

// Program options / command line arguments
static struct argp_option options[] = {
    {"input", 'i', "FILE", 0, "input file to read"},
    {"output", 'o', "FILE", 0, "output to file instead of standard output"},
    {"quiet", 'q', 0, 0, "suppress output of number of found trees, only useful combined with --enumerate"},
    {"hist", 'h', 0, 0, "Calculate homeomorphically irreducible spanning trees, this is the default option"},
    {"spanning", 's', 0, 0, "Calculate regular spanning trees instead of homeomorphically irreducible spanning trees"},
    {"hypohist", 'y', 0, 0, "Calculate wether the graph is hypohisterian or not"},
    {"enumerate", 'e', "FILE", OPTION_ARG_OPTIONAL, "Output calculated trees to given file or default output otherwise"},
    {"positives", 'p', 0, 0, "Only output when the number of found spanning trees/hists/hypohists is at least one"},
    {"negatives", 'n', 0, 0, "Only output when the number of found spanning trees/hists/hypohists is zero"},
    {"boolean", 'b', 0, 0, "Don't count but stop searching for more trees/hists when one has been found, output is either 0 or 1."},
    {"timing", 't', 0, 0, "Output cpu calculation time in seconds"},
    {"csv_header", 'c', 0, 0, "Print csv header"},
    {"graph-echo", 'g', 0, 0, "Echo read graph to output in Graph6 format"},
    {"output_format", 'f', "Format", 0, "output format. options: g6 (Graph6), am (Adjacency matrix), al (Adjacency list). Graph6 by default"},
    {0},
};

struct arguments
{
    bool quiet, enumerate, positives, negatives;
    bool spanning, hist, hypohist;
    bool timing, header, echo;
    bool boolean;
    char *output_file;
    char *input_file;
    char *enumerate_file;
    Format format;
};

// Parse a single argument
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key)
    {
    case 'i':
        arguments->input_file = arg;
        break;
    case 'o':
        arguments->output_file = arg;
        break;
    case 'q':
        arguments->quiet = true;
        break;
    case 'h':
        arguments->hist = true;
        break;
        break;
    case 's':
        arguments->spanning = true;
        break;
    case 'y':
        arguments->hypohist = true;
        break;
    case 'e':
        arguments->enumerate = true;
        arguments->enumerate_file = arg;
        break;
    case 'p':
        arguments->positives = true;
        break;
    case 'n':
        arguments->negatives = true;
        break;
    case 'b':
        arguments->boolean = true;
    case 't':
        arguments->timing = true;
        break;
    case 'c':
        arguments->header = true;
        break;
    case 'g':
        arguments->echo = true;
        break;
    case 'f':
        if (strcmp(arg, "g6") == 0)
            arguments->format = Graph6;
        else if (strcmp(arg, "am") == 0)
            arguments->format = AdjacencyMatrix;
        else if (strcmp(arg, "al") == 0)
            arguments->format = AdjacencyList;
        else
            arguments->format = Graph6;
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num >= arg_count)
            argp_usage(state);

        break;

    case ARGP_KEY_END:
        if (state->arg_num < arg_count)
            argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

void print_header(struct arguments *arguments, FILE *output)
{
    if (arguments->header)
    {
        if (arguments->echo)
            fprintf(output, "graph,");

        if (arguments->spanning)
        {
            fprintf(output, "spanning_trees");
            if (arguments->timing)
                fprintf(output, ",spanning_trees_timing");
        }

        if (arguments->hist)
        {
            if (arguments->spanning)
                fprintf(output, ",");

            fprintf(output, "hists");
            if (arguments->timing)
                fprintf(output, ",hists_timing");
        }

        if (arguments->hypohist)
        {
            if (arguments->spanning || arguments->hist)
                fprintf(output, ",");

            fprintf(output, "hypohist");
        }

        fprintf(output, "\n");
    }
}

bool should_print(struct arguments *arguments, unsigned long long int nb_spanning_trees, unsigned long long int nb_hists, unsigned long long int nb_hypohists)
{
    if (arguments->quiet)
        return false;

    if (arguments->positives && arguments->negatives)
        return true;

    if (arguments->positives && ((arguments->spanning && nb_spanning_trees > 0) || (arguments->hist && nb_hists > 0) || (arguments->hypohist && nb_hypohists > 0)))
        return true;

    if (arguments->negatives && ((arguments->spanning && nb_spanning_trees == 0) || (arguments->hist && nb_hists == 0) || (arguments->hypohist && nb_hypohists == 0)))
        return true;

    return false;
}

int main(int argc, char *argv[])
{
    struct arguments arguments = {0};
    arguments.format = Graph6;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (!arguments.hist && (!arguments.spanning && !arguments.hypohist))
        arguments.hist = true;

    if (!arguments.positives && !arguments.negatives)
        arguments.positives = arguments.negatives = true;

    FILE *input_file = stdin;

    Output standard_output;
    standard_output.output_file = stdout;
    standard_output.format = arguments.format;

    Output enumerate_output;
    enumerate_output.output_file = NULL;
    enumerate_output.format = arguments.format;
    Output *enumerate_output_address = &enumerate_output;

    if (arguments.input_file)
    {
        input_file = fopen(arguments.input_file, "r");
        if (input_file == NULL)
        {
            fprintf(stderr, "Input file opening failed.\n");
            return EXIT_FAILURE;
        }
    }

    if (arguments.output_file)
    {
        standard_output.output_file = fopen(arguments.output_file, "w");
        if (standard_output.output_file == NULL)
        {
            fprintf(stderr, "Output file opening failed.\n");
            return EXIT_FAILURE;
        }
    }

    if (arguments.enumerate)
    {
        if (arguments.enumerate_file)
        {
            enumerate_output.output_file = fopen(arguments.enumerate_file, "w");
            if (enumerate_output.output_file == NULL)
            {
                fprintf(stderr, "Enumerate file opening failed.\n");
                return EXIT_FAILURE;
            }
        }
        else
        {
            enumerate_output.output_file = standard_output.output_file;
        }
    }
    else
    {
        enumerate_output_address = NULL;
    }

    char *line = NULL;
    size_t length = 0;
    size_t read;

    Timer timer;

    unsigned long long int read_graphs = 0;
    unsigned long long int total_nb_spanning_trees = 0;
    unsigned long long int total_nb_hists = 0;
    unsigned long long int total_nb_hypohists = 0;
    Timer full_program_timer;

    print_header(&arguments, standard_output.output_file);

    start_timer(&full_program_timer);

    while ((read = getline(&line, &length, input_file)) != -1)
    {
        char output_str[1024] = "";
        char output_str_temp[512] = "";

        Graph *graph = malloc(sizeof(Graph));

        if (graph == NULL)
        {
            fprintf(stderr, "Failed to allocate graph in main loop\n");
            exit(EXIT_FAILURE);
        }

        parse_graph6_line(line, graph);
        read_graphs++;

        unsigned long long int nb_spanning_trees = 0;
        unsigned long long int nb_hists = 0;
        int is_hypoh = 0;

        if (arguments.echo)
        {
            char *g6string = get_graph6_string(graph);
            sprintf(output_str_temp, "%s,", g6string);
            strcat(output_str, output_str_temp);
            free(g6string);
        }

        if (arguments.spanning)
        {
            if (arguments.enumerate)
            {
                start_timer(&timer);
                nb_spanning_trees = find_spanning_trees(graph, enumerate_output_address, arguments.boolean);
                end_timer(&timer);
            }
            else
            {
                start_timer(&timer);
                nb_spanning_trees = kirchhoff(graph);
                end_timer(&timer);
            }

            total_nb_spanning_trees += nb_spanning_trees;

            sprintf(output_str_temp, "%llu", nb_spanning_trees);
            strcat(output_str, output_str_temp);

            if (arguments.timing)
            {
                sprintf(output_str_temp, ",%lf", elapsed_time_seconds(&timer));
                strcat(output_str, output_str_temp);
            }
        }

        if (arguments.hist)
        {
            start_timer(&timer);
            if (arguments.boolean)
            {
                RunData run_data;
                find_hists(graph, enumerate_output_address, arguments.boolean, &run_data);
                nb_hists = run_data.hists_this_run;
            }
            else
            {
                RunData run_data;
                find_hists_alg(graph, 0, enumerate_output_address, arguments.boolean, &run_data);
                nb_hists = run_data.hists_this_run;
            }
            end_timer(&timer);

            if (arguments.spanning)
                strcat(output_str, ",");

            total_nb_hists += nb_hists;

            sprintf(output_str_temp, "%llu", nb_hists);
            strcat(output_str, output_str_temp);

            if (arguments.timing)
            {
                sprintf(output_str_temp, ",%lf", elapsed_time_seconds(&timer));
                strcat(output_str, output_str_temp);
            }

            if (arguments.hypohist)
            {
                if (nb_hists == 0)
                {
                    RunData run_data;
                    is_hypoh = is_hypohist_partials(graph, enumerate_output_address, &run_data);
                }

                total_nb_hypohists += is_hypoh;

                sprintf(output_str_temp, ",%i", is_hypoh);
                strcat(output_str, output_str_temp);
            }
        }
        else if (arguments.hypohist)
        {
            RunData run_data;
            is_hypoh = is_hypohist(graph, enumerate_output_address, false, &run_data);

            total_nb_hypohists += is_hypoh;

            sprintf(output_str_temp, "%i", is_hypoh);
            strcat(output_str, output_str_temp);
        }

        strcat(output_str, "\n");

        if (should_print(&arguments, nb_spanning_trees, nb_hists, is_hypoh))
            fputs(output_str, standard_output.output_file);

        free_graph(graph);
    }

    end_timer(&full_program_timer);

    fprintf(stderr, "Found");

    if (arguments.spanning)
        fprintf(stderr, " %llu spanning trees", total_nb_spanning_trees);

    if (arguments.spanning && arguments.hist)
        fprintf(stderr, ",");

    if (arguments.hist)
        fprintf(stderr, " %llu hists", total_nb_hists);

    if ((arguments.spanning || arguments.hist) && arguments.hypohist)
        fprintf(stderr, ",");

    if (arguments.hypohist)
        fprintf(stderr, " %llu hypohists", total_nb_hypohists);

    fprintf(stderr, " for %llu graphs in %lf seconds\n", read_graphs, elapsed_time_seconds(&full_program_timer));

    exit(EXIT_SUCCESS);
}
