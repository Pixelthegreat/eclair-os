# eclair-os
Éclair OS (Not to be confused with Android Eclair)

![screenshot](res/screenshot.png)

Éclair OS is a hobby unix-like OS project. It is currently very early in its development, and as such, it is not yet capable of a lot. My primary inspiration comes from other hobby projects like [ToaruOS](https://github.com/klange/toaruos) and QuafiOS (which mysteriously disappeared some time ago), as well as educational resources like the [xv6 project](https://github.com/mit-pdos/xv6-public).

I am considering Éclair OS to be in the first stage of development. This is the stage where most (if not all) development happens within the kernel. By the end of this stage, I am hoping to have basic userspace support completed (i.e. multitasking, static executable loading, rudimentary syscall support, etc). Here is a roadmap that I have come up with for said stage:

- Boot
- Memory Management
- Device Manager
- Device Drivers
- Virtual File System
- File System Drivers
- Process Scheduling and Management
- Static Executable Loading

Although this order generally follows how these things should be implemented, they aren't necessarily the exact order that everything will be _completed_ in. Note as well that this list only includes things that are important to the primary goal of this stage, which as I mentioned was basic userspace support. However, I most certainly will work on various miscellaneous things inbetween. If you want to know where exactly the project is, you can take a look at the commit messages, which will give a general idea. I also (try to) update the screenshot every commit to reflect what changes were made.

## build and run
This project assumes that you are using a Unix-like environment (i.e. Linux, BSD, Mac OS) and you have a generic x86 ELF (i686-elf) gcc cross compiler installed. If you're system uses ELF format executables and is an 32-bit x86 machine, you may be able to use your system's gcc installation. However, it is not recommended. Nevertheless, if you have a cross compiler (or would like to use your system compiler anyway), edit the `TARGET` variable in `config.mk` with the associated target triple.

Before building, you must run the `genbootdisk.sh` script in order to generate the template bootdisk. You must also run `make setup` to create any necessary build files.

To build the project, just run `make` as you normally would. Run `make run` to run the project in qemu.
