.data 

.text 
add:
    lw $s5, -4($sp) 
    lw $s6, -8($sp) 
    addu $t0, $s5, $s6 
    move $v0, $t0 
    jr $ra