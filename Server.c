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
#include <error.h>

#define MAX_SHM_SIZE 512

void signalHandler(int signum);
char REQ_SEM_[] = "req_sem";
char RES_SEM_[] = "res_sem_";

char REQ_SEG_[] = "req_seg_";
char RES_SEG_[] = "res_seg_";

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

char responseSem[15];

int main(void) {
    long clientPid;
    LpcService lpcService;
    int numArg;
    LpcArg *lpcArgs;

    LpcRequest *lpcRequest;
    lpcRequest = malloc(sizeof(LpcRequest));

    ci = __makeKeyByName("ci_set");

    signal(SIGINT, signalHandler);
    signal(SIGSEGV, signalHandler);

    ciid = shmget(ci, CLIENT_NUM_MAX * 8, IPC_CREAT | 0777);
    pClientInfo = shmat(ciid, NULL, 0);
    memset(pClientInfo, 0x00, CLIENT_NUM_MAX * 8);
    reqSem = sem_open(REQ_SEM_, O_CREAT, 0777, 0);
    int addrcount = 0;

    int ci_clientpid = 0;
    int pidIndex = 0;

    int count = 0;
    while (1) {

        sem_wait(reqSem);

        char responseSeg[15] = {
            '\0',
        };
        char requestSeg[15] = {
            '\0',
        };
        char responseSem[15] = {
            '\0',
        };

        addrcount = 0;
        while (1) {
            if (pClientInfo[addrcount].isRequested == 1) {
                ci_clientpid = pClientInfo->pid;
                break;
            }

            if (addrcount > CLIENT_NUM_MAX) {
                perror("Error! no request\n");
            }
            addrcount += 1;
        }

        pidIndex = ci_clientpid % 1000;
        sprintf(responseSeg, "%s%d", RES_SEG_, pidIndex);
        sprintf(requestSeg, "%s%d", REQ_SEG_, pidIndex);
        sprintf(responseSem, "%s%d", RES_SEM_, pidIndex);

        requestKey = __makeKeyByName(requestSeg);
        responseKey = __makeKeyByName(responseSeg);
        requestShmid = shmget(requestKey, MAX_SHM_SIZE, IPC_CREAT | 0777);
        requestShmaddr = shmat(requestShmid, NULL, 0);
        responseShmid = shmget(responseKey, MAX_SHM_SIZE, IPC_CREAT | 0777);
        responseShmaddr = shmat(responseShmid, NULL, 0);

        pClientInfo[addrcount].isRequested = 0;

        resSem =
            sem_open(responseSem, O_CREAT, 0777, 0); // client에서 받는 과정

        memset(lpcRequest, 0x00, sizeof(LpcRequest)); // 이전 정보 초기화
        memcpy(lpcRequest, requestShmaddr, sizeof(LpcRequest));
        clientPid = lpcRequest->pid;
        lpcService = lpcRequest->service;
        numArg = lpcRequest->numArg;
        lpcArgs = lpcRequest->lpcArgs;

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

        sem_post(resSem);
        sem_close(resSem);
    }

    return 0;
}
void signalHandler(int signum) {
    if (signum == SIGINT) {
        shmdt(requestShmaddr);
        shmctl(requestShmid, IPC_RMID, NULL);
        shmdt(responseShmaddr);
        shmctl(responseShmid, IPC_RMID, NULL);

        sem_close(reqSem);
        sem_close(resSem);

        sem_unlink(REQ_SEM_);
        sem_unlink(responseSem);

        exit(0);
    }
    if (signum == SIGSEGV){
        shmdt(requestShmaddr);
        shmctl(requestShmid, IPC_RMID, NULL);
        shmdt(responseShmaddr);
        shmctl(responseShmid, IPC_RMID, NULL);

        sem_close(reqSem);
        sem_close(resSem);

        sem_unlink(REQ_SEM_);
        sem_unlink(responseSem);
        signal(signum, SIG_DFL);
    }
}
