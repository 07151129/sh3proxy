.globl _repl_setWindowStyle, _repl_setTexRes, _repl_setPresentationIntervalWindowed, _repl_setPresentationIntervalFullscreen
.text

_repl_setWindowStyle:
    pushl $0x80000000; /* X */
    pushl $0x80000000; /* dwStyle = WS_POPUP */
    jmp *(ret_setWindowStyle);

_repl_setTexRes:
    movl _resY, %eax;
    movl %eax, 8(%esp);
    movl _resX, %eax;
    movl %eax, 4(%esp);
    pushl %ecx;
    jmp *(ret_setTexRes);

_repl_setPresentationIntervalWindowed:
    movl _vsync, %ebx;
    test %ebx, %ebx;
    jnz .L_setPresentationIntervalW_true;
    movl $0x80000000, %ebx; /* D3DPRESENT_INTERVAL_IMMEDIATE */
    movl %ebx, (0x72c694);
    jmp *(ret_setPresentationIntervalWindowed);
    .L_setPresentationIntervalW_true:
    movl $1, %ebx; /* D3DPRESENT_INTERVAL_ONE */
    movl %ebx, (0x72c694);
    jmp *(ret_setPresentationIntervalWindowed);

_repl_setPresentationIntervalFullscreen:
    movl _vsync, %ebx;
    test %ebx, %ebx;
    jnz .L_setPresentationIntervalF_true;
    movl $0x80000000, %ebx; /* D3DPRESENT_INTERVAL_IMMEDIATE */
    movl %ebx, (0x72c694);
    jmp *(ret_setPresentationIntervalFullscreen);
    .L_setPresentationIntervalF_true:
    movl $1, %ebx; /* D3DPRESENT_INTERVAL_ONE */
    movl %ebx, (0x72c694);
    jmp *(ret_setPresentationIntervalFullscreen);

ret_setWindowStyle: .long 0x403038
ret_setTexRes: .long 0x402e06
ret_setPresentationIntervalWindowed: .long 0x418e66
ret_setPresentationIntervalFullscreen: .long 0x418e05
