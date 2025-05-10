#include <stdio.h>
#include "disk.h"


int main()
{
    uint8_t sector[512];
    disk_init();

    for(int i = 0; i < disk_num; i++)
        printf("disk name: %s\n", disks[i]->name);

    readSector(disks[0], 1, sector);
    for (int i = 0; i < 512; i++)
        printf("0x%x ", sector[i]);
    
    printf("\n");
    return 0;
}