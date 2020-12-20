#include "Lpc.h"
#include "LpcStub.h"
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

void signalHandler(int signum);
char REQ_SEM_[] = "req_sem";
char RES_SEM_[] = "res_sem_";

char REQ_SEG_[] = "req_seg_";
char RES_SEG_[] = "res_seg_";

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

int main(void) {
    long clientPid;
    LpcService lpcService;
    int numArg;
    LpcArg *lpcArgs;

    LpcRequest *lpcRequest;
    lpcRequest = malloc(sizeof(LpcRequest));

    ci = __makeKeyByName("ci_set");

    signal(SIGINT, signalHandler);

    ciid = shmget(ci, CLIENT_NUM_MAX * 8, IPC_CREAT | 0666);
    ciaddr = shmat(ciid, NULL, 0);
    memset(ciaddr, 0x00, CLIENT_NUM_MAX * 8);

    reqSem = sem_open(REQ_SEM_, O_CREAT , 0644, 0);

    int addrcount = 0;

    int ci_clientpid = 0;
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
                ci_clientpid = check.pid;
                break;
            }

            if (addrcount == CLIENT_NUM_MAX * 8) {
                printf("Error! no request\n");
            }
            addrcount += 8;
        }
        printf("addrcount : %d", addrcount);
        printf("client pid : %d", ci_clientpid);

        pidIndex = ci_clientpid % 1000;
        sprintf(serverSeg, "%s%d", RES_SEG_, pidIndex);
        sprintf(clientSeg, "%s%d", REQ_SEG_, pidIndex);
        sprintf(clientSem, "%s%d", REQ_SEM_, pidIndex);

        clientKey = __makeKeyByName(clientSeg);
        serverKey = __makeKeyByName(serverSeg);
        clientShmid = shmget(clientKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
        clientShmaddr = shmat(clientShmid, NULL, 0);
        serverShmid = shmget(serverKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
        serverShmaddr = shmat(serverShmid, NULL, 0);

        ClientInfo initCi; // isrequest 0으로 초기화
        initCi.pid = ci_clientpid;
        initCi.isRequested = 0;
        memcpy(ciaddr + addrcount, &initCi, 8);

        resSem = sem_open(clientSem, O_CREAT, 0644, 0);	// client에서 받는 과정

        memset(lpcRequest, 0x00, sizeof(LpcRequest));	// 이전 정보 초기화
        // count++;

        // char str[50] = {
        //     '\0',
        // };
        memcpy(lpcRequest, clientShmaddr, sizeof(LpcRequest));
        clientPid = lpcRequest->pid;
        lpcService = lpcRequest->service;
        numArg = lpcRequest->numArg;
        lpcArgs = lpcRequest->lpcArgs;

		///////////////// 이게 맞나?????????
        // sprintf(clientSem, "%s%ld", REQ_SEM_, clientPid % 1000);

        printf("\nclientSem : %s\n", clientSem);

        printf("from client : %ld, %d\n", clientPid, ci_clientpid);

        int fd, size, a;

        switch (lpcService) {
        case LPC_OPEN_FILE:
            fd = OpenFile(lpcRequest->lpcArgs[0].argData,
                          atoi(lpcRequest->lpcArgs[1].argData), clientPid);
            fflush(stdout);
            break;
        case LPC_READ_FILE:
            a = atoi(lpcRequest->lpcArgs[1].argData);
            size = ReadFile(atoi(lpcRequest->lpcArgs[0].argData),
                            atoi(lpcRequest->lpcArgs[1].argData), clientPid);
            break;
        case LPC_WRITE_FILE:
            WriteFile(atoi(lpcRequest->lpcArgs[0].argData),
                      lpcRequest->lpcArgs[1].argData,
                      atoi(lpcRequest->lpcArgs[2].argData), clientPid);
            break;
        case LPC_SEEK_FILE:
            SeekFile(atoi(lpcRequest->lpcArgs[0].argData),
                     atoi(lpcRequest->lpcArgs[1].argData),
                     atoi(lpcRequest->lpcArgs[2].argData), clientPid);
            break;
        case LPC_CLOSE_FILE:
            CloseFile(atoi(lpcRequest->lpcArgs[0].argData), clientPid);
            break;
        case LPC_MAKE_DIRECTORY:
            MakeDirectory(lpcRequest->lpcArgs[0].argData,
                          atoi(lpcRequest->lpcArgs[1].argData), clientPid);
            break;
        case LPC_REMOVE_DIRECTORY:
            RemoveDirectory(lpcRequest->lpcArgs[0].argData, clientPid);
            break;
        }

        // memset(clientShmaddr, 0x00, sizeof(clientShmaddr));
        // printf("memset seccess\n");
        // memcpy(clientShmaddr, &count, sizeof(int));
        // printf("memcpy success\n");
        printf("before semPost\n");
        sem_post(resSem);
        printf("sempost good\n");
    }

    return 0;
}
void signalHandler(int signum) {
    if (signum == SIGINT) {
        shmdt(clientShmaddr);
        shmctl(clientShmid, IPC_RMID, NULL);
        shmdt(serverShmaddr);
        shmctl(serverShmid, IPC_RMID, NULL);

        sem_close(reqSem);
        sem_close(resSem);

        sem_unlink(REQ_SEM_);
        sem_unlink(RES_SEM_);

        exit(0);
    }
}
