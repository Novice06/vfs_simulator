#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "disk.h"

#define BYTE_PER_SECTOR 512

disk_t** disks = NULL;
int disk_num = 0;

bool has_img_extension(const char* str)
{
    char* ext = strchr(str, '.');
    return (strcmp(ext, ".img") == 0) ? true : false;
}

void readSector(disk_t* disk, uint32_t lba, void* buffer)
{
    if(lba > disk->totalSectors)
        return;

    fseek(disk->stream, lba * BYTE_PER_SECTOR, SEEK_SET);

    fread(buffer, sizeof(uint8_t), BYTE_PER_SECTOR, disk->stream);
}

void writeSector(disk_t* disk, uint32_t lba, const void* buffer)
{
    if(lba > disk->totalSectors)
        return;

    fseek(disk->stream, lba * BYTE_PER_SECTOR, SEEK_SET);

    fwrite(buffer, sizeof(uint8_t), BYTE_PER_SECTOR, disk->stream);
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
        
        
        disk_t* disk = malloc(sizeof(disk_t));
        
        if(disk == NULL)
        {
            perror("error while creating disk");
            exit(1);
        }

        char path[64] = "disks/\0";
        strncat(path, info->d_name, 64 - strlen(path));

        stat(path, &metainfo);

        strncpy(disk->name, info->d_name, 25);
        disk->totalSectors = metainfo.st_size / BYTE_PER_SECTOR;
        disk->stream = fopen(path, "rb+");

        if(disk->stream == NULL)
        {
            perror("error while reading disk");
            exit(1);
        }

        disk_num++;
        disks = realloc(disks, sizeof(disks) * disk_num);
        disks[disk_num - 1] = disk;
    }

    closedir(directory);
}