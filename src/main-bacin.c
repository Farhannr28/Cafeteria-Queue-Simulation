#include "SIMLIB/simlib.h"

#define GROUP_ARRIVAL 1
#define HOT_FOOD_DEPARTURE 2
#define SANDWICH_DEPARTURE 3
#define DRINKS_DEPARTURE 4
#define CASHIER_DEPARTURE 5
#define END_SIMULATION 6

#define STREAM_INTERARRIVAL 1
#define STREAM_GROUP_SIZE 2
#define STREAM_ROUTE 3
#define STREAM_ST_HOT_FOOD 4
#define STREAM_ST_SANDWICH 5
#define STREAM_ST_DRINKS 6
#define STREAM_ACT_HOT_FOOD 7
#define STREAM_ACT_SANDWICH 8
#define STREAM_ACT_DRINKS 9

#define MAX_NUM_STATIONS 4
#define MAX_NUM_GROUP_SIZES 4
#define MAX_NUM_JOB_TYPES 3
#define MAX_NUM_CASHIERS 3

int num_stations, num_group_sizes, num_job_types, i, j,
    num_machines[MAX_NUM_STATIONS + 1], num_tasks[MAX_NUM_JOB_TYPES + 1],
    route[MAX_NUM_JOB_TYPES + 1][MAX_NUM_STATIONS + 1],
    num_machines_busy[MAX_NUM_STATIONS + 1], job_type, task,
    num_cashier_busy[MAX_NUM_CASHIERS + 1];
double mean_interarrival, length_simulation,
    prob_distrib_group_size[MAX_NUM_GROUP_SIZES + 1],
    prob_distrib_job_type[MAX_NUM_JOB_TYPES + 1],
    range_service[MAX_NUM_STATIONS + 1][2 + 1],
    range_accumulated_time[MAX_NUM_STATIONS + 1][2 + 1], accumulated_time;
FILE *infile, *outfile;

void group_arrival(void);
void hot_food_departure(void);
void sandwich_departure(void);
void drinks_departure(void);
void cashier_departure(void);
void report(void);

