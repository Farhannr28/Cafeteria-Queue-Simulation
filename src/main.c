/* External definitions for job-shop model. */

#include "SIMLIB/simlib.h" /* Required for use of simlib.c. */
#include "assert.h"

#define EVENT_ARRIVAL 1        /* Event type for arrival of a job to the \
                  system. */
#define EVENT_DEPARTURE 2      /* Event type for departure of a job from a \
                  particular station. */
#define EVENT_END_SIMULATION 3 /* Event type for end of the simulation. */
#define STREAM_INTERARRIVAL 1  /* Random-number stream for interarrivals. */
#define STREAM_JOB_TYPE 2      /* Random-number stream for job types. */
#define STREAM_SERVICE 3       /* Random-number stream for service times. */
#define MAX_NUM_STATIONS 4     /* Maximum number of stations. */
#define MAX_NUM_JOB_TYPES 3    /* Maximum number of job types. */
#define MAX_NUM_CASHIERS 3     /* Maximum number of cashiers */

/* Declare non-simlib global variables. */

int num_stations, num_job_types, i, j, num_machines[MAX_NUM_STATIONS + 1],
    num_tasks[MAX_NUM_JOB_TYPES + 1],
    route[MAX_NUM_JOB_TYPES + 1][MAX_NUM_STATIONS + 1], num_machines_busy[MAX_NUM_STATIONS + 1],
    job_type, task, num_cashier_busy[MAX_NUM_CASHIERS + 1];
double mean_interarrival, length_simulation, prob_distrib_job_type[26],
    range_service[MAX_NUM_STATIONS + 1][2 + 1], range_accumulated_time[MAX_NUM_STATIONS + 1][2 + 1],
    accumulated_time;
FILE *infile, *outfile;

