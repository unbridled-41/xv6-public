#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "stddef.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int 
sys_date(void){
  struct rtcdate *r;
  if(argptr(0,(char**)&r,sizeof(struct rtcdate))<0) return -1;
  //初始化r
  cmostime(r);
  return 0;
}

int
sys_dup2(void)
{
    int fd0, fd1;
    struct file *f;

    // 从用户栈中获取参数
    if (argint(0, &fd0) < 0 || argint(1, &fd1) < 0) {
        return -1; // 参数错误
    }

    // 如果目标文件描述符已打开，则先关闭它
    if (fd1 >= 0 && fd1 < NOFILE && myproc()->ofile[fd1] != NULL) {
        fileclose(myproc()->ofile[fd1]);
        myproc()->ofile[fd1] = NULL;
    }

    // 获取源文件描述符对应的文件结构
    if ((f = myproc()->ofile[fd0]) == NULL) {
        return -1; // 源文件描述符无效
    }

    // 将文件结构赋值给目标文件描述符
    if (fd1 >= 0 && fd1 < NOFILE) {
        myproc()->ofile[fd1] = f;
        return fd1;
    } else {
        return -1; // 目标文件描述符超出范围
    }
}

int
sys_alarm(void) {
  int ticks;
  void (*handler)();

  // 从用户空间获取参数
  if (argint(0, &ticks) < 0)     // 获取报警间隔
    return -1;
  if (argptr(1, (char**)&handler, 1) < 0)  // 获取处理函数指针
    return -1;

  // 保存到当前进程的 proc 结构体
  struct proc *p = myproc();
  p->alarmticks = ticks;
  p->alarmhandler = handler;
  p->remaining_ticks = ticks;    // 初始化剩余计数
  return 0;
}

