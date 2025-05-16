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
            continue;
        
        
        disk_info_t* disk = malloc(sizeof(disk_info_t));
        
        if(disk == NULL)
        {
            perror("error while creating disk");
            exit(1);
        }

        char path[64] = "disks/";
        strcat(path, info->d_name);

        stat(path, &metainfo);

        disk->totalSectors = metainfo.st_size / BYTE_PER_SECTOR;
        disk->stream = fopen(path, "rb+");

        if(disk->stream == NULL)
        {
            perror("error while reading disk");
            exit(1);
        }

        device_t* new_device = malloc(sizeof(device_t));
        strcpy(new_device->name, info->d_name);
        new_device->priv = disk;
        new_device->read = readSectors;
        new_device->write = writeSectors;

        add_device(new_device);
    }

    closedir(directory);
}