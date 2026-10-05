ENTRY(prx_load_gate)
SECTIONS { . = 0; .text : { *(.text.gate) *(.text*) } /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) } }
