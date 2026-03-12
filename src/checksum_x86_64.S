    .text
    .globl packet_checksum32
    .type packet_checksum32, @function
packet_checksum32:
    xor %eax, %eax
    test %rdi, %rdi
    je .done
    test %rsi, %rsi
    je .done
.loop:
    movzbl (%rdi), %edx
    add %edx, %eax
    inc %rdi
    dec %rsi
    jne .loop
.done:
    ret

    .section .note.GNU-stack,"",@progbits
