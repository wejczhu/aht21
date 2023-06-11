## Linux driver of AHT21
The linux deriver of Aosong AHT21 humidity and temperature sensor.

### 1. Device tree
将aht21.dti中的内容拷贝至目标设备的设备树文件中，编译设备树，更新设备树文件至目标嵌入式设备。
```
e.g 针对imx6ull开发板，在文件imx6ull-14x14-evk.dts中，添加到i2c1节点下

&i2c1 {
	clock-frequency = <100000>;
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_i2c1>;
	status = "okay";

	aht21@38 {
		compatible = "alientek,aht21";
		reg = <0x38>;
	};
};

```
### 2. Build
1. 将Makefile中的`KERNELDIR`改为你目标内核的目录
    ```
    e.g. KERNELDIR := /home/xxx/workspace/kenel/linux-imx-4.1.15-2.1.0-g06f53e4-v2.1
    ```
2. 使用make命令进行编译，指定CROSS_COMPILER为你使用的交叉编译器
    ```
    e.g. make CROSS_COMPILER=arm-linux-gnueabihf
    ```
    最终得到aht21.ko文件
### 3. Install
使用insmod命令安装驱动，并通过lsmod查看驱动已经被加载
```
e.g. insmod aht21.ko
```

### Usage
参考app目录下aht21_test.c
