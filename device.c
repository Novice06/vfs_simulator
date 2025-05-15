#include <stdlib.h>

#include "device.h"

device_t **device_list;
int device_num;

void add_device(device_t* device)
{
    device_num++;
    device_list = realloc(device_list, sizeof(device_list) * device_num);
    device_list[device_num - 1] = device;
}