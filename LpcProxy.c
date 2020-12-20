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

key_t clientKey;
key_t serverKey;
key_t ci;
int clientShmid;
int *clientShmaddr;
int serverShmid;
int *serverShmaddr;
int ciid;
int *ciaddr;
sem_t *reqSem, *resSem;

int pidIndex;

void Init(void) {
    char serverSeg[15] = {
        '\0',
    };
    char clientSeg[15] = {
        '\0',
    };
    char clientSem[15] = {
        '\0',
    };
    int pid = getpid();
    pidIndex = pid % 1000;

    sprintf(serverSeg, "%s%d", RES_SEG, pidIndex);
    sprintf(clientSeg, "%s%d", REQ_SEG, pidIndex);
    sprintf(clientSem, "%s%d", REQ_SEM, pidIndex);

    printf("%s\n", clientSeg);
    clientKey = __makeKeyByName(clientSeg);
    serverKey = __makeKeyByName(serverSeg);
    ci = ftok("ci_set", 0);

    clientShmid = shmget(clientKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
    clientShmaddr = shmat(clientShmid, NULL, 0);
    serverShmid = shmget(serverKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
    serverShmaddr = shmat(serverShmid, NULL, 0);

    ciid = shmget(ci, CLIENT_NUM_MAX, IPC_CREAT | 0666);
    ciaddr = shmat(ciid, NULL, 0);

    reqSem = sem_open(REQ_SEM, 0, 0644, 0);
    resSem = sem_open(clientSem, O_CREAT, 0644, 0);
}

int OpenFile(char *path, int flags) {
    Init();

    long cpid = getpid();

    LpcRequest lpcRequest;
    LpcArg lpcArg0, lpcArg1;

    memset(&lpcRequest, 0x00, sizeof(lpcRequest));

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

    memset(clientShmaddr, 0x00, sizeof(clientShmaddr)); // 내용 초기화
    memcpy(clientShmaddr, &lpcRequest, sizeof(lpcRequest));

    ClientInfo *cli = malloc(sizeof(ClientInfo));
    cli->pid = getpid();
    cli->isRequested = 1;

    memcpy(ciaddr + 8 * pidIndex, cli, sizeof(ClientInfo));

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, serverShmaddr, sizeof(LpcResponse));

    int a = atoi(lpcResponse.responseData);
    return a;
}

int ReadFile(int fd, void *pBuf, int size) {
    Init();

    LpcRequest lpcRequest;
    LpcArg lpcArg0, lpcArg1;

    memset(&lpcRequest, 0x00, sizeof(lpcRequest));

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

    memset(clientShmaddr, 0x00, sizeof(clientShmaddr)); // 내용 초기화
    memcpy(clientShmaddr, &lpcRequest, sizeof(lpcRequest));

    ClientInfo *cli = malloc(sizeof(ClientInfo));
    cli->pid = getpid();
    cli->isRequested = 1;

    memcpy(ciaddr + 8 * pidIndex, cli, sizeof(ClientInfo));

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, serverShmaddr, sizeof(LpcResponse));

    strcpy(pBuf, lpcResponse.responseData);

    return lpcResponse.responseSize;
}

int WriteFile(int fd, void *pBuf, int size) {
    Init();

    LpcRequest lpcRequest;
    LpcArg lpcArg0, lpcArg1, lpcArg2;

    memset(&lpcRequest, 0x00, sizeof(lpcRequest));

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

    memset(clientShmaddr, 0x00, sizeof(clientShmaddr)); // 내용 초기화
    memcpy(clientShmaddr, &lpcRequest, sizeof(lpcRequest));

    ClientInfo *cli = malloc(sizeof(ClientInfo));
    cli->pid = getpid();
    cli->isRequested = 1;

    memcpy(ciaddr + 8 * pidIndex, cli, sizeof(ClientInfo));

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, serverShmaddr, sizeof(LpcResponse));
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

    memset(clientShmaddr, 0x00, sizeof(clientShmaddr)); // 내용 초기화
    memcpy(clientShmaddr, &lpcRequest, sizeof(lpcRequest));

    ClientInfo *cli = malloc(sizeof(ClientInfo));
    cli->pid = getpid();
    cli->isRequested = 1;

    memcpy(ciaddr + 8 * pidIndex, cli, sizeof(ClientInfo));

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, serverShmaddr, sizeof(LpcResponse));
    int a = atoi(lpcResponse.responseData);
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

    memset(clientShmaddr, 0x00, sizeof(clientShmaddr)); // 내용 초기화
    memcpy(clientShmaddr, &lpcRequest, sizeof(lpcRequest));

    ClientInfo *cli = malloc(sizeof(ClientInfo));
    cli->pid = getpid();
    cli->isRequested = 1;

    memcpy(ciaddr + 8 * pidIndex, cli, sizeof(ClientInfo));

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, serverShmaddr, sizeof(LpcResponse));

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

    memset(clientShmaddr, 0x00, sizeof(clientShmaddr)); // 내용 초기화
    memcpy(clientShmaddr, &lpcRequest, sizeof(lpcRequest));

    ClientInfo *cli = malloc(sizeof(ClientInfo));
    cli->pid = getpid();
    cli->isRequested = 1;

    memcpy(ciaddr + 8 * pidIndex, cli, sizeof(ClientInfo));

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, serverShmaddr, sizeof(LpcResponse));
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

    memset(clientShmaddr, 0x00, sizeof(clientShmaddr)); // 내용 초기화
    memcpy(clientShmaddr, &lpcRequest, sizeof(lpcRequest));

    ClientInfo *cli = malloc(sizeof(ClientInfo));
    cli->pid = getpid();
    cli->isRequested = 1;

    memcpy(ciaddr + 8 * pidIndex, cli, sizeof(ClientInfo));

    sem_post(reqSem);

    sem_wait(resSem);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    memcpy(&lpcResponse, serverShmaddr, sizeof(LpcResponse));

    return atoi(lpcResponse.responseData);
}
