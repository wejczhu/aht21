// SPDX-License-Identifier: GPL-2.0-or-later

// Test app of AHT21, the humidity and temperature sensor

// Copyright (c) wejczhu <jx.wei@outlook.com>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define AHT21_DEVICE "/dev/aht21"

int main()
{
    unsigned int dataBuf[2];
    int fd = open(AHT21_DEVICE, O_RDWR);
    if(fd < 0)
    {
        perror("Open device");
        return -1;
    }

    read(fd, dataBuf, sizeof(dataBuf));
    float temperature = (float)dataBuf[0] / (float)10;
    float humidity = (float)dataBuf[1] / (float)10;

    printf("Temperature: %.2f", temperature);
    printf("Humidity: %.2f", humidity);

    int ret = close(fd);
    if(ret < 0)
    {
        perror("Close device");
    }

    return 0;
}