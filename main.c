/*
 * MIT License
 *
 * Copyright (c) 2025 Novice
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include <stdio.h>

#include "device.h"
#include "ramfs.h"
#include "fat12.h"
#include "disk.h"
#include "vfs.h"

int main()
{
    vfs_init();
    //ramfs_init();
    fat12_init();
    disk_init();

    uint8_t buffer[50];

    printf("device number %d\n\n", device_num);

    printf("mounting %s to /\n", device_list[0]->name);
    if(vfs_mount("fat12", "/", 0) != VFS_OK)
        printf("error while mounting %s at /!\n", device_list[0]->name);

    int fd1 = vfs_open("/mydir/dir_msg.txt", VFS_O_RDWR);
    printf("opening /mydir/dir_msg.txt\n");

    printf("descriptor: %d\n", fd1);

    size_t read = vfs_read(fd1, buffer, 49);

    printf("byte read: %ld\n", read);
    buffer[read] = '\0';

    printf("content: %s\n", buffer);

    vfs_close(fd1);


    
    return 0;
}