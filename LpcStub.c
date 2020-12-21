#include "LpcStub.h"
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

int pidIndex;

void Init(void)
{
    char serverSeg[15] = {
        '\0',
    };
    char clientSem[15] = {
        '\0',
    };

    sprintf(serverSeg, "%s%d", RES_SEG, pidIndex);
    sprintf(clientSem, "%s%d", REQ_SEM, pidIndex);
    printf("serverSeg : %s\n", serverSeg);
    responseKey = __makeKeyByName(serverSeg);

    responseShmid = shmget(responseKey, MAX_SHM_SIZE, IPC_CREAT | 0666);
    printf("get good\n");
    // shmctl(responseShmid, IPC_RMID, NULL);
    responseShmaddr = shmat(responseShmid, NULL, 0);

    // reqSem = sem_open(REQ_SEM, O_CREAT, 0644, 0);
    // resSem = sem_open(clientSem, 0, 0644, 1);
}

int OpenFile(char *path, int flag, int clientPid) {
    pidIndex = clientPid % 1000;
    Init();
    printf("init good");

    int fd = open(path, flag, 0666);    // open

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    lpcResponse.pid = clientPid;
    lpcResponse.errorno = 0;
    lpcResponse.responseSize = 4;
    lpcResponse.responseData[0] = fd;
    sprintf(lpcResponse.responseData, "%d", fd);

    memset(responseShmaddr, 0x00, MAX_SHM_SIZE);
    printf("memset good\n");
    memcpy(responseShmaddr, &lpcResponse, sizeof(lpcResponse));
    printf("memcpy good\n");

    return fd;
}

int ReadFile(int fd, int readCount, int clientPid) {
    pidIndex = clientPid % 1000;
    Init();

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    char buf[LPC_DATA_MAX + 1];
    memset(buf, 0x00, LPC_DATA_MAX + 1);

    int rsize = read(fd, buf, readCount);

    lpcResponse.pid = clientPid;
    lpcResponse.errorno = 0;
    lpcResponse.responseSize = rsize;
    strcpy(lpcResponse.responseData, buf);

    memset(responseShmaddr, 0x00, MAX_SHM_SIZE);
    memcpy(responseShmaddr, &lpcResponse, sizeof(lpcResponse));

    return rsize;
}

int WriteFile(int fd, char *pBuf, int writeCount, int clientPid) {
    pidIndex = clientPid % 1000;
    Init();

    int wsize = write(fd, pBuf, writeCount);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    lpcResponse.pid = clientPid;
    lpcResponse.errorno = 0;
    lpcResponse.responseSize = 4;
    lpcResponse.responseData[0] = wsize;
    sprintf(lpcResponse.responseData, "%d", wsize);

    memset(responseShmaddr, 0x00, MAX_SHM_SIZE);
    memcpy(responseShmaddr, &lpcResponse, sizeof(lpcResponse));

    return wsize;
}

off_t SeekFile(int fd, off_t offset, int whence, int clientPid) {
    pidIndex = clientPid % 1000;
    Init();

    off_t offs = lseek(fd, (off_t)offset, whence);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    lpcResponse.pid = clientPid;
    lpcResponse.errorno = 0;
    lpcResponse.responseSize = 8;
    lpcResponse.responseData[0] = offs;
    sprintf(lpcResponse.responseData, "%ld", offs);

    memset(responseShmaddr, 0x00, MAX_SHM_SIZE);
    memcpy(responseShmaddr, &lpcResponse, sizeof(lpcResponse));

    return offs;
}

int CloseFile(int fd, int clientPid) {
    pidIndex = clientPid % 1000;
    Init();

    int k = close(fd);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    lpcResponse.pid = clientPid;
    lpcResponse.errorno = 0;
    lpcResponse.responseSize = 4;
    lpcResponse.responseData[0] = k;
    sprintf(lpcResponse.responseData, "%d", k);

    memset(responseShmaddr, 0x00, MAX_SHM_SIZE);
    memcpy(responseShmaddr, &lpcResponse, sizeof(lpcResponse));

    return k;
}

int MakeDirectory(char *path, int mode, int clientPid) {
    pidIndex = clientPid % 1000;
    Init();

    int k = mkdir(path, mode);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    lpcResponse.pid = clientPid;
    lpcResponse.errorno = 0;
    lpcResponse.responseSize = 4;
    lpcResponse.responseData[0] = k;
    sprintf(lpcResponse.responseData, "%d", k);

    memset(responseShmaddr, 0x00, MAX_SHM_SIZE);
    memcpy(responseShmaddr, &lpcResponse, sizeof(lpcResponse));

    return k;
}

int RemoveDirectory(char *path, int clientPid) {
    pidIndex = clientPid % 1000;
    Init();

    int k = rmdir(path);

    LpcResponse lpcResponse;
    memset(&lpcResponse, 0x00, sizeof(LpcResponse));

    lpcResponse.pid = clientPid;
    lpcResponse.errorno = 0;
    lpcResponse.responseSize = 4;
    lpcResponse.responseData[0] = k;
    sprintf(lpcResponse.responseData, "%d", k);

    memset(responseShmaddr, 0x00, MAX_SHM_SIZE);
    memcpy(responseShmaddr, &lpcResponse, sizeof(lpcResponse));

    return k;
}
