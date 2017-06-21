#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIMULATION_TIME 100
#define JOB_RATE 2
#define MAX_JOB_DURATION 60
#define N_WORKERS 4

struct Job {
	int duration;
	int deadline;
};

int roundrobin(struct Job job) {
	return 0;
}

int greedy(struct Job job) {
	return 0;
}

void main() {

	srand(time(NULL));

	int roundrobin_count = 0;
	int greedy_count = 0;

	// Inicializar listas de jobs dos workers roundrobin
	/* Coisas do Isaac */

	// Inicializar listas de jobs dos workers greedy
	struct Job** greedy_workers = malloc(N_WORKERS*sizeof(struct Job*));

	int n;
	for(n=0; n < N_WORKERS; n++) {
		greedy_workers[n] = malloc(JOB_RATE*60*SIMULATION_TIME*sizeof(struct Job));
	}

	// Início da simulação
	int t;
	for(t=0; t < 60*SIMULATION_TIME - MAX_JOB_DURATION; t++) {

		printf("Time: %d\n", t);

		// Gerar jobs aleatórios
		int i;
		struct Job* newjobs = malloc(JOB_RATE*sizeof(struct Job));
		for(i=0; i < JOB_RATE; i++) {
			int random_duration = (rand() % MAX_JOB_DURATION) + 1;
			int random_deadline = (t + random_duration) + (rand() % (60*SIMULATION_TIME - MAX_JOB_DURATION - t)) + 1;
			struct Job random_job = {random_duration, random_deadline};
			newjobs[i] = random_job;

			printf("Random duration: %d, Random deadline %d\n", random_duration, random_deadline);
		}

		// Tenta adicionar os jobs
		for(i=0; i < JOB_RATE; i++) {
			//roundrobin_count += roundrobin(newjobs[i]);
			//greedy_count += greedy(newjobs[i]);
		}
	}

	printf("Round-Robin refused jobs: %d\n", roundrobin_count);
	printf("Greedy refused jobs: %d\n", greedy_count);
}
