# OS Experiment Standard_1

This folder contains simple Linux/Ubuntu-only C programs for experiments 1 to 10.

## Files

- `standard_1.c`: fork a child process and read a file
- `standard_2.c`: pthread shared data
- `standard_3.c`: parent-child signal communication
- `standard_4.c`: anonymous pipe communication
- `standard_5.c`: named pipe FIFO communication
- `standard_6.c`: producer-consumer with semaphores
- `standard_7.c`: POSIX shared memory communication
- `standard_8.c`: simple file system simulator
- `standard_9.c`: simple PCB information from `/proc`
- `standard_10.c`: simple Linux kernel module

## Compile Examples

```bash
gcc standard_1.c -o standard_1
gcc standard_2.c -o standard_2 -pthread
gcc standard_3.c -o standard_3
gcc standard_4.c -o standard_4
gcc standard_5.c -o standard_5
gcc standard_6.c -o standard_6 -pthread
gcc standard_7.c -o standard_7
gcc standard_8.c -o standard_8
gcc standard_9.c -o standard_9
```

For `standard_10.c`, create a Makefile in the same folder:

```makefile
obj-m += standard_10.o
```

Then build and run it on Linux:

```bash
make -C /lib/modules/$(uname -r)/build M=$PWD modules
sudo insmod standard_10.ko message=hello repeat=3
dmesg | tail
sudo rmmod standard_10
```
