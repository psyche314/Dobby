#if defined(__arm64__) || defined(__aarch64__)
#if defined(__APPLE__)
#define cdecl(s) _##s
#else
#define cdecl(s) s
#endif

#define TMP_REG_0 x17
#if defined(FULL_FLOATING_POINT_REGISTER_PACK)
#define FP_SIZE (32 * 16)
#else
#define FP_SIZE (8 * 16)
#endif
.align 4

.globl cdecl(closure_bridge_asm)
cdecl(closure_bridge_asm):
// alloc stack, save {q0 - q7}
sub sp, sp, #FP_SIZE
stp q6, q7, [sp, #(6 * 16)]
stp q4, q5, [sp, #(4 * 16)]
stp q2, q3, [sp, #(2 * 16)]
stp q0, q1, [sp, #(0 * 16)]

#if defined(FULL_FLOATING_POINT_REGISTER_PACK)
stp q8, q9, [sp, #(8 * 16)]
stp q10, q11, [sp, #(10 * 16)]
stp q12, q13, [sp, #(12 * 16)]
stp q14, q15, [sp, #(14 * 16)]
stp q16, q17, [sp, #(16 * 16)]
stp q18, q19, [sp, #(18 * 16)]
stp q20, q21, [sp, #(20 * 16)]
stp q22, q23, [sp, #(22 * 16)]
stp q24, q25, [sp, #(24 * 16)]
stp q26, q27, [sp, #(26 * 16)]
stp q28, q29, [sp, #(28 * 16)]
stp q30, q31, [sp, #(30 * 16)]
#endif

// alloc stack, save {x1 - x30}
sub sp, sp, #(30 * 8)
stp x29, x30, [sp, #(28 * 8)]
stp x27, x28, [sp, #(26 * 8)]
stp x25, x26, [sp, #(24 * 8)]
stp x23, x24, [sp, #(22 * 8)]
stp x21, x22, [sp, #(20 * 8)]
stp x19, x20, [sp, #(18 * 8)]
stp x17, x18, [sp, #(16 * 8)]
stp x15, x16, [sp, #(14 * 8)]
stp x13, x14, [sp, #(12 * 8)]
stp x11, x12, [sp, #(10 * 8)]
stp x9, x10, [sp, #(8 * 8)]
stp x7, x8, [sp, #(6 * 8)]
stp x5, x6, [sp, #(4 * 8)]
stp x3, x4, [sp, #(2 * 8)]
stp x1, x2, [sp, #(0 * 8)]

// alloc stack, save {x0}
sub sp, sp, #(2 * 8)
str x0, [sp, #(1 * 8)]

// calc original sp
add TMP_REG_0, sp, #(2 * 8) // closure trampoline reserved
add TMP_REG_0, TMP_REG_0, #(2 *8 + 30 * 8 + FP_SIZE) // x0, x1-x30, q0-q7 reserved

// alloc stack, save original sp
sub sp, sp, #(2 * 8)
str TMP_REG_0, [sp, #(1 * 8)]

// call convention: x0 = register context, x1 = interceptor entry
mov x0, sp
ldr x1, [sp, #(2 * 8 + 2 * 8 + 30 * 8 + FP_SIZE)]
bl cdecl(common_closure_bridge_handler)

// restore stack, saved original sp
add sp, sp, #(2 * 8)

// restore stack, saved {0}
ldr x0, [sp, #(1 * 8)]
add sp, sp, #(2 * 8)

// restore stack, saved {x1 - x30}
ldp x1, x2, [sp, #(0 * 8)]
ldp x3, x4, [sp, #(2 * 8)]
ldp x5, x6, [sp, #(4 * 8)]
ldp x7, x8, [sp, #(6 * 8)]
ldp x9, x10, [sp, #(8 * 8)]
ldp x11, x12, [sp, #(10 * 8)]
ldp x13, x14, [sp, #(12 * 8)]
ldp x15, x16, [sp, #(14 * 8)]
ldp x17, x18, [sp, #(16 * 8)]
ldp x19, x20, [sp, #(18 * 8)]
ldp x21, x22, [sp, #(20 * 8)]
ldp x23, x24, [sp, #(22 * 8)]
ldp x25, x26, [sp, #(24 * 8)]
ldp x27, x28, [sp, #(26 * 8)]
ldp x29, x30, [sp, #(28 * 8)]
add sp, sp, #(30 * 8)

// restore stack, saved {q0 - q7}
ldp q0, q1, [sp, #(0 * 16)]
ldp q2, q3, [sp, #(2 * 16)]
ldp q4, q5, [sp, #(4 * 16)]
ldp q6, q7, [sp, #(6 * 16)]
#if defined(FULL_FLOATING_POINT_REGISTER_PACK)
ldp q8, q9, [sp, #(8 * 16)]
ldp q10, q11, [sp, #(10 * 16)]
ldp q12, q13, [sp, #(12 * 16)]
ldp q14, q15, [sp, #(14 * 16)]
ldp q16, q17, [sp, #(16 * 16)]
ldp q18, q19, [sp, #(18 * 16)]
ldp q20, q21, [sp, #(20 * 16)]
ldp q22, q23, [sp, #(22 * 16)]
ldp q24, q25, [sp, #(24 * 16)]
ldp q26, q27, [sp, #(26 * 16)]
ldp q28, q29, [sp, #(28 * 16)]
ldp q30, q31, [sp, #(30 * 16)]
#endif
add sp, sp, #FP_SIZE

ret

.globl cdecl(closure_bridge_asm_end)
cdecl(closure_bridge_asm_end):

.data
.align 8
common_closure_bridge_handler_addr:
.quad cdecl(common_closure_bridge_handler)
#endif
#if defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
