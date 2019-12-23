.file	"sections.c"             // File name where the code was generated from    
	.text                        // Instruction codes are defined here
	.globl	hello               //  declaring global character array hello
	.data                       //  Beginning of data section (initialized read/write data is defined)
	.type	hello, @object     //   type of hello []- is an object
	.size	hello, 6           //   size of the object hello is 6
hello:                         //  start of character array hello content(label)
	.string	"hello"            //  declaring hello as a string
	.globl	world              //  declaring global character array world
	.section	.rodata        //  read only data section
.LC0:                          //  Memory address where “world” begins
	.string	"world"            //  string located at the memory address .LC0
	.section	.data.rel.local,"aw",@progbits   // Bits alignment and allocation in machine code
	.align 8                   // align the machine code by 8 bytes
	.type	world, @object     // type of the character array world
	.size	world, 8           // size of the character array world
world:                         // Beginning of the character array, world
	.quad	.LC0               // Indicates the size of data in memory .LC0
	.comm	arr_e,10,8         // allocates 10 bytes for the arr_e with an 8 byte boundary in the bss section
	.local	arr_s              // Local (to foo) array arr_s
	.text                      // beginning of code segment
	.globl	foo                // making foo visible to global
	.type	foo, @function     // the type of foo
foo:                           // Start of the function foo
.LFB0:                         // Local label at the start of foo
	pushq	%rbp               // Save the stack frame of the caller
	movq	%rsp, %rbp         // Allocate a new frame for a new function call foo
	subq	$48, %rsp          // Allocate 48 bytes as a stack for foo
	movq	%rdi, -40(%rbp)    // make space for first argument in stack
	movq	%rsi, -48(%rbp)    // make space for second argument in stack
	movq	%fs:40, %rax       // allocate 40 bytes for the array, arr_a 
	movq	%rax, -8(%rbp)     // declare arr_a
	xorl	%eax, %eax         // Clear the value of register
	movq	world(%rip), %rdx  // rdx points to world string
	movq	-40(%rbp), %rax    // get argument 1(s) and save to rax
	addq	%rdx, %rax         // add base address(rdx) and argument 1(s)
	movzbl  (%rax), %eax       // set eax register to arr_a[d]
	leaq	-18(%rbp), %rcx    // getting pointer of arr_a and save to rcx
	movq	-48(%rbp), %rdx    // get argument 2(d) and save to rdx
	addq	%rcx, %rdx         // add base address(rcx) and rdx(d)
	movb	%al, (%rdx)        // move world[s ] into arr_a[d] 
	leaq	hello(%rip), %rdx  // rdx point at hello string
	movq	-40(%rbp), %rax    // get argument 1(s) and save to rax register
	addq	%rdx, %rax         // add base address of hello with argument 1(s)
	movzbl  (%rax), %eax       // set eax register to arr_e[d]
	leaq	arr_e(%rip), %rcx  // setting pointer of arr_e and save to rcx
	movq	-48(%rbp), %rdx    // get argument 2(d)  and save to rdx
	addq	%rcx, %rdx         // add base address(rdx) and argument 1(s)
	movb	%al, (%rdx)        // set char in eax to arr_e[d]
	movq	world(%rip), %rdx  // rdx points to world string
	movq	-40(%rbp), %rax    // get argument 1 and save to rax
	addq	%rdx, %rax         // add base address rdx and argument 1
	movzbl	(%rax), %eax       // set eax register to arr_s[d]
	leaq	arr_s(%rip), %rcx  // get pointer of arr_s & save to rcx
	movq	-48(%rbp), %rdx    // get argument 2d & save to rdx register
	addq	%rcx, %rdx         // add base address(rcx) and rdx
	movb	%al, (%rdx)        // assign char in eax to arr_x[d]
	movq	-48(%rbp), %rdx     // preparing parameteres for bar(arg 1)
	movq	-40(%rbp), %rax     // prepare parameteres for bar (arg 2)
	movq	%rdx, %rsi          // move arg 1 to a register
	movq	%rax, %rdi          // move arg 2 in a register
	call	bar@PLT             // call bar function
	movl	$0, %eax            // move return value 0 to eax
	movq	-8(%rbp), %rsi      // take away space from rbp allocated by rsi
	xorq	%fs:40, %rsi        // clear out rsi register
.L3:                            // memeory section L3
	leave                       // restore caller's stack frame
	ret                         // get the return address and go to caller
.LFE0:                          // Memory segment for .LFE0
	.size	foo, .-foo          // size of the content in LFE0
	.globl	main                // declare main as global fun
	.type	main, @function     // telling the type of main which is function
main:                           // start of main function
.LFB1:                          // a memory segemnt for LFB1 
	pushq	%rbp                // save base pointer
	movq	%rsp, %rbp          // start a new stack frame 
	movl	$2, %esi            // Pass $2 as the first argument
	movl	$1, %edi            // Pass $1 as the second argument
	call	foo                 // call foo
	popq	%rbp                // remove base pointer
	ret                         // return from main
.LFE1:                          // Memory section for LFE1
	.size	main, .-main        // size of main 
