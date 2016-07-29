#ifdef WRAP_DINPUT
    .globl _DirectInput8Create

    _DirectInput8Create:
        jmp _DirectInput8Create@20
#else
    .globl _Direct3DCreate8

    _Direct3DCreate8:
        jmp _d3d_create@4
#endif
