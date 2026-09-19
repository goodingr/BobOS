BITS 64

extern exception_handler

global isr_stub_0
global isr_stub_6

isr_stub_0:
    mov rdi, 0
    call exception_handler
    cli
    hlt

isr_stub_6:
    mov rdi, 6
    call exception_handler
    cli
    hlt