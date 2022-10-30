#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct args_reducer{
    int M, R, id;
    pthread_t* tid;
};

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
    
    args_reducer->tid = malloc(sizeof(pthread_t) * (M + R));
    memcpy(args_reducer->tid, tid, sizeof(pthread_t) * (M + R));
}

void *mapper_function(void *arg) {
    //int thread_id = *(int *)arg;
    
    pthread_exit(NULL);
}

void *reducer_function(void *arg) {
    
    struct args_reducer* args_reducer = (struct args_reducer*) arg;
    
    for (int i = 0; i < args_reducer->M; i++) {
        pthread_join(args_reducer->tid[i], NULL);
	}

    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    int M, R;
    int N; //number of files that mappers will parse (read from FILE* in)
    FILE* in; //test.txt
    FILE** input_files;
    
    get_args(argc, argv, &M, &R, &in);
    parse_in(in, &input_files, &N);

    int thread_id[M + R];
    pthread_t* tid = malloc(sizeof(pthread_t) * (M + R));

    struct args_reducer args_reducer[R];
      
    for (int i = 0; i < M + R; i++) {
        thread_id[i] = i;
        if (i >= M) {
            build_args_reducer_struct(&args_reducer[M + R - i - 1], M, R, i, tid);
            pthread_create(&tid[i], NULL, reducer_function, &args_reducer[M + R - i - 1]);
        } else {
            pthread_create(&tid[i], NULL, mapper_function, &thread_id[i]);
        }
    }

    for (int i = M; i < M + R; i++) {
		pthread_join(tid[i], NULL);
	}

    fclose(in);
    close_all_files(&input_files, &N);
    free(input_files);
    return 0;
}