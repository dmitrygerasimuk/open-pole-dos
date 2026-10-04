; POLE seg000:5E81..5FB4: author's 8086 sprite renderer, not Pascal output.
; TASM 3.1 OMF, linked by Turbo Pascal 5.5.
; Pascal: procedure DrawRLEPackedStaff(var Source, Dest; X,Y,Transparent: Word);
; Stack: source +14, destination +10, X +8, Y +6, transparent low byte +4.
; Locals: rows -2, encoded row end -4, current bit -6, initial bit -8.
.MODEL TPASCAL                 ; Turbo Pascal code model
.8086
LOCALS @@                      ; Local labels prefix
.DATA
.CODE
PUBLIC PutPic

PutPic PROC NEAR
 push bp
 mov bp, sp
 sub sp, 8
 push ds
 cld
 lds si, [bp+14]
 les di, [bp+10]
 lodsw                         ; width is ignored
 lodsw                         ; height
 mov [bp-2], ax
 add si, 2                     ; skip header filler
 mov bx, [bp+6]
 mov ax, bx
 mov cl, 6
 shl ax, cl
 sub cl, 2
 shl bx, cl
 add bx, ax                  ; Y * 80
 dec cl                        ; CL = 3
 mov ax, [bp+8]
 mov ch, al
 shr ax, cl
 add bx, ax                  ; Y * 80 + X div 8
 add di, bx
 and ch, 7
 mov [bp-6], ch
 mov [bp-8], ch
 mov dx, 03CEh
 mov ax, 0205h                 ; EGA write mode 2
 out dx, ax
 mov al, 8                     ; bit-mask register
 out dx, al
 inc dx
@@row:
 push di
 mov ch, [bp-8]
 mov [bp-6], ch
 lodsw
 add ax, si
 mov [bp-4], ax                ; end of this encoded row
 lodsb
@@control:
 cmp al, 0FFh                  ; SIGNED: 00..7F literal; 80..FF repeat
 jle short @@repeat
 mov bl, al
 mov cl, [bp-6]
 mov ah, 080h
 shr ah, cl
@@literal_pixel:
 lodsb
 cmp al, [bp+4]
 je short @@literal_advance
 xchg al, ah                   ; TASM operand order preserves opcode 86 C4
 out dx, al
 mov bh, es:[di]               ; latch read must be retained
 mov es:[di], ah
 mov ah, al
@@literal_advance:
 inc cl
 ror ah, 1
 jnc short @@literal_count
 inc di
 xor cl, cl
@@literal_count:
 dec bl
 jne short @@literal_pixel
 mov [bp-6], cl
 jmp near ptr @@next_control
@@repeat:
 sub al, 080h
 mov bl, al
 mov ch, al                  ; retained even though CH is not used below
 mov cl, [bp-6]
 mov al, 0FFh
 shr al, cl
 mov ah, al
 lodsb
 mov bh, al
 cmp al, [bp+4]
 je short @@transparent
 add bl, [bp-6]
 cmp bl, 7
 ja short @@first_full
 mov cl, bl
 xor cl, 7
 mov al, 0FEh
 shl al, cl
 and al, ah
 mov cl, bl
 jmp short @@first_write
 nop                           ; original assembler's short-jump padding
@@first_full:
 mov al, ah
 xor cl, cl
@@first_write:
 out dx, al
 mov ah, es:[di]
 mov es:[di], bh
 sub bl, 8
 cmp bl, 0
 jl short @@repeat_done
 inc di
 mov al, 0FFh
 out dx, al
 mov al, bh
@@whole_bytes:
 cmp bl, 7
 jle short @@last_byte
 stosb
 sub bl, 8
 jmp short @@whole_bytes
@@last_byte:
 mov al, 0FEh
 mov cl, bl
 xor cl, 7
 shl al, cl
 mov cl, bl
 out dx, al
 mov ah, es:[di]
 mov es:[di], bh
@@repeat_done:
 mov [bp-6], cl
 jmp short @@next_control
 nop
@@transparent:
 add bl, [bp-6]
 mov cl, bl
 cmp bl, 7
 jle short @@skip_first
 xor cl, cl
@@skip_first:
 sub bl, 8
 cmp bl, 0
 jl short @@skip_done
 inc di
@@skip_whole:
 cmp bl, 7
 jle short @@skip_last
 inc di
 sub bl, 8
 jmp short @@skip_whole
@@skip_last:
 mov cl, bl
@@skip_done:
 mov [bp-6], cl
@@next_control:
 lodsb                         ; original reads sentinel before checking end
 mov bx, [bp-4]
 cmp bx, si
 je short @@row_done
 jmp near ptr @@control
@@row_done:
 pop di
 add di, 80
 dec word ptr [bp-2]
 je short @@done
 jmp near ptr @@row
@@done:
 pop ds
 mov sp, bp
 pop bp
 ret 14
PutPic ENDP
END
