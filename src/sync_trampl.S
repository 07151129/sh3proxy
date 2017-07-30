.globl _report_framerate_trampl
.text

_report_framerate_trampl:
    pushl %eax;
    pushl %edx;
    call _report_framerate;
    popl %edx;
    popl %eax;
    push $0;
    push $0;
    push $0;
    jmpl *(ret_report_framerate_trampl)

ret_report_framerate_trampl: .long 0x419960
