/* Tell the linker that we want an x86_64 ELF64 output file */
OUTPUT_FORMAT(elf64-x86-64)

/* We want the symbol _start to be our entry point */
ENTRY(_start)

/* Define the program headers we want so the bootloader gives us the right */
/* MMU permissions */
PHDRS
{
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    /* We wanna be placed in the topmost 2GiB of the address space, for optimisations */
    /* and because that is what the Limine spec mandates. */
    /* Any address in this region will do, but often 0xffffffff80000000 is chosen as */
    /* that is the beginning of the region. */
    . = 0xffffffff80000000;

    PROVIDE(addr_text_start = .);

    .text : {
        *(.text .text.*)
    } :text

    PROVIDE(addr_text_end = .);

    /* Move to the next memory page for .rodata */
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    
    PROVIDE(addr_rodata_start = .);

    .rodata : {
        PROVIDE_HIDDEN(init_array_start = .);
        *(.init_array .init_array.*)
        PROVIDE_HIDDEN(init_array_end = .);
        *(.rodata .rodata.*)
    } :rodata


    PROVIDE(addr_rodata_end = .);

    /* Move to the next memory page for .data */
    . = ALIGN(CONSTANT(MAXPAGESIZE));

    PROVIDE(addr_data_start = .);

    .data : {
        *(.data .data.*)

        KEEP(*(.requests_start_marker))
        KEEP(*(.requests))
        KEEP(*(.requests_end_marker))
    } :data

    /* Dynamic section for relocations, both in its own PHDR and inside data PHDR */
    .dynamic : {
        *(.dynamic)
    } :data :dynamic

    /* NOTE: .bss needs to be the last thing mapped to :data, otherwise lots of */
    /* unnecessary zeros will be written to the binary. */
    /* If you need, for example, .init_array and .fini_array, those should be placed */
    /* above this. */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    PROVIDE(addr_data_end = .);


    /* Discard .note.* and .eh_frame since they may cause issues on some hosts. */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
