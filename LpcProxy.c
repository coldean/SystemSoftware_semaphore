#include "LpcProxy.h"
#include "Lpc.h"
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
#define PERMS 0600

char REQ_SEM[] = "req_sem";
char RES_SEM[] = "res_sem_";

char REQ_SEG[] = "req_seg_";
char RES_SEG[] = "res_seg_";

key_t requestKey;
key_t responseKey;
key_t ci;
int requestShmid;
int *requestShmaddr;
int responseShmid;
int *responseShmaddr;
int ciid;
int *ciaddr;
sem_t *reqSem, *resSem;

char responseSem[15];

int pidIndex;

void Init(void) {
    char responseSeg[15] = {
        '\0',
    };
    char requestSeg[15] = {
        '\0',
    };
    char responseSem[15] = {
        '\0',
    };
    int pid = getpid();
    pidIndex = pid % 1000;

    sprintf(responseSeg, "%s%d", RES_SEG, pidIndex);
    sprintf(requestSeg, "%s%d", REQ_SEG, pidIndex);
    sprintf(responseSem, "%s%d", RES_SEM, pidIndex);

    requestKey = __makeKeyByName(requestSeg);
    responseKey = __makeKeyByName(responseSeg);
    ci = __makeKeyByName("ci_set");

    requestShmid = shmget(requestKey, MAX_SHM_SIZE, IPC_CREAT | 0777);
    requestShmaddr = shmat(requestShmid, NULL, 0);

    responseShmid = shmget(responseKey, MAX_SHM_SIZE, IPC_CREAT | 0777);
    responseShmaddr = shmat(responseShmid, NULL, 0);

    ciid = shmget(ci, CLIENT_NUM_MAX * 8, IPC_CREAT | 0777);
    pClientInfo = shmat(ciid, NULL, 0);

    reqSem = sem_open(REQ_SEM, 0, 0777, 0);
    resSem = sem_open(responseSem, O_CREAT, 0777, 0);
}

int OpenFile(char *path, int flags) {
    Init();

    long cpid = getpid();

    LpcRequest lpcRequest;
    LpcArg lpcArg0, lpcArg1;

    memset(&lpcRequest, 0x00, sizeof(LpcRequest));

    lpcArg0.argSize = sizeof(path);
    strcpy(lpcArg0.argData, path);
    lpcArg1.argSize = 4;
    lpcArg1.argData[0] = flags;
    sprintf(lpcArg1.argData, "%d", flags);

    lpcRequest.pid = cpid;
    lpcRequest.service = LPC_OPEN_FILE;
    lpcRequest.numArg = 2;
    lpcRequest.lpcArgs[0] = lpcArg0;
    lpcRequest.lpcArgs[1] = lpcArg1;

    memset(requestShmaddr, 0x00, sizeof(requestShmaddr)); // 내용 초기화
    memcpy(requestShmaddr, &lpcRequest, sizeof(lpcRequest));

    pClientInfo[pidIndex].isRequested = 1;

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, responseShmaddr, sizeof(LpcResponse));

    shmdt(requestShmaddr);
    shmdt(responseShmaddr);

    sem_close(resSem);
    sem_unlink(responseSem);

    int a = atoi(lpcResponse.responseData);
    return a;
}

int ReadFile(int fd, void *pBuf, int size) {
    Init();

    LpcRequest lpcRequest;
    LpcArg lpcArg0, lpcArg1;

    memset(&lpcRequest, 0x00, sizeof(LpcRequest));

    lpcArg0.argSize = 4;
    lpcArg0.argData[0] = fd;
    sprintf(lpcArg0.argData, "%d", fd);
    lpcArg1.argSize = 4;
    lpcArg1.argData[0] = size;
    sprintf(lpcArg1.argData, "%d", size);

    lpcRequest.pid = getpid();
    lpcRequest.service = LPC_READ_FILE;
    lpcRequest.numArg = 2;
    lpcRequest.lpcArgs[0] = lpcArg0;
    lpcRequest.lpcArgs[1] = lpcArg1;

    memset(requestShmaddr, 0x00, sizeof(requestShmaddr)); // 내용 초기화

    memcpy(requestShmaddr, &lpcRequest, sizeof(lpcRequest));

    pClientInfo[pidIndex].isRequested = 1;

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, responseShmaddr, sizeof(LpcResponse));

    strcpy(pBuf, lpcResponse.responseData);

    shmdt(requestShmaddr);
    shmdt(responseShmaddr);

    sem_close(resSem);
    sem_unlink(responseSem);

    return lpcResponse.responseSize;
}

int WriteFile(int fd, void *pBuf, int size) {
    Init();

    LpcRequest lpcRequest;
    LpcArg lpcArg0, lpcArg1, lpcArg2;

    memset(&lpcRequest, 0x00, sizeof(LpcRequest));

    lpcArg0.argSize = 4;
    lpcArg0.argData[0] = fd;
    sprintf(lpcArg0.argData, "%d", fd);
    lpcArg1.argSize = sizeof(pBuf);
    strcpy(lpcArg1.argData, pBuf);
    lpcArg2.argSize = 4;
    lpcArg2.argData[0] = size;
    sprintf(lpcArg2.argData, "%d", size);

    lpcRequest.pid = getpid();
    lpcRequest.service = LPC_WRITE_FILE;
    lpcRequest.numArg = 3;
    lpcRequest.lpcArgs[0] = lpcArg0;
    lpcRequest.lpcArgs[1] = lpcArg1;
    lpcRequest.lpcArgs[2] = lpcArg2;

    memset(requestShmaddr, 0x00, sizeof(requestShmaddr)); // 내용 초기화
    memcpy(requestShmaddr, &lpcRequest, sizeof(lpcRequest));

    pClientInfo[pidIndex].isRequested = 1;

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, responseShmaddr, sizeof(LpcResponse));

    shmdt(requestShmaddr);
    shmdt(responseShmaddr);

    sem_close(resSem);
    sem_unlink(responseSem);

    return atoi(lpcResponse.responseData);
}

