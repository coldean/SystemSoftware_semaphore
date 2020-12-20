#include "Lpc.h"
#include "LpcProxy.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <fcntl.h>

int main(void) {
    int fd = OpenFile("one.txt", O_CREAT | O_RDWR);
    char pBuf[30] = "hello";
    printf("%d", getpid());
    WriteFile(fd, &pBuf, sizeof(pBuf));


    return 0;
}
