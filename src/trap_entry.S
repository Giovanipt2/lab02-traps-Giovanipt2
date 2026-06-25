# this is what will be loaded into the stvec CSR; the
# trap handler address must be 4-byte aligned:
# https://docs.riscv.org/reference/isa/priv/supervisor.html#12-1-1-2-supervisor-trap-vector-base-address-stvec-register
.globl trap_entry
.align 4
trap_entry:

    # Allocate space for 31 registers (x1..x31, skipping x0 which is hardwired zero)
    # 31 * 8 = 248 bytes
    addi sp, sp, -248

    # Save all general-purpose registers except x0
    sd   ra,   0(sp)    # x1
    sd   gp,   8(sp)    # x3  (x2/sp saved below after adjustment)
    sd   tp,  16(sp)    # x4
    sd   t0,  24(sp)    # x5
    sd   t1,  32(sp)    # x6
    sd   t2,  40(sp)    # x7
    sd   s0,  48(sp)    # x8
    sd   s1,  56(sp)    # x9
    sd   a0,  64(sp)    # x10
    sd   a1,  72(sp)    # x11
    sd   a2,  80(sp)    # x12
    sd   a3,  88(sp)    # x13
    sd   a4,  96(sp)    # x14
    sd   a5, 104(sp)    # x15
    sd   a6, 112(sp)    # x16
    sd   a7, 120(sp)    # x17
    sd   s2, 128(sp)    # x18
    sd   s3, 136(sp)    # x19
    sd   s4, 144(sp)    # x20
    sd   s5, 152(sp)    # x21
    sd   s6, 160(sp)    # x22
    sd   s7, 168(sp)    # x23
    sd   s8, 176(sp)    # x24
    sd   s9, 184(sp)    # x25
    sd   s10,192(sp)    # x26
    sd   s11,200(sp)    # x27
    sd   t3, 208(sp)    # x28
    sd   t4, 216(sp)    # x29
    sd   t5, 224(sp)    # x30
    sd   t6, 232(sp)    # x31

    # Save original sp (x2): sp + 248 is the value before we allocated
    addi t0, sp, 248
    sd   t0, 240(sp)    # store original sp at offset 240 (x2 slot)

    call handle_trap

    # Restore all general-purpose registers except x0 and sp
    ld   ra,   0(sp)
    ld   gp,   8(sp)
    ld   tp,  16(sp)
    ld   t0,  24(sp)
    ld   t1,  32(sp)
    ld   t2,  40(sp)
    ld   s0,  48(sp)
    ld   s1,  56(sp)
    ld   a0,  64(sp)
    ld   a1,  72(sp)
    ld   a2,  80(sp)
    ld   a3,  88(sp)
    ld   a4,  96(sp)
    ld   a5, 104(sp)
    ld   a6, 112(sp)
    ld   a7, 120(sp)
    ld   s2, 128(sp)
    ld   s3, 136(sp)
    ld   s4, 144(sp)
    ld   s5, 152(sp)
    ld   s6, 160(sp)
    ld   s7, 168(sp)
    ld   s8, 176(sp)
    ld   s9, 184(sp)
    ld   s10,192(sp)
    ld   s11,200(sp)
    ld   t3, 208(sp)
    ld   t4, 216(sp)
    ld   t5, 224(sp)
    ld   t6, 232(sp)

    # Restore sp last
    addi sp, sp, 248

    sret
