# Target Program
Here you can find the source code of a vulnerable program.

To find the address of an instruction to probe, you can use gdb as the compilation doesn't strip the symbols. The following example shows how to find an instruction address to probe the function `input_A`:

```
$ gdb ./abcd

(gdb) disas input_A
Dump of assembler code for function input_A:
   0x0000000000002000 <+0>:	push   %rbp
   0x0000000000002001 <+1>:	mov    %rsp,%rbp
   0x0000000000002004 <+4>:	sub    $0x10,%rsp
   0x0000000000002008 <+8>:	movl   $0x3,-0x4(%rbp)
   0x000000000000200f <+15>:	movl   $0x4,-0xc(%rbp)
   0x0000000000002016 <+22>:	mov    -0xc(%rbp),%eax
   0x0000000000002019 <+25>:	add    %eax,-0x4(%rbp)
   .
   .
   .
   0x000000000000217c <+380>:	imul   -0xc(%rbp),%eax
   0x0000000000002180 <+384>:	mov    %eax,-0x4(%rbp)
   0x0000000000002183 <+387>:	addl   $0x1,-0x8(%rbp)
   0x0000000000002187 <+391>:	cmpl   $0x752f,-0x8(%rbp)
   0x000000000000218e <+398>:	jle    0x215f <input_A+351>
   0x0000000000002190 <+400>:	mov    -0x4(%rbp),%eax
   0x0000000000002193 <+403>:	imul   -0xc(%rbp),%eax
   0x0000000000002197 <+407>:	mov    %eax,-0x4(%rbp)
   .
   .
   .
   0x00000000000021c1 <+449>:	leaveq
   0x00000000000021c2 <+450>:	retq   
End of assembler dump.
```

To probe the instruction at `input_A+400` (`cmpl`, which is the condition to exit the loop), you should probe the address `0x2187`.
Note that the addresses in your binary might not be the same.

