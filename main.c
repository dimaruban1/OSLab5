#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define SLEEP_INTERVAL 3
#define SHARED_MEMORY_SIZE 4096
#define PROCESS_SUCCESS 't'
#define PROCESS_FAILURE 'f'
#define PROCESS_UNDEFINED 'u'
#define G_STATUS_ADDRESS 0
#define F_STATUS_ADDRESS 1


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
typedef struct {
    int x;
    int y;
} f_args;
int f(void *arg){
    signal(SIGALRM, alarmHandler);
    f_args temp = *((f_args*)arg);
    int x = temp.x;
    int y = temp.y;

    char *shared_memory = getSharedMemory();
    shared_memory[F_STATUS_ADDRESS] = PROCESS_UNDEFINED;

    while (1){
        x -= y;
        sleep(SLEEP_INTERVAL);
        if (x == 0){
            shared_memory[F_STATUS_ADDRESS] = PROCESS_SUCCESS;
            return 1;
        }
        if (x < 0){
            shared_memory[F_STATUS_ADDRESS] = PROCESS_FAILURE;
            return 0;
        }
    }
    return 0;
}

int g(void *arg){
    signal(SIGALRM, alarmHandler);
    double t = *((double*)arg);

    char *shared_memory = getSharedMemory();
    shared_memory[G_STATUS_ADDRESS] = PROCESS_UNDEFINED;


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

    double log_t5 = log(t) / log(5);
    if (isnan(log_t5)) {
        printf("Error: log_t(5) is undefined.\n");
        shared_memory[G_STATUS_ADDRESS] = PROCESS_FAILURE;
        return 0;
    }
    result -= log_t5;
    printf("t^3 + t^2 + 10t - log_t(5) = %.2f\n", result);
    sleep(SLEEP_INTERVAL);

    shared_memory[G_STATUS_ADDRESS] = PROCESS_SUCCESS;
    return 1;
}

void alarmHandler(int signum) {
    printf("Execution took too long. Do you want to continue? (y/n): ");
    char choice;
    scanf(" %c", &choice);
    if (choice != 'y' && choice != 'Y') {
        printf("Execution aborted.\n");
        exit(0); // Exit the program
    }
}

int main(){
    int x,y;
    double t;
    pthread_t g_thread, f_thread;
    int g_res, f_res;

    // Create two threads
    pthread_create(&g_thread, NULL, g, (void*)&x);
    pthread_create(&f_thread, NULL, f, NULL);

    printf("Enter the value of x (an integer): ");
    scanf("%d", &x);

    printf("Enter the value of y (an integer): ");
    scanf("%d", &y);

    printf("Enter the value of t (a double): ");
    scanf("%lf", &t);
    pthread_join(g_thread, NULL);
    pthread_join(f_thread, NULL);
    
    return 0;
}