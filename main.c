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
#define PROCESS_SUCCESS 't'
#define PROCESS_FAILURE 'f'
#define PROCESS_UNDEFINED 'u'
#define PROCESS_EXECUTING 'e'
#define G_STATUS_ADDRESS 0
#define F_STATUS_ADDRESS 1
#define CONTINUE_PROMPT_TIMEOUT_LENGTH 10
#define CHECK_OTHER_FUNCTION_TIMEOUT_LENGTH 1

/*
заємодія процесів. Паралелізм. mmap. 
Обчислити f(x) * g(x), використовуючи 2 допоміжні процеси: один обчислює f(x), а інший – g(x). 
Основна програма виконує ввод-вивід та операцію *. 
Використати mmap() доступ до спільного файлу. Функції f(x) та g(x) “нічого не знають друг про друга” і не можуть комунікувати між собою.
*/

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

void promptAlarmHandlerG(int signum) {
    printf("Execution of function g took too long. Do you want to continue? (y/n): ");
    char choice;
    scanf(" %c", &choice);
    if (choice != 'y' && choice != 'Y') {
        char *shared_memory = getSharedMemory();
        shared_memory[G_STATUS_ADDRESS] = PROCESS_UNDEFINED;
        printf("Execution aborted.\n");
        exit(0);
    }
}
void promptAlarmHandlerF(int signum) {
    printf("Execution of function f took too long. Do you want to continue? (y/n): ");
    char choice;
    scanf(" %c", &choice);
    if (choice != 'y' && choice != 'Y') {
        char *shared_memory = getSharedMemory();
        shared_memory[F_STATUS_ADDRESS] = PROCESS_UNDEFINED;
        printf("Execution aborted.\n");
        exit(0);
    }
}
void checkOtherFunctionAlarmHandlerG(int signum) {
    char *shared_memory = getSharedMemory();
    char f_res = shared_memory[F_STATUS_ADDRESS];
    printf("checking out f: %c", f_res)
}
void checkOtherFunctionAlarmHandlerF(int signum) {
    printf("Execution of function f took too long. Do you want to continue? (y/n): ");
    char choice;
    scanf(" %c", &choice);
    if (choice != 'y' && choice != 'Y') {
        printf("Execution aborted.\n");
        exit(0);
    }
}

typedef struct {
    int x;
    int y;
    int result;
} f_args;

typedef struct {
    double t;
    int result;
} g_args;

void *f(void *arg){
    signal(SIGALRM, promptAlarmHandlerF);
    
    f_args *targ = (f_args*)arg;
    int x = targ->x;
    int y = targ->y;
    int *result = malloc(sizeof(int));

    char *shared_memory = getSharedMemory();
    shared_memory[F_STATUS_ADDRESS] = PROCESS_EXECUTING;

    alarm(CONTINUE_PROMPT_TIMEOUT_LENGTH);
    while (1){
        sleep(SLEEP_INTERVAL);
        x -= y;
        if (x == 0){
            shared_memory[F_STATUS_ADDRESS] = PROCESS_SUCCESS;
            targ->result = 1;
            break;
        }
        if (x < 0){
            shared_memory[F_STATUS_ADDRESS] = PROCESS_FAILURE;
            targ->result = 0;
            break;
        }
    }
    alarm(0);
    pthread_exit(NULL);
}

void *g(void *arg){
    signal(SIGALRM, promptAlarmHandlerG);
    g_args *targ = (g_args*)arg;
    double t = targ->t;
    printf("%f\n", t);

    char *shared_memory = getSharedMemory();
    shared_memory[G_STATUS_ADDRESS] = PROCESS_EXECUTING;


    double result;
    int *res = malloc(sizeof(int));

    alarm(CONTINUE_PROMPT_TIMEOUT_LENGTH);
    result = t * t * t;
    printf("t^3 = %.2f\n", result);
    sleep(SLEEP_INTERVAL);

    result += t * t;
    printf("t^3 + t^2 = %.2f\n", result);
    sleep(SLEEP_INTERVAL);

    result += 10 * t;

    printf("t^3 + t^2 + 10t = %.2f\n", result);

    sleep(SLEEP_INTERVAL);

    double log_t5 = log(t) / log(5);
    if (isnan(log_t5)) {
        printf("Error: log_t(5) is undefined.\n");
        shared_memory[G_STATUS_ADDRESS] = PROCESS_FAILURE;
        targ->result = 0;
        pthread_exit(NULL);
    }
    result -= log_t5;
    printf("t^3 + t^2 + 10t - log_t(5) = %.2f\n", result);
    sleep(SLEEP_INTERVAL);

    shared_memory[G_STATUS_ADDRESS] = PROCESS_SUCCESS;
    alarm(0);
    targ->result = 1;
    pthread_exit(NULL);
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

    g_args g_args = { t: t };
    f_args f_args = { x: x, y: y };
    pthread_create(&g_thread, NULL, g, (void*)&g_args);
    pthread_create(&f_thread, NULL, f, (void*)&f_args);

    pthread_join(g_thread, NULL);
    pthread_join(f_thread, NULL);

    printf("g = %i\n", g_args.result);
    printf("f = %i\n", f_args.result);
    printf("g * f = %i\n", g_args.result * f_args.result);
    return 0;
}