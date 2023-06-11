// SPDX-License-Identifier: GPL-2.0-or-later

// Driver of AHT21, the humidity and temperature sensor

// Copyright (c) wejczhu <jx.wei@outlook.com>

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define DEVICE_COUNT 1
#define DEVICE_NAME "aht21"

struct aht21_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    void *private_data;
    int humidity;
    unsigned int temperature;
};

static struct aht21_dev aht21dev;

static bool check_status(struct aht21_dev* dev) 
{
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    uint8_t send_buffer[10];
    uint8_t receive_buffer[10];

    send_buffer[0] = 0x71;

    int send_length = i2c_master_send(client, send_buffer, 1);

    int receive_length = i2c_master_recv(client, receive_buffer, 1);

    if((receive_buffer[0]&0x18)!=0x18)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void calibrate(struct aht21_dev* dev)
{
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    uint8_t send_buffer[10];
    uint8_t receive_buffer[10];

    send_buffer[0] = 0x70;
    send_buffer[1] = 0xbe;
    send_buffer[2] = 0x08;
    send_buffer[3] = 0x00;

    int send_length = i2c_master_send(client, send_buffer, 4);

    mdelay(100);
}

static void start_measurement(struct aht21_dev* dev)
{
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    uint8_t send_buffer[10];
    uint8_t receive_buffer[10];
    uint8_t data[10] = {0};

    send_buffer[0] = 0x70;
    send_buffer[1] = 0xac;
    send_buffer[2] = 0x33;
    send_buffer[3] = 0x00;
    send_buffer[4] = 0x71;

    int ret = i2c_master_send(client, send_buffer, 4);

    mdelay(100);

    ret = i2c_master_send(client, &send_buffer[4], 1);

    ret = i2c_master_recv(client, receive_buffer, 1);


    ret = i2c_master_recv(client, data, 6);

    // check if measurement is done
    unsigned int humidity = 0;
    humidity = (humidity|data[1])<<8;
    humidity = (humidity|data[2])<<8;
    humidity = (humidity|data[3]);
    humidity = humidity >>4;

    unsigned int temperature = 0;
    temperature = (temperature|data[3])<<8;
    temperature = (temperature|data[4])<<8;
    temperature = (temperature|data[5]);
    temperature = temperature&0xfffff;

    humidity = humidity * 100 * 10 / 1024 / 1024;
    temperature = temperature * 200 * 10 / 1024 / 1024 - 500;

    dev->temperature = temperature;
    dev->humidity = humidity;
}

static int aht21_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &aht21dev;
    if(check_status(&aht21dev))
    {
        calibrate(&aht21dev);
    }

    return 0;
}

static ssize_t aht21_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    unsigned int data[2];
    long err = 0;

    struct aht21_dev *dev = (struct aht21_dev *)filp->private_data;

    start_measurement(dev);

    data[0] = dev->temperature;
    data[1] = dev->humidity;
    err = copy_to_user(buf, data, sizeof(data));
    return 0;
}

static int aht21_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static int aht21_remove(struct i2c_client *client)
{
    cdev_del(&aht21dev.cdev);
    unregister_chrdev_region(aht21dev.devid, DEVICE_COUNT);

    device_destroy(aht21dev.class, aht21dev.devid);
    class_destroy(aht21dev.class);
    return 0;
}

static const struct file_operations aht21_ops = {
    .owner = THIS_MODULE,
    .open = aht21_open,
    .read = aht21_read,
    .release = aht21_release,
};

static const struct i2c_device_id aht21_id[] = {
    {"aht21", 0},  
    {}
};

static const struct of_device_id aht21_of_match[] = {
    { .compatible = "aht21" },
    { /* Sentinel */ }
};

static int aht21_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    if (aht21dev.major) {
        aht21dev.devid = MKDEV(aht21dev.major, 0);
        register_chrdev_region(aht21dev.devid, DEVICE_COUNT, DEVICE_NAME);
    } else {
        alloc_chrdev_region(&aht21dev.devid, 0, DEVICE_COUNT, DEVICE_NAME);
        aht21dev.major = MAJOR(aht21dev.devid);
    }

    cdev_init(&aht21dev.cdev, &aht21_ops);
    cdev_add(&aht21dev.cdev, aht21dev.devid, DEVICE_COUNT);

    aht21dev.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(aht21dev.class)) {
        return PTR_ERR(aht21dev.class);
    }

    aht21dev.device = device_create(aht21dev.class, NULL, aht21dev.devid, NULL, DEVICE_NAME);
    if (IS_ERR(aht21dev.device)) {
        return PTR_ERR(aht21dev.device);
    }

    aht21dev.private_data = client;
    return 0;
}

static struct i2c_driver aht21_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "aht21",
        .of_match_table = aht21_of_match, 
    },
    .probe = aht21_probe,
    .remove = aht21_remove,
    .id_table = aht21_id,
};

module_i2c_driver(aht21_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("wejczhu <jx.wei@outlook.com>");
MODULE_DESCRIPTION("AHT21, the humidity and temperature sensor");
MODULE_VERSION("0.1");
