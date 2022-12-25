.section .text

.global _start
_start:
 	xor %rbp, %rbp
	call server_main