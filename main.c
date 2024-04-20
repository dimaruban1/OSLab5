#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define SLEEP_INTERVAL 3
#define SHARED_MEMORY_SIZE 4096
/*
заємодія процесів. Паралелізм. mmap. 
Обчислити f(x) * g(x), використовуючи 2 допоміжні процеси: один обчислює f(x), а інший – g(x). 
Основна програма виконує ввод-вивід та операцію *. 
Використати mmap() доступ до спільного файлу. Функції f(x) та g(x) “нічого не знають друг про друга” і не можуть комунікувати між собою.
*/
int f(int x, int y){
    char *shared_memory;
    int fd = open("shared_memory.bin", O_CREAT | O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, 4096) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    int result = x * y;

    if (result % 2 == 0) {
        while (1) {
            sleep(SLEEP_INTERVAL);
        }
    }

    return result;
}

int g(int z){
    if (z < 0){
        sleep(3);
        return -1;
    }
    else if (z > 0) {
        sleep(SLEEP_INTERVAL);
        return 1;
    }
    while(1){
        z++;
    }
    return 0;
}

int main(){

    std::cout << "hello world" << std::endl;
    g(15);
    g(-1);
    f(1,2);
    return 0;
}