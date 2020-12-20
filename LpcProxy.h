#ifndef __LPC_PROXY_H__

#define __LPC_PROXY_H__

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
extern int OpenFile(char* path, int flags);
extern int ReadFile(int fd, void* pBuf, int size);
extern int WriteFile(int fd, void* pBuf, int size);
extern int CloseFile(int fd);
extern off_t SeekFile(int fd, off_t offset, int whence);
extern int MakeDirectory(char* path, int mode);
extern int RemoveDirectory(char* path);

#endif 
