global pa_to_highhalfkernel

%define KERNELBASE 0xFFFFFFFF80000000

section .text
bits 64

pa_to_highhalfkernel:
  mov r15, KERNELBASE
  add qword [rsp], r15 ;add return address with kernelbase
  add rsp, r15 
  add rbp, r15
  ret ;pop return address 

