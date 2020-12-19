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

char SERVER_SEM[] = "req_sem";
char CLIENT_SEM[] = "res_sem_";

char CLIENT_SEG[] = "req_seg_";

typedef struct __ClientInfo {
    int pid;
    int isRequested;
} ClientInfo;

void signalHandler(int signum);

key_t clientKey = 0;
key_t ci = 0;
int shmid = 0;
int *shmaddr = NULL;
int ciid = 0;
int *ciaddr = NULL;
sem_t *req, *res;

char clientSeg[15] = {'\0',};
char clientSem[15] = {'\0',};


int main() {
    signal(SIGINT, signalHandler); // signalHandler

    int pid = getpid();
    int pidIndex = pid % 1000;


    sprintf(clientSeg, "%s%d", CLIENT_SEM, pidIndex);
    sprintf(clientSem, "%s%d", SERVER_SEM, pidIndex);

    printf("%s\n", clientSeg);
    clientKey = ftok(clientSeg, 0);
    ci = ftok("ci_set", 0);

    shmid = shmget(clientKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
    shmaddr = shmat(shmid, NULL, 0);

    ciid = shmget(ci, CLIENT_NUM_MAX, IPC_CREAT | 0666);
    ciaddr = shmat(ciid, NULL, 0);

    req = sem_open(SERVER_SEM, 0, 0644, 0);
    res = sem_open(clientSem, O_CREAT, 0644, 0);

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

        memset(shmaddr, 0x00, sizeof(shmaddr)); // 내용 초기화
        memcpy(shmaddr, &str, sizeof(str));

        printf("shmaddr success\n");
        ClientInfo *cli = malloc(sizeof(ClientInfo));
        cli->pid = pid;
        cli->isRequested = 1;

        printf("pidIndex : %d\n", pidIndex);
        memcpy(ciaddr + 8 * pidIndex, cli, sizeof(ClientInfo));
        printf("ci success\n");

        sem_post(req);

        sem_wait(res);
        memcpy(&recieve, shmaddr, sizeof(int));
        printf("from server : %d\n", recieve);
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
