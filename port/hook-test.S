.text
.globl port_probe
.hidden port_probe
.type port_probe,@function
port_probe:
mov port_probe_value(%rip), %eax
add %edi, %eax
.rept 6
nop
.endr
ret
.size port_probe,.-port_probe
.section .note.GNU-stack,"",@progbits