int main(int argc, char *argv[]) {
  /* Check command-line arguments. */

  if (argc < 3) {
    printf("Usage: %s <input_file> <output_file>\n", argv[0]);
    printf("Example: %s config/1-base.in main.out\n", argv[0]);
    return 1;
  }

  /* Open input and output files. */

  infile = fopen(argv[1], "r");
  outfile = fopen(argv[2], "w");
  if (infile == NULL || outfile == NULL) {
    printf("Error: could not open input/output file.\n");
    return 1;
  }

  /* Read input parameters. */

  fscanf(infile, "%d %d %d %lg %lg", &num_stations, &num_group_sizes,
         &num_job_types, &mean_interarrival, &length_simulation);
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
  for (i = 1; i <= num_group_sizes; ++i)
    fscanf(infile, "%lg", &prob_distrib_group_size[i]);
  for (i = 1; i <= num_job_types; ++i)
    fscanf(infile, "%lg", &prob_distrib_job_type[i]);

  /* Write report heading and input parameters. */

  fprintf(outfile, "Cafeteria Job-Shop model\n\n");
  fprintf(outfile, "Number of work stations%21d\n\n", num_stations);
  fprintf(outfile, "Number of machines (workers) in each station     ");
  for (j = 1; j <= num_stations; ++j)
    fprintf(outfile, "%5d", num_machines[j]);
  fprintf(outfile, "\n\nNumber of group sizes%25d\n\n", num_group_sizes);
  fprintf(outfile, "Distribution function of group sizes  ");
  for (i = 1; i <= num_group_sizes; ++i)
    fprintf(outfile, "%8.3f", prob_distrib_group_size[i]);
  fprintf(outfile, "\n\nNumber of job types%25d\n\n", num_job_types);
  fprintf(outfile, "Number of tasks for each job type      ");
  for (i = 1; i <= num_job_types; ++i)
    fprintf(outfile, "%5d", num_tasks[i]);
  fprintf(outfile, "\n\nDistribution function of job types  ");
  for (i = 1; i <= num_job_types; ++i)
    fprintf(outfile, "%8.3f", prob_distrib_job_type[i]);
  fprintf(outfile, "\n\nMean interarrival time of jobs%14.2f seconds\n\n",
          mean_interarrival);
  fprintf(outfile, "Length of the simulation%20.1f minutes\n\n\n",
          length_simulation);
  fprintf(outfile, "Job type      Work stations on route");
  for (i = 1; i <= num_job_types; ++i) {
    fprintf(outfile, "\n\n%4d        ", i);
    for (j = 1; j <= num_tasks[i]; ++j)
      fprintf(outfile, "%5d", route[i][j]);
  }
  fprintf(outfile, "\n\n\nJob type     ");
  fprintf(outfile, "Range service time for stations");
  for (i = 1; i <= num_stations; ++i) {
    fprintf(outfile, "\n\n%4d    ", i);
    for (j = 1; j <= 2; ++j)
      fprintf(outfile, "%9.2f", range_service[i][j]);
  }

  /* Initialize all machines in all stations to the idle state. */

  for (j = 1; j <= num_stations; ++j)
    num_machines_busy[j] = 0;

  for (j = 1; j <= num_machines[num_stations]; ++j)
    num_cashier_busy[j] = 0;

  /* Initialize simlib */

  init_simlib();

  /* Set maxatr = max(maximum number of attributes per record, 4) */

  maxatr = 5; /* NEVER SET maxatr TO BE SMALLER THAN 4. */

  /* Schedule the arrival of the first group. */

  event_schedule(expon(mean_interarrival, STREAM_INTERARRIVAL), GROUP_ARRIVAL);

  /* Schedule the end of the simulation.  (This is needed for consistency of
     units.) */

  event_schedule(60 * length_simulation, END_SIMULATION);

  /* Run the simulation until it terminates after an end-simulation event
     (type END_SIMULATION) occurs. */

  do {
    /* Determine the next event. */

    timing();

    /* Invoke the appropriate event function. */

    switch (next_event_type) {
    case GROUP_ARRIVAL:
      group_arrival();
      break;
    case HOT_FOOD_DEPARTURE:
      hot_food_departure();
      break;
    case SANDWICH_DEPARTURE:
      sandwich_departure();
      break;
    case DRINKS_DEPARTURE:
      drinks_departure();
      break;
    case CASHIER_DEPARTURE:
      cashier_departure();
      break;
    case END_SIMULATION:
      report();
      fflush(outfile);
      break;
    }

    /* If the event just executed was not the end-simulation event (type
       END_SIMULATION), continue simulating.  Otherwise, end the
       simulation. */

  } while (next_event_type != END_SIMULATION);

  fclose(infile);
  fclose(outfile);

  return 0;
}

