#include<iostream>
#include<cuda_runtime.h>

__global__ void helloWorld() {
  printf("Hello World, from GPU thread %d!\n", threadIdx.x);
}

int main() {
  printf("Hello World, from CPU!\n");
  helloWorld<<<1, 5>>>();

  cudaDeviceSynchronize();
  return 0;
}