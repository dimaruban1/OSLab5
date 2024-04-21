#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>

#define SLEEP_INTERVAL 3
#define SHARED_MEMORY_SIZE 4096
#define TRUE 'T'
#define FALSE 'F'
#define UNDEFINED 'U'
#define EXECUTING 'E'
#define G_STATUS_ADDRESS 0
#define F_STATUS_ADDRESS 1
#define CONTINUE_PROMPT_TIMEOUT_LENGTH 13
#define CONTINUE_PROMPT_TIMEOUT_LENGTH 21
#define CHECK_STATUSES_TIMEOUT_LENGTH 2
#define RESULT_NOT_SET -1

/*
заємодія процесів. Паралелізм. mmap. 
Обчислити f(x) * g(x), використовуючи 2 допоміжні процеси: один обчислює f(x), а інший – g(x). 
Основна програма виконує ввод-вивід та операцію *. 
Використати mmap() доступ до спільного файлу. Функції f(x) та g(x) “нічого не знають друг про друга” і не можуть комунікувати між собою.
*/

typedef struct {
    int x;
    int y;
    int result;
} f_args;

typedef struct {
    double t;
    int result;
} g_args;


char* getSharedMemory(){
    char* shared_memory;

    int fd = open("shared_memory.bin", O_CREAT | O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, SHARED_MEMORY_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return shared_memory;
}


char isResultDetermined(){
    char *shared_memory = getSharedMemory();
    char f_res = shared_memory[F_STATUS_ADDRESS];
    char g_res = shared_memory[G_STATUS_ADDRESS];
    printf("checking out f: %c\n", f_res);
    switch (f_res){
        case FALSE:
            return FALSE;
            break;
        default:
            break;
    }
    printf("checking out g: %c\n", g_res);
    switch (g_res){
        case FALSE:
            return FALSE;
            break;
        default:
            break;
    }
    return EXECUTING;
}

void *f(void *arg){
    f_args *targ = (f_args*)arg;
    int x = targ->x;
    int y = targ->y;

    char *shared_memory = getSharedMemory();
    shared_memory[F_STATUS_ADDRESS] = EXECUTING;

    while (1){
        sleep(SLEEP_INTERVAL);
        x -= y;
        if (x == 0){
            shared_memory[F_STATUS_ADDRESS] = TRUE;
            targ->result = TRUE;
            break;
        }
        if (x < 0){
            shared_memory[F_STATUS_ADDRESS] = FALSE;
            targ->result = FALSE;
            break;
        }
    }
    pthread_exit(NULL);
}

void *g(void *arg){
    g_args *targ = (g_args*)arg;
    double t = targ->t;
    printf("%f\n", t);

    char *shared_memory = getSharedMemory();
    shared_memory[G_STATUS_ADDRESS] = EXECUTING;


    double result;

    result = t * t * t;
    printf("t^3 = %.2f\n", result);
    sleep(SLEEP_INTERVAL);

    result += t * t;
    printf("t^3 + t^2 = %.2f\n", result);
    sleep(SLEEP_INTERVAL);

    result += 10 * t;

    printf("t^3 + t^2 + 10t = %.2f\n", result);

    sleep(SLEEP_INTERVAL);

    if (t <= 0) {
        printf("Error: t can`t be negative or zero.\n");
        shared_memory[G_STATUS_ADDRESS] = FALSE;
        targ->result = FALSE;
        pthread_exit(NULL);
    }
    double log_t5 = log(t) / log(5);

    result -= log_t5;
    printf("t^3 + t^2 + 10t - log_t(5) = %.2f\n", result);
    sleep(SLEEP_INTERVAL);

    shared_memory[G_STATUS_ADDRESS] = TRUE;
    targ->result = TRUE;
    pthread_exit(NULL);
}

void *executeG(void *arg) {
    g_args *targ = (g_args*)arg;
    pthread_t function_thread;
    pthread_create(&function_thread, NULL, g, (void*)targ);
    int time_executing = 0;
    while(1){
       if (targ->result != RESULT_NOT_SET){
            printf("g result set = %c\n", targ->result);
            break;
        }
        sleep(CHECK_STATUSES_TIMEOUT_LENGTH);
        char res = isResultDetermined();
        if (res != EXECUTING){
            pthread_cancel(function_thread);
            targ->result = res;
        }

        time_executing += CHECK_STATUSES_TIMEOUT_LENGTH;
        if (time_executing >= CONTINUE_PROMPT_TIMEOUT_LENGTH){
            time_executing = 0;
            printf("Execution of function g is taking long. Do you want to continue? (y/n): ");
            char choice;
            scanf(" %c", &choice);
            if (choice != 'y' && choice != 'Y') {
                char *shared_memory = getSharedMemory();
                shared_memory[G_STATUS_ADDRESS] = UNDEFINED;
                targ->result = UNDEFINED;
                printf("Execution of g aborted.\n");
                pthread_cancel(function_thread);
                break;
            }
        }
    }
}
void *executeF(void *arg) {
    f_args *targ = (f_args*)arg;
    pthread_t function_thread;
    pthread_create(&function_thread, NULL, f, (void*)targ);
    int time_executing = 0;
    while(1){
        if (targ->result != RESULT_NOT_SET){
            printf("f result set = %i\n", targ->result);
            break;
        }
        sleep(CHECK_STATUSES_TIMEOUT_LENGTH);
        char res = isResultDetermined();
        if (res != EXECUTING){
            pthread_cancel(function_thread);
            targ->result = UNDEFINED;
            break;
        }

        time_executing += CHECK_STATUSES_TIMEOUT_LENGTH;
        if (time_executing >= CONTINUE_PROMPT_TIMEOUT_LENGTH){
            time_executing = 0;
            printf("Execution of function f is taking long. Do you want to continue? (y/n): ");
            char choice;
            scanf(" %c", &choice);
            if (choice != 'y' && choice != 'Y') {
                char *shared_memory = getSharedMemory();
                shared_memory[F_STATUS_ADDRESS] = UNDEFINED;
                targ->result = UNDEFINED;
                printf("Execution of f aborted.\n");
                pthread_cancel(function_thread);
                break;
            }
        }
    }
}

char dot(char left, char right){
    if (left == FALSE || right == FALSE){
        return FALSE;
    }
    else if (left == UNDEFINED || right == UNDEFINED){
        return UNDEFINED;
    }
    else if (left == TRUE && right == TRUE){
        return TRUE;
    }
    return UNDEFINED;
}

int main(){
    int x,y;
    double t;
    pthread_t g_thread, f_thread;
    int g_res, f_res;

    printf("Enter the value of x (an integer): ");
    scanf("%d", &x);

    printf("Enter the value of y (an integer): ");
    scanf("%d", &y);

    printf("Enter the value of t (a double): ");
    scanf("%lf", &t);

    g_args g_args = { t: t, result: RESULT_NOT_SET };
    f_args f_args = { x: x, y: y, result: RESULT_NOT_SET };
    pthread_create(&g_thread, NULL, executeG, (void*)&g_args);
    pthread_create(&f_thread, NULL, executeF, (void*)&f_args);

    pthread_join(g_thread, NULL);
    pthread_join(f_thread, NULL);

    printf("g = %c\n", g_args.result);
    printf("f = %c\n", f_args.result);
    printf("g * f = %c\n", dot(g_args.result, f_args.result));
    return 0;
}