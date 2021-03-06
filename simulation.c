#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define SIMULATION_TIME 60      // Duração da simulação em segundos
#define JOB_RATE 12              // Taxa de surgimento de jobs em kHz
#define MAX_JOB_DURATION 1000     // Máxima duração de um job em centésimos de segundo
#define N_WORKERS 24            // Número de workers no sistema distribuído

struct Job {
	int duration;
	int deadline;
	int start;
	int end;
};

typedef struct list List;

struct list {
	struct Job job;
	List *prox;
};


struct Worker {
	int queuelen;
	struct Job* jobqueue;
	List* jobs_list; // para Round-Robin
};

List* lst_cria();

int lst_vazia(List *l);

List* lst_insere(List *l, struct Job info);

void lst_imprime_rec(List* l);

struct Job lst_last_element(List* l);

//cria um vetor circular para estratégia do round robin
//Ex.: entrada = 6 -> [ 6 7 8 ... N_WORKERS 0 1 2 3 4 5  ]
void create_vet_seq(int* vet,int start){
	int a;
	int cont = start;
	for(a=0; a < N_WORKERS; a++){
		vet[a] = cont;
		cont++;
		if(cont == N_WORKERS)
			cont=0;
	}
}

int last_work = -1;

int roundrobin(struct Worker* robin_workers,struct Job job, int t) {
	int* seq = (int*) malloc(N_WORKERS*sizeof(int));
	if(last_work == N_WORKERS -1)
		last_work = -1;
	create_vet_seq(seq, last_work + 1 );
	int i;

	for (i = 0; i < N_WORKERS; i++) {
		if ( robin_workers[ seq[i] ].jobs_list == NULL ) {
			job.start= t; job.end= t + job.duration;
			robin_workers[ seq[i] ].jobs_list = lst_insere(robin_workers[ seq[i] ].jobs_list,job);

			last_work = seq[i];
			return 0;
		}else{
			struct Job last_job = lst_last_element(robin_workers[ seq[i] ].jobs_list);

			if(last_job.end + job.duration <= job.deadline  && last_job.end + job.duration < 100*SIMULATION_TIME){
				if(last_job.end > t){
					job.start= last_job.end +1;
				}else{
					job.start= t;
				}

				job.end= job.start + job.duration;
				robin_workers[ seq[i] ].jobs_list = lst_insere(robin_workers[ seq[i] ].jobs_list,job);

				last_work = seq[i];
				return 0;
			}
		}
	}


	return 1;
}

void greedy_debug_workers(struct Worker** workers) {
	/* Imprime o conteúdo das listas de workers greedy */
    int i, j;
    for(i=0; i < N_WORKERS; i++) {
        printf("Worker %d: %d\n", i, workers[i]->queuelen);
        for(j=0; j < workers[i]->queuelen; j++) {
            printf("[%d %d] ", workers[i]->jobqueue[j].start, workers[i]->jobqueue[j].end);
        }
        printf("\n");
    }
    getchar();
}

