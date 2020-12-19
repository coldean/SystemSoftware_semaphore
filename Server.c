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
char SERVER_SEM[] = "req_sem";
char CLIENT_SEM[] = "res_sem_";

typedef struct __ClientInfo {
    int pid;
    int isRequested;
} ClientInfo;

ClientInfo *pClientInfo;

key_t clientKey = 0;
key_t ci = 0;
int shmid = 0;
int *shmaddr = NULL;
int ciid = 0;
int *ciaddr = NULL;
sem_t *req, *res;

int main() {
    clientKey = ftok("mykey", 1);
    ci = ftok("ci_set", 0);

    signal(SIGINT, signalHandler);

    ciid = shmget(ci, CLIENT_NUM_MAX, IPC_CREAT | 0666);
    ciaddr = shmat(ciid, NULL, 0);

    req = sem_open(SERVER_SEM, O_CREAT | O_EXCL, 0644, 0);


    int addrcount = 0;

    int clientpid = 0;
    int pidIndex = 0;

    int count = 0;
    while (1) {

        sem_wait(req);
        printf("got req\n");

        char clientSeg[15] = {
            '\0',
        };
        char clientSem[15] = {
            '\0',
        };

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
        sprintf(clientSeg, "%s%d", CLIENT_SEM, pidIndex);
        sprintf(clientSem, "%s%d", SERVER_SEM, pidIndex);

        clientKey = ftok(clientSeg, 0);
        shmid = shmget(clientKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
        shmaddr = shmat(shmid, NULL, 0);

        res = sem_open(clientSem, 0, 0644, 1);

        count++;

        char str[50] = {
            '\0',
        };
        memcpy(&str, shmaddr, sizeof(str));
        printf("from client : %s, %d\n", str, clientpid);

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

        sem_unlink(SERVER_SEM);
        sem_unlink(CLIENT_SEM);

        exit(0);
    }
}
