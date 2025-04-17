	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register; it is caller saved.
 */
	.globl thread_switch
thread_switch:
    # 保存当前线程的寄存器
	pushal
    # 将当前栈指针保存到 current_thread->sp
    movl current_thread, %eax
    movl %esp, (%eax)
    # 切换目标线程：current_thread = next_thread
    movl next_thread, %eax
    movl %eax, current_thread
    # 加载新线程的栈指针
    movl (%eax), %esp
    # 恢复新线程的寄存器
    popal
    # 返回到新线程的执行流
	ret				/* pop return address from stack */
