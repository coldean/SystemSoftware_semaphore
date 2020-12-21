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
#define CLIENT_NUM_MAX 8192

char REQ_SEM[] = "req_sem";
char RES_SEM[] = "res_sem_";

char REQ_SEG[] = "req_seg_";
char RES_SEG[] = "res_seg_";

typedef struct __ClientInfo {
    int pid;
    int isRequested;
} ClientInfo;

void signalHandler(int signum);

key_t requestKey = 0;
key_t responseKey = 0;
key_t ci = 0;
int requestShmid = 0;
int *requestShmaddr = NULL;
int responseShmid = 0;
int *responseShmaddr = NULL;
int ciid = 0;
int *ciaddr = NULL;
sem_t *reqSem, *resSem;

char serverSeg[15] = {'\0',};
char clientSeg[15] = {'\0',};
char clientSem[15] = {'\0',};


int main() {
    signal(SIGINT, signalHandler); // signalHandler

    int pid = getpid();
    int pidIndex = pid % 1000;

    sprintf(serverSeg, "%s%d", RES_SEG, pidIndex);
    sprintf(clientSeg, "%s%d", REQ_SEG, pidIndex);
    sprintf(clientSem, "%s%d", REQ_SEM, pidIndex);

    printf("%s\n", clientSeg);
    requestKey = ftok(clientSeg, 0);
    responseKey = ftok(serverSeg, 0);
    ci = ftok("ci_set", 0);

    requestShmid = shmget(requestKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
    requestShmaddr = shmat(requestShmid, NULL, 0);
    responseShmid = shmget(responseKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
    responseShmaddr = shmat(responseShmid, NULL, 0);

    ciid = shmget(ci, CLIENT_NUM_MAX, IPC_CREAT | 0666);
    ciaddr = shmat(ciid, NULL, 0);

    reqSem = sem_open(REQ_SEM, 0, 0644, 0);
    resSem = sem_open(clientSem, O_CREAT, 0644, 0);

    while (1) {
        printf("waiting...\n");
        char str[50] = {
            '\0',
        };
        int recieve;
        printf("<< ");
        scanf("%s", str);
        fflush(stdout);
        fflush(stdin);

        memset(requestShmaddr, 0x00, sizeof(requestShmaddr)); // 내용 초기화
        memcpy(requestShmaddr, &str, sizeof(str));

        printf("shmaddr success\n");
        ClientInfo *cli = malloc(sizeof(ClientInfo));
        cli->pid = pid;
        cli->isRequested = 1;

        printf("pidIndex : %d\n", pidIndex);
        memcpy(ciaddr + 8 * pidIndex, cli, sizeof(ClientInfo));
        printf("ci success\n");

        sem_post(reqSem);

        sem_wait(resSem);
        memcpy(&recieve, responseShmaddr, sizeof(int));
        printf("from server : %d\n", recieve);
    }
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        shmdt(requestShmaddr);
        shmctl(requestShmid, IPC_RMID, NULL);
        shmdt(responseShmaddr);
        shmctl(responseShmid, IPC_RMID, NULL);

        sem_close(reqSem);
        sem_close(resSem);

        sem_unlink(REQ_SEM);
        sem_unlink(clientSem);

        exit(0);
    }
}
