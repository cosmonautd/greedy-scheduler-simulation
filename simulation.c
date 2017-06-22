#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define SIMULATION_TIME 2
#define JOB_RATE 2
#define MAX_JOB_DURATION 60
#define N_WORKERS 4

struct Job {
	int duration;
	int deadline;
	int start;
	int end;
};

struct Worker {
	int queuelen;
	struct Job* jobqueue;
};

int roundrobin(struct Job job) {
	return 0;
}

int get_next_available_time(struct Job job, struct Worker* worker, int current_time, int* test_job_position) {

	int id;
	int queuelen = worker->queuelen;
	struct Job* jobqueue = worker->jobqueue;

	if(queuelen == 0 || ( queuelen > 0 && jobqueue[queuelen-1].end <= job.deadline - job.duration)) {
		*test_job_position = queuelen;
		return job.deadline - job.duration;
	}

	for(id=queuelen-1; id > 0 && id < queuelen; id--) {
		int deadline_restriction = MIN(job.deadline, jobqueue[id].start);
		if(jobqueue[id-1].end + job.duration <= deadline_restriction) {
			*test_job_position = id;
			return deadline_restriction - job.duration;
		}
	}

	int deadline_restriction = MIN(job.deadline, jobqueue[0].start);
	if(current_time <= deadline_restriction - job.duration) {
		*test_job_position = 0;
		return jobqueue[0].start - job.duration;
	}

	return -1;
}

void add_job(struct Job* job, struct Worker** workers, int worker_id, int worker_time, int job_position) {

	int i;
	for(i=job_position; i < workers[worker_id]->queuelen; i++) {
		workers[worker_id]->jobqueue[i+1].start = workers[worker_id]->jobqueue[i].start;
		workers[worker_id]->jobqueue[i+1].end = workers[worker_id]->jobqueue[i].end;
		workers[worker_id]->jobqueue[i+1].duration = workers[worker_id]->jobqueue[i].duration;
		workers[worker_id]->jobqueue[i+1].deadline = workers[worker_id]->jobqueue[i].deadline;
	}

	workers[worker_id]->jobqueue[job_position].start = worker_time;
	workers[worker_id]->jobqueue[job_position].end = worker_time + job->duration;
	workers[worker_id]->jobqueue[job_position].duration = job->duration;
	workers[worker_id]->jobqueue[job_position].deadline = job->deadline;
	workers[worker_id]->queuelen += 1;
}

void remove_job(struct Worker** workers, int worker_id) {
	int i;
	for(i=1; i < workers[worker_id]->queuelen; i++) {
		workers[worker_id]->jobqueue[i-1].start = workers[worker_id]->jobqueue[i].start;
		workers[worker_id]->jobqueue[i-1].end = workers[worker_id]->jobqueue[i].end;
		workers[worker_id]->jobqueue[i-1].duration = workers[worker_id]->jobqueue[i].duration;
		workers[worker_id]->jobqueue[i-1].deadline = workers[worker_id]->jobqueue[i].deadline;
	}
	workers[worker_id]->queuelen += 1;
}

int greedy(struct Job job, struct Worker** workers, int current_time) {

	int id;
	int best_worker_id = -1;
	int best_worker_time = 0;
	int* test_job_position = malloc(sizeof(int));
	*test_job_position = -1;
	int new_job_position = -1;

	for(id=0; id < N_WORKERS; id++) {
		int worker_time = get_next_available_time(job, workers[id], current_time, test_job_position);
		if(worker_time != -1 && best_worker_time < worker_time) {
			best_worker_time = worker_time;
			best_worker_id = id;
			new_job_position = *test_job_position;
		}
	}

	if(best_worker_id != -1 && (new_job_position) != -1) {
		add_job(&job, workers, best_worker_id, best_worker_time, new_job_position);
		return 0;
	} else return 1;
}

void main() {

	srand(time(NULL));

	int roundrobin_count = 0;
	int greedy_count = 0;

	// Inicializar listas de jobs dos workers roundrobin
	/* Coisas do Isaac */

	// Inicializar listas de jobs dos workers greedy
	struct Worker** greedy_workers = malloc(N_WORKERS*sizeof(struct Worker*));

	int n;
	for(n=0; n < N_WORKERS; n++) {
		greedy_workers[n] = malloc(sizeof(struct Worker));
		greedy_workers[n]->queuelen = 0;
		greedy_workers[n]->jobqueue = malloc(JOB_RATE*60*SIMULATION_TIME*sizeof(struct Job));
	}

	// Início da simulação
	int t;
	for(t=0; t < 60*SIMULATION_TIME - MAX_JOB_DURATION; t++) {

		printf("Time: %d\n", t);

		// Gerar jobs aleatórios
		int i, j;
		struct Job* newjobs = malloc(JOB_RATE*sizeof(struct Job));
		for(i=0; i < JOB_RATE; i++) {
			int random_duration = (rand() % MAX_JOB_DURATION) + 1;
			int random_deadline = (t + random_duration) + (rand() % (60*SIMULATION_TIME - MAX_JOB_DURATION - t)) + 1;
			struct Job random_job = {random_duration, random_deadline};
			newjobs[i] = random_job;

			printf("Random duration: %2d, Random deadline %4d\n", random_duration, random_deadline);
		}

		// Tenta adicionar os jobs e atualiza a contagem de jobs recusados
		for(i=0; i < JOB_RATE; i++) {
			roundrobin_count += roundrobin(newjobs[i]);
			greedy_count += greedy(newjobs[i], greedy_workers, t);
		}

		for(i=0; i < N_WORKERS; i++) {
			printf("Worker %d: %d\n", i, greedy_workers[i]->queuelen);
			for(j=0; j < greedy_workers[i]->queuelen; j++) {
				printf("[%d %d] ", greedy_workers[i]->jobqueue[j].start, greedy_workers[i]->jobqueue[j].end);
			}
			printf("\n");

			if(greedy_workers[i]->queuelen > 0 && greedy_workers[i]->jobqueue[0].end < t + 1) remove_job(greedy_workers, i);
		}

		getchar();
	}

	printf("\n");
	printf("Round-Robin refused jobs: %.2f%%\n", 100*(roundrobin_count/(double)((60*SIMULATION_TIME - MAX_JOB_DURATION)*JOB_RATE)));
	printf("Greedy refused jobs: %.2f%%\n", 100*(greedy_count/(double)((60*SIMULATION_TIME - MAX_JOB_DURATION)*JOB_RATE)));
}
