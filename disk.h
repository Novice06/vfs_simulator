#pragma once
#include <stdio.h>
#include <stdint.h>

typedef struct disk
{
    char name[25];
    uint32_t totalSectors;
    FILE* stream;
}disk_t;

extern disk_t** disks;
extern int disk_num;

void disk_init();
void readSector(disk_t* disk, uint32_t lba, void* buffer);