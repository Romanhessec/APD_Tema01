#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h> 
#include "list.h"

pthread_mutex_t mutex;
bool done = false;

struct args_reducer {
    int M, R, id;
    bool mutexed;
    pthread_t* tid;
};

struct args_mapper {
    int M, R, id, N;
    FILE** input_files;
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

void parse_in(FILE* in, FILE*** input_files, int* N) {
    char* line = malloc(sizeof(char) * 100);
    fgets(line, sizeof(line), in);
    *N = atoi(line);
    
    *input_files = malloc(sizeof(FILE*) * (*N));

    for (int i = 0; i < (*N); i++) {
        fgets(line, sizeof(char) * 100, in);
        line[strcspn(line, "\n")] = 0; //strip the '\n' character

        (*input_files)[i] = fopen(line, "r");
        
        if ((*input_files)[i] == NULL) {
            printf("Error: File given doesn't exist.\n");
            exit(1);
        }
    }
}

void close_all_files(FILE*** input_files, int* N) {
    for (int i = 0; i < (*N); i++) {
        fclose((*input_files)[i]);
    }
}

void build_args_reducer_struct(struct args_reducer* args_reducer, int M, int R, int id, pthread_t* tid) {
    args_reducer->M = M;
    args_reducer->R = R;
    args_reducer->id = id;
    args_reducer->mutexed = false;
    args_reducer->tid = malloc(sizeof(pthread_t) * (M + R));
    memcpy(args_reducer->tid, tid, sizeof(pthread_t) * (M + R));
}

void build_args_mapper_struct(struct args_mapper* args_mapper, int M, int R, int id, int N, FILE** input_files, 
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
    //number = 243, exp = 4
    while (true) {
        int check = raise_to_power(to_check, exponent);
        if (check > number)
            break;
        if (check == number)
            return true;
        to_check ++;
    }

    return false;
}

void *mapper_function(void *arg) {
    
    struct args_mapper* args_mapper = (struct args_mapper*) arg;
    int start = args_mapper->id * (double) args_mapper->N / args_mapper->M;
    int end = min((args_mapper->id + 1) * (double) args_mapper->N / args_mapper->M, args_mapper->N);

    char* line = malloc(sizeof(char) * 100);

    for (int i = start; i < end; i++) {

        fgets(line, sizeof(char) * 100, args_mapper->input_files[i]);
        int nr_of_numbers = atoi(line);
        
        for (int j = 0; j < nr_of_numbers; j++) {

            fgets(line, sizeof(char) * 100, args_mapper->input_files[i]);
            int number = atoi(line);

            for (int k = 2; k < args_mapper->R + 2; k++) {

                if (is_perfect(number, k)) {
                    add(&args_mapper->mapper_results[k - 2], number);
                }
            }
        }

        fclose(args_mapper->input_files[i]);
    }

    pthread_exit(NULL);
}

void *reducer_function(void *arg) {
    
    struct args_reducer* args_reducer = (struct args_reducer*) arg;
    
    pthread_mutex_lock(&mutex);
    if (done == false) {
        for (int i = 0; i < args_reducer->M; i++) {
            pthread_join(args_reducer->tid[i], NULL);
	    }
        done = true;
    } 
    pthread_mutex_unlock(&mutex);
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    int M, R;
    int N; //number of files that mappers will parse (read from FILE* in)
    FILE* in; //test.txt
    FILE** input_files;
    
    pthread_mutex_init(&mutex, NULL);

    get_args(argc, argv, &M, &R, &in);
    parse_in(in, &input_files, &N);

    pthread_t* tid = malloc(sizeof(pthread_t) * (M + R));

    struct node* mapper_results[M][R];

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < R; j++) {
            mapper_results[i][j] = NULL; //init, vezi cum merge add din list.c
        }
    }

    struct args_reducer args_reducers[R];
    struct args_mapper args_mappers[M];

    for (int i = 0; i < M + R; i++) {

        if (i >= M) {
            build_args_reducer_struct(&args_reducers[M + R - i - 1], M, R, i, tid);
            pthread_create(&tid[i], NULL, reducer_function, &args_reducers[M + R - i - 1]);
        } else {
            build_args_mapper_struct(&args_mappers[i], M, R, i, N, input_files, mapper_results[i]);
            pthread_create(&tid[i], NULL, mapper_function, &args_mappers[i]);
        }
    }
    
    for (int i = M; i < M + R; i++) {
		pthread_join(tid[i], NULL);
	}

    fclose(in);
    free(input_files);
    return 0;
}