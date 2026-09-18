.text
global switch_context
global task_entrance

; void switch_context(struct task *previous, struct task *next)
switch_context:
	push rax
	push rdx
	add rsp, 0x10
	pop rax		;RIP
	mov rdx, rsp
	sub rsp, 0x18

	;user space need to change
	push qword 0x10		;SS
	push rdx		;RSP
	pushfq
	or qword [rsp], 0x200	;ENABLE IF
	cli
	push qword 0x8		;CS
	push rax
	
	add rsp, 0x28
	pop rdx
	pop rax
	sub rsp, 0x38

	push rax
	push rcx
	push rdx
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15

	;Due to the first field of the task structure is rsp 
	mov [rdi], rsp
	mov rsp, [rsi]

	;the third field of the task structure is cr3
	mov rax, [rsi + 16]
	mov rdx, cr3
	cmp rax, rdx
	je .dont_switch
	mov cr3, rax
.dont_switch
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	sti
	ret

;void task_entrance(struce task *firstTask)
task_entrance:
	mov rsp, [rdi]

        mov rax, [rdi + 16]
        mov cr3, rax

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	sti
	ret








