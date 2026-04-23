.text
global switch_context
global task_entrance


; void switch_context(struct task *previous, struct task *next)
switch_context:
	;Due to useing function to schedule, it dont need to save all of register
	;the  sort of this register is follow task structure
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

	ret

;void task_entrance(struce task *firstTask)
task_entrance:
	mov rsp, [rdi]

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx

	ret
