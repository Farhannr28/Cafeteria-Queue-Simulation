#include "SIMLIB/simlib.h"

#define SEC_TO_MIN(x) ((x) / 60.0)

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
    num_cashier_busy[MAX_NUM_CASHIERS + 1], num_customers_in_system;
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

  /* Service ranges read only for stations 1..num_stations-1 (cashier excluded).
   */
  for (i = 1; i <= num_stations - 1; ++i)
    for (j = 1; j <= 2; ++j)
      fscanf(infile, "%lg", &range_service[i][j]);

  /* Accumulated-time ranges read only for stations 1..num_stations-1. */
  for (i = 1; i <= num_stations - 1; ++i)
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
  for (j = 1; j <= num_stations; ++j) {
    if (num_machines[j] < 0) {
      /* Show infinity/self-service explicitly (e.g., Drinks). */
      fprintf(outfile, "  inf");
    } else {
      fprintf(outfile, "%5d", num_machines[j]);
    }
  }

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

  /* IMPORTANT: interarrival is in seconds; length_simulation is already in
   * minutes. */
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

  fprintf(outfile, "\n\n\nStation     ");
  fprintf(outfile, "Range service time for stations");
  /* Print only for stations with actual service-time ranges
   * (1..num_stations-1). */
  for (i = 1; i <= num_stations - 1; ++i) {
    fprintf(outfile, "\n\n%4d    ", i);
    for (j = 1; j <= 2; ++j)
      fprintf(outfile, "%9.2f", range_service[i][j]);
  }
  /* For the cashier station, show N/A (service is ACT, not a station service
   * range). */
  fprintf(outfile, "\n\n%4d    %9s%9s", num_stations, "N/A", "N/A");

  /* Initialize machines/cashiers to idle and counters. */
  for (j = 1; j <= num_stations; ++j)
    num_machines_busy[j] = 0;
  for (j = 1; j <= num_machines[num_stations]; ++j)
    num_cashier_busy[j] = 0;
  num_customers_in_system = 0;

  /* Initialize simlib */
  init_simlib();

  /* transfer[1..6] used (extra for cashier id + cumulative queue delay). */
  maxatr = 6; /* NEVER SET maxatr TO BE SMALLER THAN 4. */

  /* Schedule the first arrival and the end of simulation. */
  event_schedule(expon(mean_interarrival, STREAM_INTERARRIVAL), GROUP_ARRIVAL);
  event_schedule(60 * length_simulation, END_SIMULATION);

  /* Main event loop */
  do {
    timing();
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
    /* Track customer entering the system */
    ++num_customers_in_system;
    timest((double)num_customers_in_system, num_stations + num_job_types + 2);

    int random_route = random_integer(prob_distrib_job_type, STREAM_ROUTE);

    if (random_route == 1) {
      /* Hot food route: 1 -> 3 -> 4 */
      if (num_machines_busy[1] == num_machines[1]) {
        /* Queue at station 1 */
        transfer[1] = sim_time; /* arrival to station */
        transfer[2] = 1;        /* job type */
        transfer[3] = 1;        /* current task (unused beyond route id) */
        transfer[4] = 0.0;      /* ACT so far */
        transfer[5] = 0.0;      /* cumulative queue delay so far */
        list_file(LAST, 1);
      } else {
        /* Start service immediately */
        sampst(0.0, 1); /* 0 delay for station 1 */
        ++num_machines_busy[1];
        timest((double)num_machines_busy[1], 1);

        /* For departure event */
        transfer[3] = 1;   /* job type */
        transfer[4] = 0.0; /* ACT so far */
        transfer[5] = 0.0; /* cumulative queue delay so far */

        event_schedule(sim_time + uniform(range_service[1][1],
                                          range_service[1][2],
                                          STREAM_ST_HOT_FOOD),
                       HOT_FOOD_DEPARTURE);
      }
    } else if (random_route == 2) {
      /* Sandwich route: 2 -> 3 -> 4 */
      if (num_machines_busy[2] == num_machines[2]) {
        /* Queue at station 2 */
        transfer[1] = sim_time; /* arrival to station */
        transfer[2] = 2;        /* job type */
        transfer[3] = 2;        /* current task (unused beyond route id) */
        transfer[4] = 0.0;      /* ACT so far */
        transfer[5] = 0.0;      /* cumulative queue delay so far */
        list_file(LAST, 2);
      } else {
        /* Start service immediately */
        sampst(0.0, 2); /* 0 delay for station 2 */
        ++num_machines_busy[2];
        timest((double)num_machines_busy[2], 2);

        /* For departure event */
        transfer[3] = 2;   /* job type */
        transfer[4] = 0.0; /* ACT so far */
        transfer[5] = 0.0; /* cumulative queue delay so far */

        event_schedule(sim_time + uniform(range_service[2][1],
                                          range_service[2][2],
                                          STREAM_ST_SANDWICH),
                       SANDWICH_DEPARTURE);
      }
    } else {
      /* Drinks-only route: 3 -> 4 */
      double customer_act =
          uniform(range_accumulated_time[3][1], range_accumulated_time[3][2],
                  STREAM_ACT_DRINKS);

      transfer[3] = 3;            /* job type */
      transfer[4] = customer_act; /* ACT so far */
      transfer[5] = 0.0;          /* cumulative queue delay so far */

      event_schedule(sim_time + uniform(range_service[3][1],
                                        range_service[3][2], STREAM_ST_DRINKS),
                     DRINKS_DEPARTURE);
    }
  }
}