void group_arrival(void) {
  event_schedule(sim_time + expon(mean_interarrival, STREAM_INTERARRIVAL),
                 GROUP_ARRIVAL);

  int group_size = random_integer(prob_distrib_group_size, STREAM_GROUP_SIZE);
  for (int i = 0; i < group_size; i++) {
    int random_route = random_integer(prob_distrib_job_type, STREAM_ROUTE);

    // Hot food route (1): 1 -> 3 -> 4
    // Sandwich route (2): 2 -> 3 -> 4
    // Drinks route (3): 3 -> 4
    if (random_route == 1) {
      if (num_machines_busy[1] == num_machines[1]) {
        // 1. Time of arrival to this station.
        // 2. Job type.
        // 3. Current task number.
        // 4. Accumulated time (ACT) for this customer
        transfer[1] = sim_time;
        transfer[2] = 1;
        transfer[3] = 1;
        transfer[4] = 0.0; // Initialize ACT = 0
        list_file(LAST, 1);
      } else {
        sampst(0.0, 1);                // Record 0 delay for station
        sampst(0.0, num_stations + 1); // Record 0 delay for job type 1
        ++num_machines_busy[1];
        timest((double)num_machines_busy[1], 1);

        // Store customer info in transfer for departure event
        transfer[3] = 1;   // job type
        transfer[4] = 0.0; // Initialize ACT = 0

        // Schedule departure with U(50,120) using stream 4
        event_schedule(sim_time + uniform(range_service[1][1],
                                          range_service[1][2],
                                          STREAM_ST_HOT_FOOD),
                       HOT_FOOD_DEPARTURE);
      }
    } else if (random_route == 2) {
      if (num_machines_busy[2] == num_machines[2]) {
        // 1. Time of arrival to this station.
        // 2. Job type.
        // 3. Current task number.
        // 4. Accumulated time (ACT) for this customer
        transfer[1] = sim_time;
        transfer[2] = 2;
        transfer[3] = 2;
        transfer[4] = 0.0; // Initialize ACT = 0
        list_file(LAST, 2);
      } else {
        sampst(0.0, 2);                // Record 0 delay for station
        sampst(0.0, num_stations + 2); // Record 0 delay for job type 2
        ++num_machines_busy[2];
        timest((double)num_machines_busy[2], 2);

        // Store customer info in transfer for departure event
        transfer[3] = 2;   // job type
        transfer[4] = 0.0; // Initialize ACT = 0

        // Schedule departure with U(60, 180) using stream 5
        event_schedule(sim_time + uniform(range_service[2][1],
                                          range_service[2][2],
                                          STREAM_ST_SANDWICH),
                       SANDWICH_DEPARTURE);
      }
    } else {
      // Drinks-only route (route 3)
      // Initialize ACT = U(5,10) using stream 9
      double customer_act =
          uniform(range_accumulated_time[3][1], range_accumulated_time[3][2],
                  STREAM_ACT_DRINKS);

      // Store customer's route and ACT in transfer for drinks_departure
      transfer[3] = 3; // Route 3 (drinks-only)
      transfer[4] = customer_act;

      // Schedule DRINKS_DEPARTURE at now + U(5,20) using stream 6
      event_schedule(sim_time + uniform(range_service[3][1],
                                        range_service[3][2], STREAM_ST_DRINKS),
                     DRINKS_DEPARTURE);
    }
  }
}

void hot_food_departure(void) {
  // Get customer info from event transfer
  int job_type = transfer[3]; // Route 1
  double customer_act = transfer[4];

  // Add accumulated time U(20,40) using stream 7
  customer_act += uniform(range_accumulated_time[1][1],
                          range_accumulated_time[1][2], STREAM_ACT_HOT_FOOD);

  // Store job_type and ACT for drinks_departure
  transfer[3] = job_type;
  transfer[4] = customer_act;

  // Schedule DRINKS_DEPARTURE at now + U(5,20) using stream 6
  event_schedule(sim_time + uniform(range_service[3][1], range_service[3][2],
                                    STREAM_ST_DRINKS),
                 DRINKS_DEPARTURE);

  // Check if hot food queue is nonempty
  if (list_size[1] == 0) {
    // Queue is empty, free the worker
    --num_machines_busy[1];
    timest((double)num_machines_busy[1], 1);
  } else {
    // Queue is nonempty, start service for next customer
    list_remove(FIRST, 1); // Pop head from hot food queue

    // Calculate and record delay
    double delay = sim_time - transfer[1];
    sampst(delay, 1); // Record delay for hot food station

    // Get customer info from transfer (after list_remove)
    int next_job_type = transfer[2];
    int next_task = transfer[3];
    double next_customer_act = transfer[4]; // Customer's ACT

    // Also record delay for this job type (route)
    sampst(delay, num_stations + next_job_type);

    // Schedule next HOT_FOOD_DEPARTURE with U(50,120) using stream 4
    transfer[3] = next_job_type;
    transfer[4] = next_customer_act; // Pass ACT to departure event
    event_schedule(sim_time + uniform(range_service[1][1], range_service[1][2],
                                      STREAM_ST_HOT_FOOD),
                   HOT_FOOD_DEPARTURE);
    // Worker stays busy
  }
}

