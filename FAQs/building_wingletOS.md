# AV_Badge_2024
Aerospace Village Badge 2024

## Building Winglet OS

**NOTE: You MUST have a Linux machine (Tested on Ubuntu 22.04 and 24.04) in order to compile this, and preferably one with a good CPU and a decent amount of RAM.**

This repo is designed to allow you to easily build your own copy of winglet-os and develop on top of it. To start building, you just need to clone this repo and run the following commands:

	sudo apt install git build-essential libncurses-dev libssl-dev python3-pip file wget cpio unzip rsync bc xxd
	pip3 install setuptools
	git submodule update --init
	cd software/winglet-os
	make

The commands above will take a while to run (on my machine it takes around an hour and a half), so get that started now and then keep you can keep reading while it builds.

## GUI Development

If you want to modify winglet-gui, or develop custom Qt applications, there is a simpler way to test your changes rather than issuing a full buildroot build. Qt Creator supports deploying directly
to a Linux machine. See the AV Badge Development Tutorial directory for more information on setting this up.

## Repository Structure

Winglet OS is based on [Buildroot](https://www.buildroot.org/), a tool to automate the creation of embedded linux images. If you intend to modify the core OS in any way, it is recommended
that you familiarize yourself with buildroot concepts, as it is the primary tool used during build.

The buildroot is core is located inside of the `winglet-os` directory. Buildroot is added as a submodule (following the same structure as the [buildroot-submodule](https://github.com/Openwide-Ingenierie/buildroot-submodule)
repo. This means that all buildroot commands should be ran inside the root `winglet-os` directory, including menuconfig and requesting rebuilds of certain packages.

There are three other software repositories used in winglet OS, these are:
 * `winglet-kernel`: Fork of the Linux kernel with custom modifications required for the badge
 * `winglet-boot`: Fork of U-Boot with custom modifications required for the badge
 * `winglet-gui`: The Qt5 Project which holds all of the UI for the badge

If you want to rapidly develop the kernel/bootloader without fully cleaning buildroot, the kernel and U-boot repos can be rebuilt using `make linux-rebuild all` and `make uboot-rebuild all` respectively.

## Winglet OS Output Format

When the build completes, you will find the build artifacts inside `winglet-os/winglet/output/images`. The important files are `winglet-os.img.gz` which is the image that can be flashed to the badge's
eMMC or, if you flash u-boot as well, an external SD card.

Additionally, there is `u-boot-sunxi-with-spl.bin` which is the final u-boot image. By default the badge has this installed in a write-protected boot partition of eMMC. However, if you want to test
a custom u-boot version out, you can load it with [sunxi-tools](https://github.com/linux-sunxi/sunxi-tools) and boot over USB FEL mode. This is recommended if you're testing u-boot changes.
Note if you're messing around with u-boot, it is strongly recommended that you add UART support to your badge. Additionally, if you are attempting to flash winglet-os to an SD card to dual boot
your badge, u-boot must be placed at a 128KB offset into the SD card so that the bootrom can find it. You can find more info about the boot process [here](https://doc.funkey-project.com/developer_guide/software_reference/boot_process/boot_rom/).