off_t SeekFile(int fd, off_t offset, int whence) {
    Init();

    LpcRequest lpcRequest;
    LpcArg lpcArg0, lpcArg1, lpcArg2;

    memset(&lpcRequest, 0x00, sizeof(lpcRequest));

    lpcArg0.argSize = 4;
    lpcArg0.argData[0] = fd;
    sprintf(lpcArg0.argData, "%d", fd);
    lpcArg1.argSize = sizeof(off_t);
    lpcArg1.argData[0] = offset;
    sprintf(lpcArg1.argData, "%ld", offset);
    lpcArg2.argSize = 4;
    lpcArg2.argData[0] = whence;
    sprintf(lpcArg2.argData, "%d", whence);

    lpcRequest.pid = getpid();
    lpcRequest.service = LPC_SEEK_FILE;
    lpcRequest.numArg = 3;
    lpcRequest.lpcArgs[0] = lpcArg0;
    lpcRequest.lpcArgs[1] = lpcArg1;
    lpcRequest.lpcArgs[2] = lpcArg2;

    memset(requestShmaddr, 0x00, sizeof(requestShmaddr)); // 내용 초기화
    memcpy(requestShmaddr, &lpcRequest, sizeof(lpcRequest));

    pClientInfo[pidIndex].isRequested = 1;

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, responseShmaddr, sizeof(LpcResponse));
    int a = atoi(lpcResponse.responseData);

    shmdt(requestShmaddr);
    shmdt(responseShmaddr);

    sem_close(resSem);
    sem_unlink(responseSem);

    return (off_t)a;
}

int CloseFile(int fd) {
    Init();

    LpcRequest lpcRequest;
    LpcArg lpcArg0;

    memset(&lpcRequest, 0x00, sizeof(lpcRequest));

    lpcArg0.argSize = 4;
    lpcArg0.argData[0] = fd;
    sprintf(lpcArg0.argData, "%d", fd);

    lpcRequest.pid = getpid();
    lpcRequest.service = LPC_CLOSE_FILE;
    lpcRequest.numArg = 1;
    lpcRequest.lpcArgs[0] = lpcArg0;

    memset(requestShmaddr, 0x00, sizeof(requestShmaddr)); // 내용 초기화
    memcpy(requestShmaddr, &lpcRequest, sizeof(lpcRequest));

    pClientInfo[pidIndex].isRequested = 1;

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, responseShmaddr, sizeof(LpcResponse));

    shmdt(requestShmaddr);
    shmdt(responseShmaddr);

    sem_close(resSem);
    sem_unlink(responseSem);

    return atoi(lpcResponse.responseData);
}

int MakeDirectory(char *path, int mode) {
    Init();

    LpcRequest lpcRequest;
    LpcArg lpcArg0, lpcArg1;

    memset(&lpcRequest, 0x00, sizeof(lpcRequest));

    lpcArg0.argSize = sizeof(path);
    strcpy(lpcArg0.argData, path);
    lpcArg1.argSize = 4;
    lpcArg1.argData[0] = mode;
    sprintf(lpcArg1.argData, "%d", mode);

    lpcRequest.pid = getpid();
    lpcRequest.service = LPC_MAKE_DIRECTORY;
    lpcRequest.numArg = 2;
    lpcRequest.lpcArgs[0] = lpcArg0;
    lpcRequest.lpcArgs[1] = lpcArg1;

    memset(requestShmaddr, 0x00, sizeof(requestShmaddr)); // 내용 초기화
    memcpy(requestShmaddr, &lpcRequest, sizeof(lpcRequest));

    pClientInfo[pidIndex].isRequested = 1;

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, responseShmaddr, sizeof(LpcResponse));

    shmdt(requestShmaddr);
    shmdt(responseShmaddr);

    sem_close(resSem);
    sem_unlink(responseSem);

    return atoi(lpcResponse.responseData);
}

int RemoveDirectory(char *path) {
    Init();

    LpcRequest lpcRequest;
    LpcArg lpcArg0;

    memset(&lpcRequest, 0x00, sizeof(lpcRequest));

    lpcArg0.argSize = sizeof(path);
    strcpy(lpcArg0.argData, path);

    lpcRequest.pid = getpid();
    lpcRequest.service = LPC_REMOVE_DIRECTORY;
    lpcRequest.numArg = 1;
    lpcRequest.lpcArgs[0] = lpcArg0;

    memset(requestShmaddr, 0x00, sizeof(requestShmaddr)); // 내용 초기화
    memcpy(requestShmaddr, &lpcRequest, sizeof(lpcRequest));

    pClientInfo[pidIndex].isRequested = 1;

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, responseShmaddr, sizeof(LpcResponse));

    shmdt(requestShmaddr);
    shmdt(responseShmaddr);

    sem_close(resSem);
    sem_unlink(responseSem);

    return atoi(lpcResponse.responseData);
}
