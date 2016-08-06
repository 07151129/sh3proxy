.globl _repl_texInit, _repl_setWindowStyle
.text

_repl_texInit:
    add $4, %esp;
    lea 0x15(,%eax,4), %eax;
    cmp $512, %edi;
    jne .L_texInit_ne512;
    push $0
    jmp *(ret_texInit);
    .L_texInit_ne512:
    push $1;
    jmp *(ret_texInit);

_repl_setWindowStyle:
    pushl $0x80000000; /* X */
    pushl $0x80000000; /* dwStyle = WS_POPUP */
    jmp *(ret_setWindowStyle);

ret_texInit: .long 0x461dc4
ret_setWindowStyle: .long 0x403038