void hot_food_departure(void) {
  int job_type = transfer[3]; /* Route 1 */
  double customer_act = transfer[4];

  /* Add accumulated time for hot food step */
  customer_act += uniform(range_accumulated_time[1][1],
                          range_accumulated_time[1][2], STREAM_ACT_HOT_FOOD);

  /* Store for drinks_departure */
  transfer[3] = job_type;
  transfer[4] = customer_act;
  /* transfer[5] already holds cumulative queue delay so far */

  /* Go to drinks (self-service timing before cashier) */
  event_schedule(sim_time + uniform(range_service[3][1], range_service[3][2],
                                    STREAM_ST_DRINKS),
                 DRINKS_DEPARTURE);

  /* Free/advance server at station 1 */
  if (list_size[1] == 0) {
    --num_machines_busy[1];
    timest((double)num_machines_busy[1], 1);
  } else {
    /* Start service for next customer */
    list_remove(FIRST, 1);
    double delay = sim_time - transfer[1];
    sampst(delay, 1); /* station 1 queue delay */

    int next_job_type = transfer[2];
    double next_customer_act = transfer[4];
    double cum_q =
        transfer[5] + delay; /* accumulate per-customer queue delay */

    /* Schedule next hot food completion */
    transfer[3] = next_job_type;
    transfer[4] = next_customer_act;
    transfer[5] = cum_q;
    event_schedule(sim_time + uniform(range_service[1][1], range_service[1][2],
                                      STREAM_ST_HOT_FOOD),
                   HOT_FOOD_DEPARTURE);
  }
}

void sandwich_departure(void) {
  int job_type = transfer[3]; /* Route 2 */
  double customer_act = transfer[4];

  /* Add accumulated time for sandwich step */
  customer_act += uniform(range_accumulated_time[2][1],
                          range_accumulated_time[2][2], STREAM_ACT_SANDWICH);

  /* Store for drinks_departure */
  transfer[3] = job_type;
  transfer[4] = customer_act;
  /* transfer[5] already holds cumulative queue delay so far */

  /* Go to drinks (self-service timing before cashier) */
  event_schedule(sim_time + uniform(range_service[3][1], range_service[3][2],
                                    STREAM_ST_DRINKS),
                 DRINKS_DEPARTURE);

  /* Free/advance server at station 2 */
  if (list_size[2] == 0) {
    --num_machines_busy[2];
    timest((double)num_machines_busy[2], 2);
  } else {
    /* Start service for next customer */
    list_remove(FIRST, 2);
    double delay = sim_time - transfer[1];
    sampst(delay, 2); /* station 2 queue delay */

    int next_job_type = transfer[2];
    double next_customer_act = transfer[4];
    double cum_q =
        transfer[5] + delay; /* accumulate per-customer queue delay */

    /* Schedule next sandwich completion */
    transfer[3] = next_job_type;
    transfer[4] = next_customer_act;
    transfer[5] = cum_q;
    event_schedule(sim_time + uniform(range_service[2][1], range_service[2][2],
                                      STREAM_ST_SANDWICH),
                   SANDWICH_DEPARTURE);
  }
}

