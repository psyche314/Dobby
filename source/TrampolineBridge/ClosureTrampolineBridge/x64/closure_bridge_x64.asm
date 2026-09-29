#if defined(__x86_64__)
#if defined(__APPLE__)
#define cdecl(s) _##s
#else
#define cdecl(s) s
#endif

.intel_syntax noprefix
.align 4

.globl cdecl(closure_bridge_asm)
cdecl(closure_bridge_asm):
  // flags register
  pushfq
  // used for alignment
  sub rsp, 8

  // general register
  sub rsp, 16*8
  mov [rsp+8*0], rax
  mov [rsp+8*1], rbx
  mov [rsp+8*2], rcx
  mov [rsp+8*3], rdx
  mov [rsp+8*4], rbp
  mov [rsp+8*5], rsp
  mov [rsp+8*6], rdi
  mov [rsp+8*7], rsi
  mov [rsp+8*8], r8
  mov [rsp+8*9], r9
  mov [rsp+8*10], r10
  mov [rsp+8*11], r11
  mov [rsp+8*12], r12
  mov [rsp+8*13], r13
  mov [rsp+8*14], r14
  mov [rsp+8*15], r15

#define rsp_offset (8*5)
#define orig_rsp_offset (16*8+2*8+8)
  mov rax, rsp
  add rax, orig_rsp_offset // include `closure_tramp_entry_addr` stack var
  mov [rsp+rsp_offset], rax

  mov rbx, rsp
  and rsp, -16
  sub rsp, 16*16 + 32
  movdqu [rsp+32+16*0], xmm0
  movdqu [rsp+32+16*1], xmm1
  movdqu [rsp+32+16*2], xmm2
  movdqu [rsp+32+16*3], xmm3
  movdqu [rsp+32+16*4], xmm4
  movdqu [rsp+32+16*5], xmm5
  movdqu [rsp+32+16*6], xmm6
  movdqu [rsp+32+16*7], xmm7
  movdqu [rsp+32+16*8], xmm8
  movdqu [rsp+32+16*9], xmm9
  movdqu [rsp+32+16*10], xmm10
  movdqu [rsp+32+16*11], xmm11
  movdqu [rsp+32+16*12], xmm12
  movdqu [rsp+32+16*13], xmm13
  movdqu [rsp+32+16*14], xmm14
  movdqu [rsp+32+16*15], xmm15
#if defined(_WIN32)
  mov rcx, rbx
  mov rdx, [rbx+16*8+2*8]
#else
  mov rdi, rbx
  mov rsi, [rbx+16*8+2*8]
#endif
  call cdecl(common_closure_bridge_handler)
  movdqu xmm0, [rsp+32+16*0]
  movdqu xmm1, [rsp+32+16*1]
  movdqu xmm2, [rsp+32+16*2]
  movdqu xmm3, [rsp+32+16*3]
  movdqu xmm4, [rsp+32+16*4]
  movdqu xmm5, [rsp+32+16*5]
  movdqu xmm6, [rsp+32+16*6]
  movdqu xmm7, [rsp+32+16*7]
  movdqu xmm8, [rsp+32+16*8]
  movdqu xmm9, [rsp+32+16*9]
  movdqu xmm10, [rsp+32+16*10]
  movdqu xmm11, [rsp+32+16*11]
  movdqu xmm12, [rsp+32+16*12]
  movdqu xmm13, [rsp+32+16*13]
  movdqu xmm14, [rsp+32+16*14]
  movdqu xmm15, [rsp+32+16*15]
  mov rsp, rbx

  // general register
  pop rax
  pop rbx
  pop rcx
  pop rdx
  pop rbp
  add rsp, 8
  pop rdi
  pop rsi
  pop r8
  pop r9
  pop r10
  pop r11
  pop r12
  pop r13
  pop r14
  pop r15

  // used for alignment
  add rsp, 8
  // flags register
  popfq

  // trick: use `closure_tramp_entry_addr` stack_addr to store the return address
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