void sandwich_departure(void) {
  // Get customer info from event transfer
  int job_type = transfer[3]; // Route 2
  double customer_act = transfer[4];

  // Add accumulated time U(5,15) using stream 8
  customer_act += uniform(range_accumulated_time[2][1],
                          range_accumulated_time[2][2], STREAM_ACT_SANDWICH);

  // Store job_type and ACT for drinks_departure
  transfer[3] = job_type;
  transfer[4] = customer_act;

  // Schedule DRINKS_DEPARTURE at now + U(5,20) using stream 6
  event_schedule(sim_time + uniform(range_service[3][1], range_service[3][2],
                                    STREAM_ST_DRINKS),
                 DRINKS_DEPARTURE);

  // Check if sandwich queue is nonempty
  if (list_size[2] == 0) {
    // Queue is empty, free the worker
    --num_machines_busy[2];
    timest((double)num_machines_busy[2], 2);
  } else {
    // Queue is nonempty, start service for next customer
    list_remove(FIRST, 2); // Pop head from sandwich queue

    // Calculate and record delay
    double delay = sim_time - transfer[1];
    sampst(delay, 2); // Record delay for sandwich station

    // Get customer info from transfer (after list_remove)
    int next_job_type = transfer[2];
    int next_task = transfer[3];
    double next_customer_act = transfer[4]; // Customer's ACT

    // Also record delay for this job type (route)
    sampst(delay, num_stations + next_job_type);

    // Schedule next SANDWICH_DEPARTURE with U(60, 180) using stream 5
    transfer[3] = next_job_type;
    transfer[4] = next_customer_act; // Pass ACT to departure event
    event_schedule(sim_time + uniform(range_service[2][1], range_service[2][2],
                                      STREAM_ST_SANDWICH),
                   SANDWICH_DEPARTURE);
    // Worker stays busy
  }
}

void drinks_departure(void) {
  // Cashier arrival - choose shortest cashier queue
  int shortest_queue = 1; // Start with cashier 1
  int min_queue_size =
      list_size[4]; // Queue for cashier 1 (list 4 = station 4, cashier 1)

  // Find the shortest cashier queue
  for (int i = 2; i <= num_machines[4]; i++) {
    int queue_list =
        3 + i; // Cashier queues are on lists 4, 5, 6 for cashiers 1, 2, 3
    if (list_size[queue_list] < min_queue_size) {
      min_queue_size = list_size[queue_list];
      shortest_queue = i;
    }
  }

  int cashier_queue_list =
      3 + shortest_queue; // List number for this cashier's queue

  // Get customer's route and ACT from event transfer
  int job_type = transfer[3];
  double customer_act = transfer[4];

  // Check if chosen cashier is idle
  if (num_cashier_busy[shortest_queue] == 0) {
    // Cashier is idle, start service immediately
    num_cashier_busy[shortest_queue] = 1;

    // Record 0 delay for station and job type
    sampst(0.0, 4);                       // Cashier station
    sampst(0.0, num_stations + job_type); // Job type

    // Schedule CASHIER_DEPARTURE at now + ACT (accumulated time)
    transfer[3] = job_type;       // Store job type
    transfer[4] = customer_act;   // Pass ACT to cashier_departure
    transfer[5] = shortest_queue; // Store which cashier
    event_schedule(sim_time + customer_act, CASHIER_DEPARTURE);
  } else {
    // Cashier is busy, enqueue with timestamp
    transfer[1] = sim_time;       // Timestamp
    transfer[2] = job_type;       // Job type (route)
    transfer[3] = customer_act;   // Store ACT for this customer
    transfer[4] = shortest_queue; // Which cashier queue
    list_file(LAST, cashier_queue_list);
  }
}

