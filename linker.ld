/* 64bit ELFとして出力 */
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS
{
    text    PT_LOAD    FLAGS(5); /* RX */
    rodata  PT_LOAD    FLAGS(4); /* R  */
    data    PT_LOAD    FLAGS(6); /* RW */
}

SECTIONS
{
    /* Limineプロトコルの推奨アドレス */
    . = 0xffffffff80000000;

    .text : {
        *(.text .text.*)
    } :text

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
