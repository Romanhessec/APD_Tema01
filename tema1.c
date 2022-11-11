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

bool nth_root(unsigned int n, unsigned int k) {
    if (n == 0)
        return false;
        
    unsigned int a = n, b, c, r = k ? n + (n > 1) : n == 1 ;
    for (; a < r; b = a + (k - 1) * r, a = b / k)
        for (r = a, a = n, c = k - 1; c && (a /= r); --c);

    unsigned int toMultiply = r;
    for (int i = k; i > 1; i--){
        r *= toMultiply;
    }
    return (n == r);
}

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

void build_args_reducer_struct(struct args_reducer* args_reducer, int M, int R, int id, pthread_t* tid, struct node*** mapper_result, struct node** aggregate_list) {
    args_reducer->M = M;
    args_reducer->R = R;
    args_reducer->id = id;
    args_reducer->mutexed = false;
    args_reducer->tid = tid;
    args_reducer->mapper_results = mapper_result;
    args_reducer->aggregate_list = aggregate_list;
}

void build_args_mapper_struct(struct args_mapper* args_mapper, int M, int R, int id, int N, char** input_files, struct node* mapper_result[]) {
    args_mapper->M = M;
    args_mapper->R = R;
    args_mapper->id = id;
    args_mapper->N = N;
    args_mapper->input_files = input_files; 
    args_mapper->mapper_results = mapper_result;
}

void write_in_file(int nr, int count) {
    char* out_name = malloc(sizeof(char*) * 100);
    strcpy(out_name, "out");
    char* number_char = malloc(sizeof(char*) * 100);
    sprintf(number_char, "%d", nr);
    strcat(out_name, number_char);
    strcat(out_name, ".txt");

    sprintf(number_char, "%d", count);
    FILE* out = fopen(out_name, "w");
    fputs(number_char, out);
}

void *mapper_function(void *arg) {
    
    struct args_mapper* args_mapper = (struct args_mapper*) arg;
    int start = args_mapper->id * (double) args_mapper->N / args_mapper->M;
    int end = min((args_mapper->id + 1) * (double) args_mapper->N / args_mapper->M, args_mapper->N);

    char* line = malloc(sizeof(char) * 100);

    for (int i = start; i < end; i++) {

        FILE* input = fopen(args_mapper->input_files[i], "r");
        fgets(line, sizeof(char) * 100, input);
        int nr_of_numbers = atoi(line);
        
        for (int j = 0; j < nr_of_numbers; j++) {

            fgets(line, sizeof(char) * 100, input);
            int number = atoi(line);

            for (int k = 2; k < args_mapper->R + 2; k++) {
                if (nth_root(number, k)) {
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

    //what the fuck did this code do
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
    
    write_in_file(args_reducer->id - args_reducer->M + 2, count);
    
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