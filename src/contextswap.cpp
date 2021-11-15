#include "contextswap.h"

extern "C" {
    
    __attribute__((naked)) void context_swap(void */***/, void */***/)
    {
        __asm__(
            "mov %rax, 0x0(%rdi)\n"
            "mov %rbx, 0x8(%rdi)\n"
            "mov %rcx, 0x10(%rdi)\n"
            "mov %rdx, 0x18(%rdi)\n"
            "mov %rbp, 0x28(%rdi)\n"
            "mov %rsp, 0x30(%rdi)\n"
            "mov %r8, 0x38(%rdi)\n"
            "mov %r9, 0x40(%rdi)\n"
            "mov %r10, 0x48(%rdi)\n"
            "mov %r11, 0x50(%rdi)\n"
            "mov %r12, 0x58(%rdi)\n"
            "mov %r13, 0x60(%rdi)\n"
            "mov %r14, 0x68(%rdi)\n"
            "mov %r15, 0x70(%rdi)\n"

            "mov	0x0(%rsi),	%rax\n"
            "mov	0x8(%rsi),	%rbx\n"
            "mov	0x10(%rsi),	%rcx\n"
            "mov	0x18(%rsi),	%rdx\n"
            "mov	0x28(%rsi),	%rbp\n"
            "mov	0x30(%rsi),	%rsp\n"
            "mov	0x38(%rsi),	%r8\n"
            "mov	0x40(%rsi),	%r9\n"
            "mov	0x48(%rsi),	%r10\n"
            "mov	0x50(%rsi),	%r11\n"
            "mov	0x58(%rsi),	%r12\n"
            "mov	0x60(%rsi),	%r13\n"
            "mov	0x68(%rsi),	%r14\n"
            "mov	0x70(%rsi),	%r15\n"
        );
        __asm__ ("ret\n");
    }

    __attribute__((naked)) void rcx_to_rdi_mov()
    {
        __asm__ ("mov %rcx, %rdi\n");
        __asm__ ("ret\n");
    }
}
