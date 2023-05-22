#include <histg_lib.h>
#include <time.h>
#include <stdio.h>

void start_timer(Timer *timer)
{
    timer->start = clock();
}

void end_timer(Timer *timer)
{
    timer->end = clock();
}

double elapsed_time_seconds(Timer *timer)
{
    return (double)(timer->end - timer->start) / CLOCKS_PER_SEC;
}

void print_elapsed_time_to_output(FILE *output, Timer *timer)
{
    char unit = 0;
    double time = elapsed_time_seconds(timer);

    if (time < 1)
    {
        time /= 1000.0;
        unit = 'm';
    }
    if (time < 1)
    {
        time /= 1000.0;
        unit = 'u';
    }

    if (unit)
    {
        fprintf(output, "%f%cs\n", time, unit);
    }
    else
    {
        fprintf(output, "%fs\n", time);
    }
}