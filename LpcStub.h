#ifndef __LPC_STUB_H__

#define __LPC_STUB_H__

#include <sys/types.h>
#include <unistd.h>

#define CLIENT_NUM_MAX  (1024)

typedef struct __ClientInfo
{
    int pid;               
    int isRequested;
} ClientInfo; 

ClientInfo* pClientInfo;


extern void Init(void);
extern int OpenFile(char* path, int flag, int clientPid);
extern int ReadFile(int fd, int readCount, int clientPid);
extern int WriteFile(int fd, char* pBuf, int writeCount, int clientPid);
extern off_t SeekFile(int fd, off_t offset, int whence, int clientPid);
extern int CloseFile(int fd, int clientPid);
extern int MakeDirectory(char* path, int mode, int clientPid);
extern int RemoveDirectory(char* path, int clientPid);
#endif 
