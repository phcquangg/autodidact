```
sudo apt-get install gcc-arm-linux-gnueabi

arm-linux-gnueabi-as hello.s -o hello.o

arm-linux-gnuebi-gcc hello.o -o hello -nostdlib

./hello && echo $?
```
