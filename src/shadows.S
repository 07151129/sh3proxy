.globl _repl_setShadowTexRes, _repl_setShadowRes, _repl_readLightOp_case0, _repl_readLightOp_case3
.text

_repl_setShadowTexRes:
    // push %eax;
    mov %ds:0x8e43a8, %edx;
    test %edx, %edx;
    je .LisType3;
    mov _ishadowRes, %edx;
    jmp .Lexit;
    .LisType3:
    mov $256, %edx;
    .Lexit:
    push %edx;
    push %edx;
    // pop %eax;
    jmp *(ret_setShadowTexRes);

_repl_setShadowRes:
    push %edx;
    mov %ds:0x8e43a8, %edx;
    test %edx, %edx;
    je .LisType3_sr
    mov _shadowRes, %edx;
    jmp .Lexit_sr;
    .LisType3_sr:
    mov $0x43800000, %edx; /* 256.0f */
    .Lexit_sr:
    mov %edx, %ds:0x6B86C8;
    mov %edx, %ds:0x6B86F8;
    mov %edx, %ds:0x6B86FC;
    mov %edx, %ds:0x6B8714;
    mov %edx, %ds:0x6B8728;
    mov %edx, %ds:0x6B8758;
    mov %edx, %ds:0x6B875C;
    mov %edx, %ds:0x6B8774;
    pop %edx;
    jmp *(ret_setShadowRes);

_repl_readLightOp_case0:
    push %edx;
    mov %ds:0x8e43a8, %edx; /* shadow type changed from type 3 to type 0 */
    test %edx, %edx;
    jne .Lexit_case0;
    call .resetShadowTex;
    .Lexit_case0:
    mov %eax, %ds:0x8e43a8;
    pop %edx;
    mov %edx, %ds:0x8e43b0;
    jmp *(ret_readLightOp_case0);

_repl_readLightOp_case3:
    push %edx;
    mov %ds:0x8e43a8, %edx; /* shadow type changed from type 0 to type 3 */
    test %edx, %edx;
    je .Lexit_case3;
    call .resetShadowTex;
    .Lexit_case3:
    movl $0, %ds:0x8e43a8;
    pop %edx;
    jmp *(ret_readLightOp_case3);

ret_setShadowTexRes: .long 0x48b1bf
ret_setShadowRes: .long 0x48d8db
ret_readLightOp_case0: .long 0x49109a
ret_readLightOp_case3: .long 0x49111d

.resetShadowTex: .long 0x48b1b50
