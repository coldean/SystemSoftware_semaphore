#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_SHM_SIZE 512

void signalHandler(int signum);
char REQ_NAME[] = "fuckone";
char RES_NAME[] = "fucktwo";

key_t mykey = 0;
int shmid = 0;
int *shmaddr = NULL;
sem_t *req, *res;

int main() {
    mykey = ftok("mykey", 1);
    shmid = shmget(1234, MAX_SHM_SIZE, IPC_CREAT | 0666);
    shmaddr = shmat(shmid, NULL, 0);

    sem_unlink(REQ_NAME);   // 시작 전에 이거 안써주면 세마포어가
    sem_unlink(RES_NAME);   // 존재할경우 이미 있는 세마포어 씀

    req = sem_open(REQ_NAME, O_CREAT | O_EXCL, 0644, 0);
    res = sem_open(RES_NAME, O_CREAT | O_EXCL, 0644, 1);

    int count = 0;
    while (1) {
        sem_wait(req);
        count++;

        char str[50] = {
            '\0',
        };
        memcpy(&str, shmaddr, sizeof(str));
        printf("from client : %s\n", str);

        memset(shmaddr, 0x00, sizeof(shmaddr));
        memcpy(shmaddr, &count, sizeof(int));
        sem_post(res);
    }
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        shmdt(shmaddr);
        shmctl(shmid, IPC_RMID, NULL);

        sem_close(req);
        sem_close(res);

        sem_unlink(REQ_NAME);
        sem_unlink(RES_NAME);

        exit(0);
    }
}
