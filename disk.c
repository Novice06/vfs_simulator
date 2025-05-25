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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "disk.h"
#include "device.h"

#define BYTE_PER_SECTOR 512

bool has_img_extension(const char* str)
{
    char* ext = strchr(str, '.');
    return (strcmp(ext, ".img") == 0) ? true : false;
}

/**
 * Writes data to a virtual disk starting at the given logical block address (LBA).
 *
 * @param buffer      Pointer to the data buffer to write.
 * @param lba         Logical block address where writing should begin.
 * @param sector_num  Number of sectors to write.
 * @param priv        Pointer to the disk structure (cast from void*). This provides access to
 *                    the specific disk instance to operate on.
 *
 * Note:
 * The `priv` pointer is expected to reference a valid `disk` structure. While it's possible
 * to pass a device ID and resolve the disk through the device list, this design choice 
 * simplifies callback-based access by passing the disk reference directly.
 */
void writeSectors(const uint8_t* buffer, uint32_t lba, uint32_t sector_num, void* priv)
{
    if(priv == NULL)
        return;

    disk_info_t* disk = (disk_info_t*)priv;

    if(lba > disk->totalSectors || (lba + sector_num) > disk->totalSectors)
        return;

    fseek(disk->stream, lba * BYTE_PER_SECTOR, SEEK_SET);

    fwrite(buffer, sizeof(uint8_t), BYTE_PER_SECTOR * sector_num, disk->stream);
}

void readSectors(uint8_t* buffer, uint32_t lba, uint32_t sector_num, void* priv)
{
    if(priv == NULL)
        return;

    disk_info_t* disk = (disk_info_t*)priv;

    if(lba > disk->totalSectors || (lba + sector_num) > disk->totalSectors)
        return;

    fseek(disk->stream, lba * BYTE_PER_SECTOR, SEEK_SET);

    fread(buffer, sizeof(uint8_t), BYTE_PER_SECTOR * sector_num, disk->stream);
}

/**
 * Initializes all available virtual disks.
 *
 * This function scans the "disks" directory for all files with the `.img` extension.
 * Each `.img` file found is treated as a potential disk image. For each valid image,
 * a corresponding disk structure is created and added to the device list.
 *
 * This setup simulates a simple virtual device discovery mechanism, useful for 
 * emulating hardware-like behavior in an OS development environment.
 */
void disk_init()
{
    DIR* directory = NULL;
    struct dirent* info = NULL;
    struct stat metainfo;
    

    directory = opendir("disks");
    if(directory == NULL)
    {
        perror("error while opening the disks directory");
        exit(1);
    }

    while((info = readdir(directory)) != NULL)
    {
        if(info->d_type != DT_REG || !has_img_extension(info->d_name))
            continue;   // nothing to do it's not a .img file
        
        
        // a .img file was found !
        disk_info_t* disk = malloc(sizeof(disk_info_t));
        
        if(disk == NULL)
        {
            perror("error while creating disk");
            exit(1);
        }

        // because stat needs the full relative path
        char path[64] = "disks/";
        strcat(path, info->d_name);

        stat(path, &metainfo);

        disk->totalSectors = metainfo.st_size / BYTE_PER_SECTOR;
        disk->stream = fopen(path, "rb+");  // now our data stream

        if(disk->stream == NULL)
        {
            perror("error while reading disk\n");
            free(disk);
            exit(1);
        }

        device_t* new_device = malloc(sizeof(device_t));
        if(new_device == NULL)
        {
            perror("error while creating a device\n");
            free(disk);
            exit(1);
        }

        new_device->priv = disk;    // yeah we store the disk structure here !
        strcpy(new_device->name, info->d_name);
        new_device->read = readSectors;
        new_device->write = writeSectors;

        add_device(new_device);
    }

    closedir(directory);
}