void arrive(int new_job) /* Function to serve as both an arrival event of a job
                to the system, as well as the non-event of a job's
                arriving to a subsequent station along its
                route. */
{
    int station;

    /* If this is a new arrival to the system, generate the time of the next
       arrival and determine the job type and task number of the arriving
       job. */

    if (new_job == 1)
    {

        event_schedule(sim_time + expon(mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL);
        job_type = random_integer(prob_distrib_job_type, STREAM_JOB_TYPE);
        task = 1;
        accumulated_time = 0;
    }

    /* Determine the station from the route matrix. */

    station = route[job_type][task];

    /* Check to see whether all machines in this station are busy. */

    if (num_machines[station] != -1 && num_machines_busy[station] == num_machines[station]) // TODO cek kl station 4
    {
        /* All machines in this station are busy, so place the arriving job at
           the end of the appropriate queue. Note that the following data are
           stored in the record for each job:
           1. Time of arrival to this station.
           2. Job type.
           3. Current task number. */

        // if station 4 & busy -> num_machines or ismachinebusy
        // check fewest queue -> variable sendiri
        // list file there -> station + 1/2

        if (station == 4)
        {
            int target_cashier = 1;
            for (int i = 2; i <= num_machines[4]; i++)
            {
                if (list_size[station + i] < list_size[station + target_cashier]) // check fewest queue
                {
                    target_cashier = i;
                }
            }

            // transfer[1] = sim_time;
            // transfer[2] = job_type;
            // transfer[3] = task;
            // transfer[5] = accumulated_time;
            // transfer[6] = target_cashier;
            // list_file(LAST, station);

            transfer[1] = sim_time;
            transfer[2] = job_type;
            transfer[3] = task;
            transfer[5] = accumulated_time;
            transfer[6] = target_cashier;
            list_file(LAST, station + target_cashier);

            return;
        }

        transfer[1] = sim_time;
        transfer[2] = job_type;
        transfer[3] = task;
        transfer[5] = accumulated_time;
        transfer[6] = -1;
        list_file(LAST, station);
    }

    else
    {

        /* A machine in this station is idle, so start service on the arriving
           job (which has a delay of zero). */

        int target_cashier = -1;
        if (station == 4)
        {
            for (int i = 1; i <= num_machines[4]; i++)
            {
                if (num_cashier_busy[i] != 1)
                {
                    target_cashier = i;
                    break;
                }
            }

            /* shouldn't happen but handle anyway */
            if (target_cashier == -1) {
                target_cashier = 1;
                for (int i = 2; i <= num_machines[4]; i++) {
                    if (list_size[4 + i] < list_size[4 + target_cashier])
                        target_cashier = i;
                }

                transfer[1] = sim_time;
                transfer[2] = job_type;
                transfer[3] = task;
                transfer[5] = accumulated_time;
                transfer[6] = target_cashier;
                list_file(LAST, station + target_cashier);

                return;
            }

            sampst(0.0, station + target_cashier); /* For cashier. */
        }

        sampst(0.0, station);                 /* For station. */
        sampst(0.0, num_stations + job_type); /* For job type. */

        if (num_machines[station] != -1)
        {
            ++num_machines_busy[station];
            timest((double)num_machines_busy[station], station);

            if (station == 4)
            {
                assert(target_cashier != -1);

                ++num_cashier_busy[target_cashier];
                timest((double)num_cashier_busy[target_cashier], station + target_cashier);
            }
        }

        double service_duration;
        double accumulated_duration;
        if (station == 4)
        {
            service_duration = accumulated_time;
        }
        else
        {
            service_duration = uniform(range_service[station][1], range_service[station][2], STREAM_SERVICE);
            accumulated_duration = uniform(range_accumulated_time[station][1], range_accumulated_time[station][2], STREAM_SERVICE);
            accumulated_time += accumulated_duration;
        }

        /* Schedule a service completion.  Note defining attributes beyond the
           first two for the event record before invoking event_schedule. */

        transfer[3] = job_type;
        transfer[4] = task;
        transfer[5] = accumulated_time;
        transfer[6] = target_cashier;
        event_schedule(sim_time + service_duration, EVENT_DEPARTURE);
    }
}

void depart(void) /* Event function for departure of a job from a particular
             station. */
{
    int station, job_type_queue, task_queue, target_cashier = -1;
    double current_accumulated_time, accumulated_time_queue;

    /* Determine the station from which the job is departing. */

    job_type = transfer[3];
    task = transfer[4];
    current_accumulated_time = transfer[5];
    target_cashier = transfer[6];
    station = route[job_type][task];

    /* Check to see whether the queue for this station is empty. */

    if (num_machines[station] != -1) // skip for self-serving station because it has no queue
    {
        if (station == 4)
        {
            assert(target_cashier != -1);

            if (list_size[station + target_cashier] == 0)
            {
                /* The queue for this cashier is empty, so make this cashier idle */

                --num_cashier_busy[target_cashier];
                timest((double)num_cashier_busy[target_cashier], station + target_cashier);

                --num_machines_busy[station];
                timest((double)num_machines_busy[station], station);
            }
            else
            {
                /* The queue is nonempty, so start service on first job in queue. */
                list_remove(FIRST, station + target_cashier);

                /* Tally this delay for this station. */

                sampst(sim_time - transfer[1], station);

                /* Tally this same delay for this job type. */

                job_type_queue = transfer[2];
                task_queue = transfer[3];
                sampst(sim_time - transfer[1], num_stations + job_type_queue);

                double service_duration;
                double accumulated_duration;
                accumulated_time_queue = transfer[5];

                if (station == 4)
                {
                    service_duration = accumulated_time_queue;
                }
                else
                {
                    service_duration = uniform(range_service[station][1], range_service[station][2], STREAM_SERVICE);
                    accumulated_duration = uniform(range_accumulated_time[station][1], range_accumulated_time[station][2], STREAM_SERVICE);
                    accumulated_time_queue += accumulated_duration;
                }

                /* Schedule end of service for this job at this station.  Note defining
                   attributes beyond the first two for the event record before invoking
                   event_schedule. */

                transfer[3] = job_type_queue;
                transfer[4] = task_queue;
                transfer[5] = accumulated_time_queue;
                transfer[6] = target_cashier; // tidak berubah
                event_schedule(sim_time + service_duration, EVENT_DEPARTURE);
            }
        }
        else if (list_size[station] == 0)
        {
            /* The queue for this station is empty, so make a machine in this
               station idle. */

            --num_machines_busy[station];
            timest((double)num_machines_busy[station], station);
        }
        else
        {

            /* The queue is nonempty, so start service on first job in queue. */
            list_remove(FIRST, station);

            /* Tally this delay for this station. */

            sampst(sim_time - transfer[1], station);

            /* Tally this same delay for this job type. */

            job_type_queue = transfer[2];
            task_queue = transfer[3];
            sampst(sim_time - transfer[1], num_stations + job_type_queue);

            double service_duration;
            double accumulated_duration;
            accumulated_time_queue = transfer[5];

            if (station == 4)
            {
                service_duration = accumulated_time_queue;
            }
            else
            {
                service_duration = uniform(range_service[station][1], range_service[station][2], STREAM_SERVICE);
                accumulated_duration = uniform(range_accumulated_time[station][1], range_accumulated_time[station][2], STREAM_SERVICE);
                accumulated_time_queue += accumulated_duration;
            }

            /* Schedule end of service for this job at this station.  Note defining
               attributes beyond the first two for the event record before invoking
               event_schedule. */

            transfer[3] = job_type_queue;
            transfer[4] = task_queue;
            transfer[5] = accumulated_time_queue;
            transfer[6] = target_cashier; // tidak berubah
            event_schedule(sim_time + service_duration, EVENT_DEPARTURE);
        }
    }
    else
    {
        // if drinks, do nothing
    }

    /* If the current departing job has one or more tasks yet to be done, send
       the job to the next station on its route. */

    if (task < num_tasks[job_type])
    {
        ++task;
        accumulated_time = current_accumulated_time;
        arrive(2);
    }
}

void report(void) /* Report generator function. */
{
    int i;
    double overall_avg_job_tot_delay, avg_job_tot_delay, sum_probs;

    /* Compute the average total delay in queue for each job type and the
       overall average job total delay. */

    fprintf(outfile, "\n\n\n\nJob type     Average total delay in queue");
    overall_avg_job_tot_delay = 0.0;
    sum_probs = 0.0;
    for (i = 1; i <= num_job_types; ++i)
    {
        avg_job_tot_delay = sampst(0.0, -(num_stations + i)) * num_tasks[i];
        fprintf(outfile, "\n\n%4d%27.3f", i, avg_job_tot_delay);
        overall_avg_job_tot_delay += (prob_distrib_job_type[i] - sum_probs) * avg_job_tot_delay;
        sum_probs = prob_distrib_job_type[i];
    }
    fprintf(outfile, "\n\nOverall average job total delay =%10.3f\n", overall_avg_job_tot_delay);

    /* Compute the average number in queue, the average utilization, and the
       average delay in queue for each station. */

    fprintf(outfile, "\n\n\n Work      Average delay       Maximum delay         Average number         Maximum Number");
    fprintf(outfile, "\nstation      in queue            in queue               in queue               in queue");
    for (j = 1; j <= num_stations; ++j)
    {
        if (num_machines[j] == -1)
            continue;

        sampst(0.0, -j);
        double avg_delay = transfer[1];
        double max_delay = transfer[3];

        filest(-j);
        double avg_queue = transfer[1];
        double max_queue = transfer[2];

        fprintf(outfile, "\n\n%4d%17.3f%17.3f", j, avg_delay, max_delay);

        if (j != 4)
        {
            fprintf(outfile, "%17.3f%17.3f", avg_queue, max_queue);
        }
        else // if cashier
        {
            fprintf(outfile, "%5s", ""); // small gap before each cashier

            // Print all average queues
            for (int k = 1; k <= num_machines[4]; k++)
            {
                filest(-(4 + k));
                double avg_queue_k = transfer[1];
                fprintf(outfile, "%6.3f", avg_queue_k);
                if (k < num_machines[4])
                    fprintf(outfile, " | ");
            }

            fprintf(outfile, "%5s", ""); // spacing between avg and max

            // Print all max queues
            for (int k = 1; k <= num_machines[4]; k++)
            {
                filest(-(4 + k));
                double max_queue_k = transfer[2];
                fprintf(outfile, "%6.3f", max_queue_k);
                if (k < num_machines[4])
                    fprintf(outfile, " | ");
            }
        }
    }
}

int main() /* Main function. */
{
    /* Open input and output files. */

    infile = fopen("config1.in", "r"); // Ubah config sesuai keperluan
    outfile = fopen("main.out", "w");
    if (infile == NULL || outfile == NULL)
    {
        printf("Error: could not open input/output file.\n");
        return 1;
    }

    /* Read input parameters. */

    fscanf(infile, "%d %d %lg %lg", &num_stations, &num_job_types, &mean_interarrival, &length_simulation);
    for (j = 1; j <= num_stations; ++j)
        fscanf(infile, "%d", &num_machines[j]);
    for (i = 1; i <= num_job_types; ++i)
        fscanf(infile, "%d", &num_tasks[i]);
    for (i = 1; i <= num_job_types; ++i)
        for (j = 1; j <= num_tasks[i]; ++j)
            fscanf(infile, "%d", &route[i][j]);
    for (i = 1; i <= num_stations - 1; ++i) // ! Exclude Cashier (Station 4)
        for (j = 1; j <= 2; ++j)
            fscanf(infile, "%lg", &range_service[i][j]);
    for (i = 1; i <= num_stations - 1; ++i) // ! Exclude Cashier (Station 4)
        for (j = 1; j <= 2; ++j)
            fscanf(infile, "%lg", &range_accumulated_time[i][j]);
    for (i = 1; i <= num_job_types; ++i)
        fscanf(infile, "%lg", &prob_distrib_job_type[i]);

    /* Write report heading and input parameters. */

    fprintf(outfile, "Cafeteria Job-Shop model\n\n");
    fprintf(outfile, "Number of work stations%21d\n\n", num_stations);
    fprintf(outfile, "Number of machines (workers) in each station     ");
    for (j = 1; j <= num_stations; ++j)
        fprintf(outfile, "%5d", num_machines[j]);
    fprintf(outfile, "\n\nNumber of job types%25d\n\n", num_job_types);
    fprintf(outfile, "Number of tasks for each job type      ");
    for (i = 1; i <= num_job_types; ++i)
        fprintf(outfile, "%5d", num_tasks[i]);
    fprintf(outfile, "\n\nDistribution function of job types  ");
    for (i = 1; i <= num_job_types; ++i)
        fprintf(outfile, "%8.3f", prob_distrib_job_type[i]);
    fprintf(outfile, "\n\nMean interarrival time of jobs%14.2f seconds\n\n", mean_interarrival);
    fprintf(outfile, "Length of the simulation%20.1f minutes\n\n\n", length_simulation);
    fprintf(outfile, "Job type      Work stations on route");
    for (i = 1; i <= num_job_types; ++i)
    {
        fprintf(outfile, "\n\n%4d        ", i);
        for (j = 1; j <= num_tasks[i]; ++j)
            fprintf(outfile, "%5d", route[i][j]);
    }
    fprintf(outfile, "\n\n\nJob type     ");
    fprintf(outfile, "Range service time for stations");
    for (i = 1; i <= num_stations; ++i)
    {
        fprintf(outfile, "\n\n%4d    ", i);
        for (j = 1; j <= 2; ++j)
            fprintf(outfile, "%9.2f", range_service[i][j]);
    }

    /* Initialize all machines in all stations to the idle state. */

    for (j = 1; j <= num_stations; ++j)
        num_machines_busy[j] = 0;

    for (j = 1; j <= MAX_NUM_CASHIERS; ++j) 
        num_cashier_busy[j] = 0;

    /* Initialize simlib */

    init_simlib();

    /* Set maxatr = max(maximum number of attributes per record, 4) */

    maxatr = 8; /* NEVER SET maxatr TO BE SMALLER THAN 4. */

    /* Schedule the arrival of the first job. */

    event_schedule(expon(mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL);

    /* Schedule the end of the simulation.  (This is needed for consistency of
       units.) */

    event_schedule(60 * length_simulation, EVENT_END_SIMULATION);

    /* Run the simulation until it terminates after an end-simulation event
       (type EVENT_END_SIMULATION) occurs. */

    do
    {
        /* Determine the next event. */

        timing();

        /* Invoke the appropriate event function. */

        switch (next_event_type)
        {
        case EVENT_ARRIVAL:
            arrive(1);
            break;
        case EVENT_DEPARTURE:
            depart();
            break;
        case EVENT_END_SIMULATION:
            report();
            fflush(outfile);
            break;
        }

        /* If the event just executed was not the end-simulation event (type
           EVENT_END_SIMULATION), continue simulating.  Otherwise, end the
           simulation. */

    } while (next_event_type != EVENT_END_SIMULATION);

    fclose(infile);
    fclose(outfile);

    return 0;
}

// *dari src
// gcc main.c ./SIMLIB/simlib.c -o main -lm