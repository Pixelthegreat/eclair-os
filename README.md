# eclair-os
Éclair OS (Not to be confused with Android Eclair)

![screenshot](res/screenshot.png)

[_more screenshots_](res/screenshots)

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

## Build Requirements
There are a few requirements for the build to work. Here they are:

 - A UNIX-like environment (Linux, BSD, etc)
 - Python 3
 - Make
 - NASM (Netwide Assembler)
 - Host GCC (GCC that runs on your system and compiles for your system, available with most package managers as just `gcc`)

In addition to a host GCC suite for some of the host system tools, a cross-compilation suite is also required to build the OS itself. In the future, there will be a script probably named `scripts/toolchain.sh` that will automatically build and install this cross-compilation suite. However, in the mean time please refer to [this osdev wiki article](https://wiki.osdev.org/GCC_Cross-Compiler). As an extremely important note, the _target triple_ for Eclair OS is `i686-elf`.

## Build and Run

Before building, run `make setup_init` to do basic preparation tasks.

To build the project, just run `make` as you normally would. Run `make run` to run the project in qemu.
