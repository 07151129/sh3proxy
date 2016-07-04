.globl _repl_setTransform
.text

_repl_setTransform:
    push %eax;

    movl _projH, %eax;
    movl %eax, %ds:(0x72c360);

    movl _projV, %eax;
    movl %eax, %ds:(0x72c360 + 20);

    pop %eax;
    call *%ds:0x94(%ecx);
    jmpl *(ret)

ret: .long 0x67bbad
