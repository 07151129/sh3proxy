.globl _repl_createDevice, _repl_updateVideoSettings
.text

_repl_createDevice:
    movl %edx, %ds:(0x72c664 + 0x4); /* gD3dPresentParameters->BackBufferHeight */

    push %eax;
    movl _refreshRate, %eax;
    movl %eax, %ds:(0x72c664 + 0x30); /* gD3dPresentParameters->FullScreen_RefreshRateInHz */

    xorl %eax, %eax;
    movb _fullscreen, %al;
    xorb $1, %al;
    movl %eax, %ds:(0x72c664 + 0x20); /* gD3dPresentParameters->Windowed */

    pop %eax;

    jmpl *(ret_createDevice);

_repl_updateVideoSettings:
    movl _refreshRate, %esi;
    movl %esi, %ds:(0x72c664 + 0x30); /* gD3dPresentParameters->FullScreen_RefreshRateInHz */

    jmpl *(ret_updateVideoSettings)

ret_createDevice: .long 0x419628;
ret_updateVideoSettings: .long 0x418e05;
