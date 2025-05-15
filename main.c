#include <stdio.h>

#include "device.h"
#include "ramfs.h"
#include "vfs.h"

int main()
{
    vfs_init();
    ramfs_init();

    uint8_t buffer[15];

    if(vfs_mount("ramfs", "/", 0) == VFS_ERROR)
        printf("error !\n");
    
    int fd = vfs_open("/doc/hello.txt", VFS_O_RDWR);
    printf("byte read: %ld\n", vfs_read(fd, buffer, 13));
    buffer[13] = '\0';

    printf("content: %s\n", buffer);

    return 0;
}