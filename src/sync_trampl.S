.globl _report_framerate_trampl, _scale_event_rate_trampl
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
    jmpl *(ret_report_framerate_trampl);

_scale_event_rate_trampl:
    pushl %ecx;
    call _scale_event_rate;
    jmpl *(ret_scale_event_rate);

ret_report_framerate_trampl: .long 0x419960
ret_scale_event_rate: .long 0x5f1a79
