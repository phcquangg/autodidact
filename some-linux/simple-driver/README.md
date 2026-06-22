 
First compile the code
```
make
```
Then insert the module with custom parameter 
```
sudo insmod simple-driver.ko buffer_size=512
```

Check dmesg for the confirmation, also the random Major Number assigned
```
dmesg | tail -n 5

simple-driver: Initializing device with a buffer size of 512 bytes
simple-driver: Registered successfully with Major Number 236
```

Expose the Device File.

Linux need a node in ```/dev/``` to link the Major Number to a file
```
sudo mknod /dev/simple-driver c [major_number] 0
```

Grant read/write permission
```
sudo chmod 666 /dev/simple-driver
```

Try to write something
```
echo "Hola amigo, que pasa" > /dev/simple-driver
```

And check for it!
```
cat /dev/simple-driver
```

Everything goood? :D Cleanup time.
```
sudo rmmod simple-driver
sudo rm /dev/simple-driver
make clean
```

And that, gentlemen, is my practice on custom parameter and device character 