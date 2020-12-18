#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SHM_SIZE 512
char REQ_NAME[] = "fuckone";
char RES_NAME[] = "fucktwo";

void signalHandler(int signum);

key_t mykey = 0;
int shmid = 0;
int *shmaddr = NULL;
    sem_t *req, *res;

int main() {
    mykey = ftok("mykey", 1);
    shmid = shmget(1234, MAX_SHM_SIZE, IPC_CREAT | 0666);
    shmaddr = shmat(shmid, NULL, 0);

    req = sem_open(REQ_NAME, 0, 0644, 0);
    res = sem_open(RES_NAME, 0, 0644, 0);

    while(1){
        printf("waiting...\n");
        sem_wait(res);
        char str[50] = {
            '\0',
        };
        int recieve;
        printf("<< ");
        scanf("%s", str);
        fflush(stdout);
        fflush(stdin);

        memset(shmaddr, 0x00, sizeof(shmaddr)); // 내용 초기화
        memcpy(shmaddr, &str, sizeof(str));
        sem_post(req);

        sem_wait(res);
        memcpy(&recieve, shmaddr, sizeof(int));
        printf("from server : %d\n", recieve);
        sem_post(res);
    }
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        shmdt(shmaddr);
        shmctl(shmid, IPC_RMID, NULL);

        sem_close(req);
        sem_close(res);

        exit(0);
    }
}
