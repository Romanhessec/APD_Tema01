#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h> 
#include "list.h"
#include <math.h>

pthread_mutex_t mutex;
pthread_barrier_t barrier;
bool done = false;
int mappers_done = 0;

struct args_reducer {
    int M, R, id;
    bool mutexed;
    struct node*** mapper_results;
    struct node** aggregate_list;
    pthread_t* tid;
};

struct args_mapper {
    int M, R, id, N;
    char** input_files;
    struct node** mapper_results;
};

struct node {
    int val;
    struct node* next;
};

int min(int a, int b) {
    return a >= b ? b : a;
}

void get_args(int argc, char **argv, int* M, int* R, FILE** in) {
    if(argc < 4) {
		printf("Error: Inssuficient parameters.\n");
		exit(1);
	}
    *M = atoi(argv[1]);
    *R = atoi(argv[2]);
    *in = fopen(argv[3], "r");
    if (in == NULL) {
        printf("Error: File given doesn't exist.\n");
        exit(1);
    }
}

void parse_in(FILE* in, char*** input_files, int* N) {
    char* line = malloc(sizeof(char) * 100);
    fgets(line, sizeof(line), in);
    *N = atoi(line);
    
    *input_files = malloc(sizeof(char*) * (*N));
    
    for (int i = 0; i < (*N); i++) {
        (*input_files)[i] = malloc(sizeof(char*));
    }

    for (int i = 0; i < (*N); i++) {
        fgets(line, sizeof(char) * 100, in);
        line[strcspn(line, "\n")] = 0; //strip the '\n' character
        strcpy((*input_files)[i],line);
    }
}

void close_all_files(FILE*** input_files, int* N) {
    for (int i = 0; i < (*N); i++) {
        fclose((*input_files)[i]);
    }
}

void build_args_reducer_struct(struct args_reducer* args_reducer, int M, int R, int id, pthread_t* tid,
                                struct node*** mapper_result, struct node** aggregate_list) {
    args_reducer->M = M;
    args_reducer->R = R;
    args_reducer->id = id;
    args_reducer->mutexed = false;
    //args_reducer->tid = malloc(sizeof(pthread_t) * (M + R));
    //memcpy(args_reducer->tid, tid, sizeof(pthread_t) * (M + R));
    args_reducer->tid = tid;
    args_reducer->mapper_results = mapper_result;
    args_reducer->aggregate_list = aggregate_list;
}

void build_args_mapper_struct(struct args_mapper* args_mapper, int M, int R, int id, int N, char** input_files, 
                                struct node* mapper_result[]) {
    args_mapper->M = M;
    args_mapper->R = R;
    args_mapper->id = id;
    args_mapper->N = N;
    args_mapper->input_files = input_files; 
    args_mapper->mapper_results = mapper_result;
}

int raise_to_power(int number, int exponent) {
    int to_multiply = number;
    for (int i = exponent; i > 1; i--) {
        number *= to_multiply;
    }
    return number;
}

bool is_perfect(int number, int exponent) {
    
    if (number == 1)
        return true;
    if (number == 0)
        return false;

    int to_check = 2;
    //number = 243, exp = 5
    
    float radical = sqrt(number);
    while (true) {
        int check = raise_to_power(to_check, exponent);
        if (check > number)
            break;
        if (check == number)
            return true;
        to_check ++;
        if (to_check > radical) {
            break;
        }
    }

    return false;
}

void *mapper_function(void *arg) {
    
    struct args_mapper* args_mapper = (struct args_mapper*) arg;
    int start = args_mapper->id * (double) args_mapper->N / args_mapper->M;
    int end = min((args_mapper->id + 1) * (double) args_mapper->N / args_mapper->M, args_mapper->N);

    char* line = malloc(sizeof(char) * 100);

    for (int i = start; i < end; i++) {
        
        printf("%d %s\n", args_mapper->id, args_mapper->input_files[i]);

        FILE* input = fopen(args_mapper->input_files[i], "r");
        fgets(line, sizeof(char) * 100, input);
        int nr_of_numbers = atoi(line);
        
        for (int j = 0; j < nr_of_numbers; j++) {

            fgets(line, sizeof(char) * 100, input);
            int number = atoi(line);

            for (int k = 2; k < args_mapper->R + 2; k++) {

                if (is_perfect(number, k)) {
                    add(&args_mapper->mapper_results[k - 2], number);
                }
            }
        }

        fclose(input);
    }

    pthread_barrier_wait(&barrier);
    
    done = true;
    
    pthread_exit(NULL);
}

void *reducer_function(void *arg) {
    
    struct args_reducer* args_reducer = (struct args_reducer*) arg;
    pthread_mutex_lock(&mutex);
    while (!done) {
    }
    pthread_mutex_unlock(&mutex);

    struct node** al = args_reducer->aggregate_list;

    int reducer_id = args_reducer->id - args_reducer->M; //reducers are indexed after M

    for (int i = 0; i < args_reducer->M; i++) {
        for (; args_reducer->mapper_results[i][reducer_id] != NULL;
         args_reducer->mapper_results[i][reducer_id] = args_reducer->mapper_results[i][reducer_id]->next) {
            add(&args_reducer->aggregate_list[reducer_id], args_reducer->mapper_results[i][reducer_id]->val);
        }
    }
    
    int count = 0;
    for (; al[reducer_id] != NULL; al[reducer_id] = al[reducer_id]->next) {
        count++;
        if (is_already(al[reducer_id]->next, al[reducer_id]->val)) {
            count--;
        }
    }
    
    char* out_name = malloc(sizeof(char*) * 100);
    strcpy(out_name, "out");
    char* nr = malloc(sizeof(char*) * 100);
    sprintf(nr, "%d", args_reducer->id - args_reducer->M + 2);
    strcat(out_name, nr);
    strcat(out_name, ".txt");

    sprintf(nr, "%d", count);
    FILE* out = fopen(out_name, "w");
    fputs(nr, out);
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    int M, R;
    int N; //number of files that mappers will parse (read from FILE* in)
    FILE* in; //test.txt
    char** input_files; 

    get_args(argc, argv, &M, &R, &in);
    parse_in(in, &input_files, &N);

    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, M);

    pthread_t* tid = malloc(sizeof(pthread_t) * (M + R));

    struct node*** mapper_results;
    mapper_results = malloc(sizeof(struct node*) * M);
    for (int i = 0; i < M; i++) {
        mapper_results[i] = malloc(sizeof(struct node*) * R);
        for (int j = 0; j < R; j++) {
            mapper_results[i][j] = NULL; //init, vezi cum merge add din list.c
        }
    }

    struct node** aggregate_list = malloc(sizeof(struct node*) * R);
    for (int i = 0; i < R; i++) {
        aggregate_list[i] = NULL; //init, vezi cum merge add din list.c
    }

    struct args_reducer args_reducers[R];
    struct args_mapper args_mappers[M];
    
    for (int i = 0; i < M + R; i++) {
        
        if (i >= M) {
            build_args_reducer_struct(&args_reducers[M + R - i - 1], M, R, i, tid, mapper_results, aggregate_list);
            pthread_create(&tid[i], NULL, reducer_function, &args_reducers[M + R - i - 1]);
        } else {
            build_args_mapper_struct(&args_mappers[i], M, R, i, N, input_files, mapper_results[i]);
            pthread_create(&tid[i], NULL, mapper_function, &args_mappers[i]);
        }
    }

    for (int i = 0; i < M + R; i++) {
		pthread_join(tid[i], NULL);
	}

    pthread_barrier_destroy(&barrier);
    fclose(in);
    free(input_files);
    return 0;
}