void drinks_departure(void) {
  /* Determine target cashier: prefer any idle cashier, else shortest queue */
  int target_cashier = 0;

  /* 1) Look for idle cashier */
  for (int c = 1; c <= num_machines[4]; ++c) {
    if (num_cashier_busy[c] == 0) {
      target_cashier = c;
      break;
    }
  }

  /* 2) If none idle, choose the cashier with shortest queue */
  if (target_cashier == 0) {
    int best = 1, best_q = list_size[4]; /* list 4 = cashier 1 queue */
    for (int c = 2; c <= num_machines[4]; ++c) {
      int qsz = list_size[3 + c]; /* queues 4,5,6,... */
      if (qsz < best_q) {
        best_q = qsz;
        best = c;
      }
    }
    target_cashier = best;
  }

  int cashier_queue_list = 3 + target_cashier;

  /* Get route and ACT; transfer[5] holds cumulative queue delay so far */
  int job_type = transfer[3];
  double customer_act = transfer[4];
  double cum_q = transfer[5];

  if (num_cashier_busy[target_cashier] == 0) {
    /* Cashier idle: start immediately */
    num_cashier_busy[target_cashier] = 1;
    sampst(0.0, 4); /* 0 queue delay at cashier */

    transfer[3] = job_type;       /* keep job type */
    transfer[4] = customer_act;   /* ACT */
    transfer[5] = cum_q;          /* cumulative queue delay so far */
    transfer[6] = target_cashier; /* cashier id */
    event_schedule(sim_time + customer_act, CASHIER_DEPARTURE);

    /* Track total number in all cashier queues */
    int total_cashier_queue = 0;
    for (int i = 1; i <= num_machines[4]; i++)
      total_cashier_queue += list_size[3 + i];
    timest((double)total_cashier_queue, num_stations + num_job_types + 1);
  } else {
    /* Enqueue */
    transfer[1] = sim_time;       /* enqueue timestamp */
    transfer[2] = job_type;       /* job type */
    transfer[3] = customer_act;   /* ACT */
    transfer[4] = cum_q;          /* cumulative queue delay so far */
    transfer[5] = target_cashier; /* which cashier's queue (for clarity) */
    list_file(LAST, cashier_queue_list);

    int total_cashier_queue = 0;
    for (int i = 1; i <= num_machines[4]; i++)
      total_cashier_queue += list_size[3 + i];
    timest((double)total_cashier_queue, num_stations + num_job_types + 1);
  }
}

void cashier_departure(void) {
  /* The departing customer's info is in transfer[...] */
  int cashier_num = transfer[6];
  int cashier_queue_list = 3 + cashier_num;

  /* Sample this customer's TOTAL queue delay by job type (once, on departure)
   */
  int job_type = transfer[3];
  double cum_q =
      transfer[5]; /* cumulative queue delay including hot/sandwich (+ cashier
                      delay added below for queued ones) */
  sampst(cum_q, num_stations + job_type);

  /* Customer leaves the system */
  --num_customers_in_system;
  timest((double)num_customers_in_system, num_stations + num_job_types + 2);

  /* Start next, if any */
  if (list_size[cashier_queue_list] == 0) {
    /* No one waiting: cashier becomes idle */
    num_cashier_busy[cashier_num] = 0;

    int total_cashier_queue = 0;
    for (int i = 1; i <= num_machines[4]; i++)
      total_cashier_queue += list_size[3 + i];
    timest((double)total_cashier_queue, num_stations + num_job_types + 1);
  } else {
    /* Someone is waiting — start their service */
    list_remove(FIRST, cashier_queue_list);

    /* Extract waiting customer's info */
    double enqueue_time = transfer[1];
    int next_job_type = transfer[2];
    double next_act = transfer[3];
    double next_cum_q = transfer[4];  /* cum queue delay so far (pre-cashier) */
    int queued_cashier = transfer[5]; /* should equal cashier_num */

    /* Add this customer's cashier queue delay */
    double delay = sim_time - enqueue_time;
    sampst(delay, 4);    /* cashier station queue delay */
    next_cum_q += delay; /* now includes cashier queueing */

    /* Schedule their completion at this cashier */
    transfer[3] = next_job_type;
    transfer[4] = next_act;
    transfer[5] = next_cum_q;     /* pass cumulative delay forward */
    transfer[6] = queued_cashier; /* cashier id */
    event_schedule(sim_time + next_act, CASHIER_DEPARTURE);

    /* Track total number in all cashier queues */
    int total_cashier_queue = 0;
    for (int i = 1; i <= num_machines[4]; i++)
      total_cashier_queue += list_size[3 + i];
    timest((double)total_cashier_queue, num_stations + num_job_types + 1);
  }
}