void cashier_departure(void) {
  // Get which cashier this departure is from
  int cashier_num = transfer[5];
  int cashier_queue_list =
      3 + cashier_num; // List 4, 5, or 6 for cashiers 1, 2, 3

  // Customer departs the system (no further processing needed)

  // Check if this cashier's queue is nonempty
  if (list_size[cashier_queue_list] == 0) {
    // Queue is empty, free the cashier
    num_cashier_busy[cashier_num] = 0;
  } else {
    // Queue is nonempty, start service for next customer
    list_remove(FIRST, cashier_queue_list); // Pop head

    // After list_remove, transfer contains the customer's data:
    // transfer[1] = timestamp when they joined queue
    // transfer[2] = job type (route)
    // transfer[3] = ACT for this customer
    // transfer[4] = cashier number

    // Calculate and record delay
    double delay = sim_time - transfer[1];
    sampst(delay, 4); // Record delay for cashier station (station 4)

    // Get customer info from transfer
    int job_type = transfer[2];
    double customer_act = transfer[3];
    int queued_cashier = transfer[4];

    // Also record delay for this job type
    sampst(delay, num_stations + job_type);

    // Store info for next departure
    transfer[3] = job_type;
    transfer[4] = customer_act;
    transfer[5] = queued_cashier;

    // Schedule CASHIER_DEPARTURE at now + ACT
    event_schedule(sim_time + customer_act, CASHIER_DEPARTURE);
    // Cashier stays busy
  }
}

void report(void) {
  int i;
  double overall_avg_job_tot_delay, avg_job_tot_delay, sum_probs;

  /* Compute the average total delay in queue for each job type and the
     overall average job total delay. */

  fprintf(outfile, "\n\n\n\nJob type     Average total delay in queue");
  overall_avg_job_tot_delay = 0.0;
  sum_probs = 0.0;
  for (i = 1; i <= num_job_types; ++i) {
    avg_job_tot_delay = sampst(0.0, -(num_stations + i)) * num_tasks[i];
    fprintf(outfile, "\n\n%4d%27.3f", i, avg_job_tot_delay);
    overall_avg_job_tot_delay +=
        (prob_distrib_job_type[i] - sum_probs) * avg_job_tot_delay;
    sum_probs = prob_distrib_job_type[i];
  }
  fprintf(outfile, "\n\nOverall average job total delay =%10.3f\n",
          overall_avg_job_tot_delay);

  /* Compute the average number in queue, the average utilization, and the
     average delay in queue for each station. */

  fprintf(outfile, "\n\n\n Work      Average delay       Maximum delay         "
                   "Average number         Maximum Number");
  fprintf(outfile, "\nstation      in queue            in queue               "
                   "in queue               in queue");
  for (j = 1; j <= num_stations; ++j) {
    if (num_machines[j] == -1)
      continue;

    sampst(0.0, -j);
    double avg_delay = transfer[1];
    double max_delay = transfer[3];

    fprintf(outfile, "\n\n%4d%17.3f%17.3f", j, avg_delay, max_delay);

    if (j != 4) {
      filest(j);
      double avg_queue = transfer[1];
      double max_queue = transfer[2];
      fprintf(outfile, "%17.3f%17.3f", avg_queue, max_queue);
    } else // if cashier
    {
      fprintf(outfile, "%5s", ""); // small gap before each cashier

      // Print all average queues
      for (int k = 1; k <= num_machines[4]; k++) {
        filest((3 + k)); // Lists 4, 5, 6 for cashiers 1, 2, 3
        double avg_queue_k = transfer[1];
        // Handle unused lists (SIMLIB returns garbage for lists never used)
        if (avg_queue_k < 0)
          avg_queue_k = 0.0;
        fprintf(outfile, "%6.3f", avg_queue_k);
        if (k < num_machines[4])
          fprintf(outfile, " | ");
      }

      fprintf(outfile, "%5s", ""); // spacing between avg and max

      // Print all max queues
      for (int k = 1; k <= num_machines[4]; k++) {
        filest((3 + k)); // Lists 4, 5, 6 for cashiers 1, 2, 3
        double max_queue_k = transfer[2];
        // Handle unused lists (SIMLIB returns garbage for lists never used)
        if (max_queue_k < 0)
          max_queue_k = 0.0;
        fprintf(outfile, "%6.3f", max_queue_k);
        if (k < num_machines[4])
          fprintf(outfile, " | ");
      }
    }
  }
}