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
#define CLIENT_NUM_MAX 8192

void signalHandler(int signum);
char REQ_SEM[] = "req_sem";
char RES_SEM[] = "res_sem_";

char REQ_SEG[] = "req_seg_";
char RES_SEG[] = "res_seg_";

typedef struct __ClientInfo {
    int pid;
    int isRequested;
} ClientInfo;

ClientInfo *pClientInfo;

key_t clientKey = 0;
key_t serverKey = 0;
key_t ci = 0;
int clientShmid = 0;
int *clientShmaddr = NULL;
int serverShmid = 0;
int *serverShmaddr = NULL;
int ciid = 0;
int *ciaddr = NULL;
sem_t *reqSem, *resSem;

int main() {
    // clientKey = ftok("mykey", 1);
    ci = ftok("ci_set", 0);

    signal(SIGINT, signalHandler);

    ciid = shmget(ci, CLIENT_NUM_MAX, IPC_CREAT | 0666);
    ciaddr = shmat(ciid, NULL, 0);

    reqSem = sem_open(REQ_SEM, O_CREAT | O_EXCL, 0644, 0);


    int addrcount = 0;

    int clientpid = 0;
    int pidIndex = 0;

    int count = 0;
    while (1) {

        sem_wait(reqSem);
        printf("got req\n");

        char serverSeg[15] = {
            '\0',
        };
        char clientSeg[15] = {
            '\0',
        };
        char clientSem[15] = {
            '\0',
        };

        addrcount = 0;
        while (1) {
            ClientInfo check;
            memcpy(&check, ciaddr + addrcount, 8);

            if (check.isRequested == 1) {
                clientpid = check.pid;
                break;
            }

            if (addrcount == CLIENT_NUM_MAX) {
                printf("Error! no request\n");
            }
            addrcount += 8;
        }
        printf("client pid : %d", clientpid);

        pidIndex = clientpid % 1000;
        sprintf(serverSeg, "%s%d", RES_SEG, pidIndex);
        sprintf(clientSeg, "%s%d", REQ_SEG, pidIndex);
        sprintf(clientSem, "%s%d", REQ_SEM, pidIndex);

        clientKey = ftok(clientSeg, 0);
        serverKey = ftok(serverSeg, 0);
        clientShmid = shmget(clientKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
        clientShmaddr = shmat(clientShmid, NULL, 0);
        serverShmid = shmget(serverKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
        serverShmaddr = shmat(serverShmid, NULL, 0);

        ClientInfo initCi;      // isrequest 0으로 초기화
        initCi.pid = clientpid;
        initCi.isRequested = 0;
        memcpy(ciaddr + addrcount, &initCi, 8);

        resSem = sem_open(clientSem, 0, 0644, 1);

        count++;

        char str[50] = {
            '\0',
        };
        memcpy(&str, clientShmaddr, sizeof(str));
        printf("from client : %s, %d\n", str, clientpid);

        memset(serverShmaddr, 0x00, sizeof(clientShmaddr));
        printf("memset seccess\n");
        memcpy(serverShmaddr, &count, sizeof(int));
        printf("memcpy success\n");
        sem_post(resSem);
    }
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        shmdt(clientShmaddr);
        shmctl(clientShmid, IPC_RMID, NULL);
        shmdt(serverShmaddr);
        shmctl(serverShmid, IPC_RMID, NULL);

        sem_close(reqSem);
        sem_close(resSem);

        sem_unlink(REQ_SEM);
        sem_unlink(RES_SEM);

        exit(0);
    }
}