void report(void) {
  int i;
  double overall_avg_job_tot_delay, avg_job_tot_delay, sum_probs;

  fprintf(outfile, "\n\n\n========================================\n");
  fprintf(outfile, "SYSTEM PERFORMANCE MEASURES\n");
  fprintf(outfile, "========================================\n");

  /* 1. Average and maximum delays in queue for each (queueing) station */
  fprintf(outfile, "\n\n1. DELAYS IN QUEUE BY STATION\n");
  fprintf(outfile, "---------------------------------------------\n");
  fprintf(outfile, "Station                    Average    Maximum\n");
  fprintf(outfile, "                           (minutes)  (minutes)\n");
  fprintf(outfile, "---------------------------------------------\n");

  /* Hot food (station 1) */
  sampst(0.0, -1);
  fprintf(outfile, "Hot Food                   %8.2f   %8.2f\n",
          SEC_TO_MIN(transfer[1]), SEC_TO_MIN(transfer[3]));

  /* Specialty sandwiches (station 2) */
  sampst(0.0, -2);
  fprintf(outfile, "Specialty Sandwiches       %8.2f   %8.2f\n",
          SEC_TO_MIN(transfer[1]), SEC_TO_MIN(transfer[3]));

  /* Cashiers (station 4) */
  sampst(0.0, -4);
  fprintf(outfile, "Cashiers (all)             %8.2f   %8.2f\n",
          SEC_TO_MIN(transfer[1]), SEC_TO_MIN(transfer[3]));

  /* 2. Time-average and maximum number in queue (exclude Drinks — infinite
   * server) */
  fprintf(outfile, "\n\n2. NUMBER IN QUEUE\n");
  fprintf(outfile, "---------------------------------------------\n");
  fprintf(outfile, "Station                    Time-Avg   Maximum\n");
  fprintf(outfile, "---------------------------------------------\n");

  /* Hot food queue */
  filest(1);
  fprintf(outfile, "Hot Food                   %8.2f   %8.0f\n", transfer[1],
          transfer[2]); /* counts, not time */

  /* Specialty sandwiches queue */
  filest(2);
  fprintf(outfile, "Specialty Sandwiches       %8.2f   %8.0f\n", transfer[1],
          transfer[2]); /* counts, not time */

  /* Total cashier queue across all cashiers (tracked via timest) */
  timest(0.0, -(num_stations + num_job_types + 1));
  fprintf(outfile, "All Cashiers (total)       %8.2f   %8.0f\n", transfer[1],
          transfer[2]); /* counts, not time */

  /* Individual cashier queues (optional detail) */
  fprintf(outfile, "\n   Individual Cashier Queues:\n");
  for (int k = 1; k <= num_machines[4]; k++) {
    filest(3 + k);
    double avg_queue_k = transfer[1];
    double max_queue_k = transfer[2];
    if (avg_queue_k < 0)
      avg_queue_k = 0.0;
    if (max_queue_k < 0)
      max_queue_k = 0.0;
    fprintf(outfile, "   Cashier %d                 %8.2f   %8.0f\n", k,
            avg_queue_k, max_queue_k); /* counts */
  }

  /* 3. Average and maximum TOTAL queue delay per customer, by type
        (sampled exactly once per customer upon leaving cashier) */
  fprintf(outfile, "\n\n3. TOTAL DELAY BY CUSTOMER TYPE\n");
  fprintf(outfile, "---------------------------------------------\n");
  fprintf(outfile, "Customer Type              Average    Maximum\n");
  fprintf(outfile, "                           (minutes)  (minutes)\n");
  fprintf(outfile, "---------------------------------------------\n");

  const char *customer_names[] = {"", "Hot Food Route", "Sandwich Route",
                                  "Drinks-Only Route"};

  for (i = 1; i <= num_job_types; ++i) {
    sampst(0.0, -(num_stations +
                  i)); /* read totals per customer type (in seconds) */
    double avg_sec = transfer[1];
    double max_sec = transfer[3];
    fprintf(outfile, "%-26s %8.2f   %8.2f\n", customer_names[i],
            SEC_TO_MIN(avg_sec), SEC_TO_MIN(max_sec));
  }

  /* 4. OVERALL average total delay (probabilities are CDF-style) */
  fprintf(outfile, "\n\n4. OVERALL AVERAGE TOTAL DELAY\n");
  fprintf(outfile, "---------------------------------------------\n");

  overall_avg_job_tot_delay = 0.0; /* accumulate in seconds */
  sum_probs = 0.0;
  for (i = 1; i <= num_job_types; ++i) {
    sampst(0.0, -(num_stations + i));
    double avg_sec = transfer[1];
    overall_avg_job_tot_delay +=
        (prob_distrib_job_type[i] - sum_probs) * avg_sec;
    sum_probs = prob_distrib_job_type[i];
  }
  fprintf(outfile, "Overall Average Total Delay:  %8.2f minutes\n",
          SEC_TO_MIN(overall_avg_job_tot_delay));

  /* 5. Total number of customers in entire system (counts) */
  fprintf(outfile, "\n\n5. TOTAL CUSTOMERS IN SYSTEM\n");
  fprintf(outfile, "---------------------------------------------\n");
  timest(0.0, -(num_stations + num_job_types + 2));
  fprintf(outfile, "Time-Average Number:          %8.2f\n",
          transfer[1]); /* count */
  fprintf(outfile, "Maximum Number:               %8.0f\n",
          transfer[2]); /* count */

  fprintf(outfile, "\n========================================\n");
  fprintf(outfile, "END OF REPORT\n");
  fprintf(outfile, "========================================\n");
}