int greedy_next_available_time(struct Job job, struct Worker* worker, int current_time, int* test_job_position) {

	/* Retorna o momento em que o worker pode realizar o job, respeitando
	   a estratégia gulosa de que o job deve ser executado o mais tarde possível,
	   mas antes de sua deadline */

	int id;
	int queuelen = worker->queuelen;
	struct Job* jobqueue = worker->jobqueue;

	if(queuelen == 0 || ( queuelen > 0 && jobqueue[queuelen-1].end <= job.deadline - job.duration)) {
		*test_job_position = queuelen;
		return job.deadline - job.duration;
	}

	for(id=queuelen-1; id > 0; id--) {
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

void greedy_add_job(struct Job* job, struct Worker** workers, int worker_id, int worker_time, int job_position) {

	/* Adiciona o job à lista de jobs do worker especificado, na posição especificada,
	   move os jobs à direita de job_position uma cada para a direita */

	int i;
	for(i=workers[worker_id]->queuelen-1; i >= job_position; i--) {
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

void greedy_remove_job(struct Worker** workers, int worker_id) {

	/* Remove o primeiro job da lista de jobs de um worker */
	int i;
	for(i=1; i < workers[worker_id]->queuelen; i++) {
		workers[worker_id]->jobqueue[i-1].start = workers[worker_id]->jobqueue[i].start;
		workers[worker_id]->jobqueue[i-1].end = workers[worker_id]->jobqueue[i].end;
		workers[worker_id]->jobqueue[i-1].duration = workers[worker_id]->jobqueue[i].duration;
		workers[worker_id]->jobqueue[i-1].deadline = workers[worker_id]->jobqueue[i].deadline;
	}
	workers[worker_id]->queuelen -= 1;
}

int greedy(struct Job job, struct Worker** workers, int current_time) {

	/* Retorna 1 se o job foi recusado, 0 se foi aceito */

	/* Inicialização de variáveis */
	int id;
	int best_worker_id = -1;
	int best_worker_time = 0;
	int* test_job_position = malloc(sizeof(int));
	*test_job_position = -1;
	int new_job_position = -1;

	/* Envia as informações do novo job para todos os workers e obtém de cada
	   um o tempo em que este pode realizar o trabalho. Seleciona o worker que
	   pode executar o job o mais tarde possível */

	for(id=0; id < N_WORKERS; id++) {
		int worker_time = greedy_next_available_time(job, workers[id], current_time, test_job_position);
		if(worker_time != -1 && best_worker_time < worker_time) {
			best_worker_time = worker_time;
			best_worker_id = id;
			new_job_position = *test_job_position;
		}
	}

	/* Se um worker tiver sido selecionado, adiciona o job à lista de jobs
	   do worker selecionado. Caso nenhum worker possa realizar a tarefa,
	   indica que o job foi recusado */
	if(best_worker_id != -1 && new_job_position != -1) {
		greedy_add_job(&job, workers, best_worker_id, best_worker_time, new_job_position);
		return 0;
	} else return 1;
}

void main() {

	// Iniciar gerador aleatório
	srand(time(NULL));

	// Inicializar contadores de jobs recusados para cada estratégia
	int roundrobin_count = 0;
	int greedy_count = 0;

	// Inicializar listas de jobs dos workers roundrobin
	/* Coisas do Isaac */
	// Inicializar listas dos workers roundrobin
	struct Worker* robin_workers = malloc(N_WORKERS*sizeof(struct Worker));

	//coisas do david
	// Inicializar listas de jobs dos workers greedy
	struct Worker** greedy_workers = malloc(N_WORKERS*sizeof(struct Worker*));

	int n;
	for(n=0; n < N_WORKERS; n++) {
		greedy_workers[n] = malloc(sizeof(struct Worker));
		greedy_workers[n]->queuelen = 0;
		greedy_workers[n]->jobqueue = malloc(JOB_RATE*100*SIMULATION_TIME*sizeof(struct Job));
	}

	/* 	Início da simulação
		Cada valor de t representa um intervalo de tempo de 0.01 segundo
		O sistema distribuído aceita processar pedidos de jobs até uma quantidade
		MAX_JOB_DURATION de segundos antes do fim da simulação */
	int t;
	for(t=0; t < 20/*00*SIMULATION_TIME - MAX_JOB_DURATION */; t++) {

		//printf("Time: %d\n", t);

		/* Gerar jobs aleatórios
		 A cada 0.01 segundo, o sistema distribuído recebe uma quantidade
		 JOB_RATE de novos jobs. Assim, o valor JOB_RATE indica a frêquencia de
		 novos jobs em kHz. Cada novo job aleatório pode ter duração entre 1 e
		 MAX_JOB_DURATION e deadline máxima para até 2 vezes depois que sua duração */
		int i, j;
		struct Job* newjobs = malloc(JOB_RATE*sizeof(struct Job));
		for(i=0; i < JOB_RATE; i++) {
			int random_duration = (rand() % MAX_JOB_DURATION) + 1;
			int random_deadline = (t + random_duration) + (rand() % (2*MAX_JOB_DURATION)) + 1;
			struct Job random_job = {random_duration, random_deadline};
			newjobs[i] = random_job;

			//printf("Random duration: %2d, Random deadline %4d\n", random_duration, random_deadline);
		}

		// Tenta submeter os jobs e atualiza a contagem de jobs recusados
		for(i=0; i < JOB_RATE; i++) {
			roundrobin_count += roundrobin(robin_workers,newjobs[i],t);
			greedy_count += greedy(newjobs[i], greedy_workers, t);
		}

		// Remove os jobs finalizados das listas de execução dos workers
		for(i=0; i < N_WORKERS; i++) {
			if(greedy_workers[i]->queuelen > 0 && greedy_workers[i]->jobqueue[0].end < t) greedy_remove_job(greedy_workers, i);
		}
	}
	int i;


	/*for(i=0; i<N_WORKERS; i++){
		printf("\n\n\n jobs do worker %d \n\n",i );
		lst_imprime_rec(robin_workers[i].jobs_list);

	}*/

	printf("Round-Robin refused jobs: %.2f%%\n", 100*(roundrobin_count/(double)((100*SIMULATION_TIME - MAX_JOB_DURATION)*JOB_RATE)));
	printf("Greedy refused jobs: %.2f%%\n", 100*(greedy_count/(double)((100*SIMULATION_TIME - MAX_JOB_DURATION)*JOB_RATE)));
}


/* Cria uma lista vazia.*/
List* lst_cria(){
	return NULL;
}

/* Testa se uma lista é vazia.*/
int lst_vazia(List *l){
	return (l==NULL);
}

/* Insere um elemento no inicio da lista.*/
List* lst_insere(List *l, struct Job info){
	List* ln = (List*)malloc(sizeof(List));
	ln->job = info;
	ln->prox = l;
	return ln;
}


struct Job lst_last_element(List* l){
	List* lAux = l;
	return lAux->job;
}


void lst_imprime_rec(List* l){
	if(lst_vazia(l)){
		return;
	}else{
			printf("job\n");
			printf("job duration: %d\n",l->job.duration);
			printf("job deadline: %d\n",l->job.deadline);
			printf("job start: %d\n",l->job.start);
			printf("job end: %d\n",l->job.end);

			lst_imprime_rec(l->prox);
	}
}
