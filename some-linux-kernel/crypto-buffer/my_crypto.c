/* 
Crypto-buffer Driver

Description:
	When driver loaded, allocate 10Mb of RAM
	When user write data into /dev/my_crypto, encrypt them using XOR pattern (later update to AES)
	and decrypt them when user read

For learning
	Building Linux module
	Kernel memory management
	XOR / AES encryption
	...
	
*/



