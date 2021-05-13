# shelllab
## 做什么
补全缺失的函数，以实现简易的命令行。
### 需要补全的函数有：
1. `void eval(char *cmdline)`//对一条命令进行解析运行
2. `builtin_cmd(char **argv)` //解析是内置命令还是可执行性文件名
3. `void do_bgfg(char **argv)`//执行bg，fg命令 
4. `void waitfg(pid_t pid)`//等待前台进程结束
5. `void sigchld_handler(int sig)` //SIGCHLD信号处理函数
6. `void sigint_handler(int sig)` //SIGINT处理函数
7. `void sigtstp_handler(int sig)` //SIGSTP处理函数
8. 以及一系列的包装函数如`Fork`，`Execve`，`Wait`，`Waitpid`等
## 怎么做
我在开始时只知道根据书上的shell例子写，但是书上明确指出了他的shell程序无法回收后台运行的程序，要想解决这个问题，引入了信号的概念来通过信号得知后台进程的状态，并根据状态来操作后台进程。
eval，解析并处理一条命令。可能的疑问：
1. 为什么阻塞信号，什么时候阻塞信号?
答：在对jobs进行修改时阻塞信号，因为如果在修改jobs前，或者修改jobs的元素时，进入其他的信号处理程序，比如bg，fg，sigstp，sigint，sigchld等可能会导致jobs的元素值可能是错误的，不符合预期的，甚至是相互矛盾的，并且每次运行结果都可能不同，最终导致错误。
2. 注意一点，在此程序中程序组ID和PID一致，在程序中会修改和体现为什么这么做（发送信号时方便）。
```c
void eval(char *cmdline)//对一条命令进行解析运行
{  
    char **argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;
    sigset_t mask_all, mask_chld, prev_chld;

    Sigfillset(&mask_all);
    Sigemptyset(&mask_chld);
    Sigaddset(&mask_chld, SIGCHLD);

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if(argv[0] == NULL)
        return;
    if(!builtin_cmd(argv)){

        if((pid = Fork()) == 0){//子进程才会进入
            Setpgid(0, 0);//成立一个新的进程组，进程组ID与进程ID一致。
            Sigprocmask(SIG_SETMASK, &prev_chld, NULL);
            Execve(argv[0], argv, environ);
        }
        Sigprocmask(SIG_BLOCK, &mask_all, NULL);
        addjob(jobs, pid, bg + 1, cmdline);/*在对jobs进行修改时，对所有信号进行拥堵，不然由于竞争信号处理程序的突然插入，可能会造成自相矛盾的错误结果。*/
        Sigprocmask(SIG_SETMASK, &prev_chld, NULL);
        if(!bg){
            waitfg(pid);//等待前台程序运行完成
        }
        else
            printf("[%d] (%d) %s\n", pid2jid(pid), pid, cmdline);
    }
    return;
}
```
buildin_cmd函数
该函数的目的时解析命令，到底时内置命令如：quit，bg，fg，jobs还是目录内的可执行性文件，这个函数与书中的例子一样，没有特别的改进。是内置命令则直接在这执行了，并返回1，不是则返回0。
```c
int builtin_cmd(char **argv) 
{
    if(!strcmp(argv[0], "quit"))
        exit(0);
    if(!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg") ){
        do_bgfg(argv);
        return 1;
    }
    if(!strcmp(argv[0], "jobs")){
        listjobs(jobs);
        return 1;
    }
    if(!strcmp(argv[0], "&")){
        return 1;
    }
    return 0;     /* not a builtin command */
}

```
执行bg，fg命令。可能的疑问：
1. `sscanf(argv[1], "%%%d", &jid)` 啥意思？
答：因为如果使用jobs jid格式要在jid前面加上百分号，sscanf中如果匹配%则要需要打两个百分号。
2. 为什么更改完状态后，要发送给该进程组的所有进程SIGCONT信号重启呢？
答： 我也不知道为啥...,可能是在更改进程状态前，先发送SIGSTP信号使得进程暂停在修改，修改后在发送SIGCONT信号使其恢复运行，但没有找到理论依据。
```c
void do_bgfg(char **argv) 
{   
    int jid;
    pid_t pid;
    struct job_t *job;
    sigset_t mask, prev;
    if(argv[1]==NULL){
        printf("Illegal command!\n");
        return;
    }

    if(sscanf(argv[1], "%%%d", &jid) > 0 ){
        job = getjobjid(jobs, jid);
        if(job == NULL || job->state == UNDEF){
                printf("(%s): NO such process\n", argv[1]);
                return;
        }
    }
    else if(sscanf(argv[1], "%d", &pid) > 0){
        job = getjobpid(jobs, pid);
            if(job == NULL || job->state == UNDEF){
                printf("(%s): NO such process\n", argv[1]);
                return;
        }
    } else{
        printf("The format behind the %s command is pid_t or %%jobid", argv[0]);
        return;
    }
    Sigfillset(&mask);
    Sigprocmask(SIG_BLOCK, &mask, &prev);
    if(!strcmp(argv[0], "bg")){
        job->state = BG;
    }
    else{
        job->state = FG;
    }
    Sigprocmask(SIG_SETMASK, &prev, NULL);
    /*https://zhuanlan.zhihu.com/p/119034923
    * 这篇文章中作者在这里发送SIGCONT信号给该进程组的所有进程，来唤醒* 停止进程，我没搞懂为什么要这么做，就加上吧。
   */ 
  	pid = job->pid;
	//发送SIGCONT重启
	kill(-pid, SIGCONT);

	if(!strcmp(argv[0], "fg")){
		waitfg(pid);
	}else{
		printf("[%d] (%d) %s", job->jid, pid, job->cmdline);
	}

}
```
不断的查询该pid的状态，是否还是前台，如果状态改变则退出。也可以使用书中介绍的方法`sigsuspend`。
```c
void waitfg(pid_t pid)
{
    while(getjobpid(jobs, pid)->state == FG){
        sleep(0.01);
    }
    return;
}
```
sigchld_handler:sigchld信号处理函数：
可能的疑问点：
1. 为什么按照这种思路写，有什么依据没有？
  答：我在书中没有具体找到什么时候发送SIGCHLD信号，所以参考
  https://blog.csdn.net/qq_33883085/article/details/89325396

2. 为什么是waitpid(-1, &state, WNOHANG | WUNTRACED)
  而不是 waitpid(-1, &state, WNOHANG)
  答： 因为只有WNOHANG时，如果子进程是在暂停的，如果成功调用也只会返回0，则不会被处理。

3. 为什么是用while？用if不可以吗？

  答：他可以正确解决信号不会排队等待的情况。
```c
void sigchld_handler(int sig) 
{
    
    int old_errno = errno;
    pid_t pid;
    sigset_t mask, prev;
    int state;
    struct job_t *job;
    Sigfillset(&mask);
    /*参考：https://blog.csdn.net/qq_33883085/article/details/89325396
    * 什么时候进程会发送sigchld信号
    */
    while((pid = waitpid(-1, &state, WNOHANG | WUNTRACED)) > 0){
        /* 为何是WNOHANG | WUNTRACED
        * 而不是WNOHANG，因为只有WNOHANG时，如果子进程是在暂停的，则不会被处理，如果成功调用也只会返回0。
        */
       Sigprocmask(SIG_BLOCK, &mask, &prev);
        if(WIFEXITED(state)){
            deletejob(jobs, pid);
        }
        else if(WIFSIGNALED(state)){
            printf("PID:(%d) terminated due to an unknown signal: %d\n", pid, WTERMSIG(state));
            deletejob(jobs, pid);
        }
        else if(WIFSTOPPED(state)){
            //无需处理，只需要更新自己维护的状态即可。
            job = getjobpid(jobs, pid);
            job->state = ST;
            printf("PID：(%d), the signal that caused the child process to stop: %d\n", pid, WSTOPSIG(state));
        }
        Sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    errno = old_errno;
    return;
}
```
SIGINT信号处理程序：
可能的问题：
1. 为什么给进程组发送信号就可以了，不是已经改变了信号处理的默认行为了吗？
答：https://blog.csdn.net/qq_43648751/article/details/104623880
因为如果signal对信号的行为是捕获则，fork之后运行新的程序则会覆盖主进程的程序，所以直接发送是被信号处理的默认行为捕获，并且kill信号发送给进程组，又因为fork后的进程组被修改为单独一个进程一个进程组并且二者id相同，所以他只会发送给单独一个子进程不会影响主进程。
2. 主进程下的内置发送sigint信号被处理了在调用是什么情况？
答：书上写，内核会默认阻塞任何当前处理程序正在处理信号类型的待处理信号。但是经过实验，在这个实验貌似并不会，从而通过kill会再次触发sigint_handler进行类递归调用，这也是我没有想明白的事情。
```c

void sigint_handler(int sig) 
{
    //结束前台进程。
    /*信号处理程序与进程共享全局变量，所以要对errno进行保存*/
    int old_errno = errno;
    pid_t pid = fgpid(jobs);
    if(pid != 0){
        kill(-pid, sig);
    }
    errno = old_errno;
    return;
}
```
与sigint_handler类似
```c
void sigtstp_handler(int sig) 
{
    int old_errno = errno;
    pid_t pid = fgpid(jobs);
    if(pid!=0){
        kill(-pid, sig);
    }
    errno = old_errno;
    return;
}
```
包装函数：
```c
pid_t Fork(void){
    pid_t pid;
    if((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}
void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if (execve(filename, argv, envp) < 0) {
    	printf("%s: Command not found\n", argv[0]);
        exit(0);
    }
}

pid_t Wait(int *status)
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait error");
    return pid;
}

pid_t Waitpid(pid_t pid, int *iptr, int options)
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0)
        if (errno != ECHILD)
	       unix_error("Waitpid error");
    return(retpid);
}

void Kill(pid_t pid, int signum)
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill error");
}

void Setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("Setpgid error");
    return;
}

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
	unix_error("Sigprocmask error");
    return;
}

void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
	unix_error("Sigemptyset error");
    return;
}

void Sigfillset(sigset_t *set)
{
    if (sigfillset(set) < 0)
	unix_error("Sigfillset error");
    return;
}

void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
	unix_error("Sigaddset error");
    return;
}

void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("Sigdelset error");
    return;
}

int Sigismember(const sigset_t *set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) < 0)
	unix_error("Sigismember error");
    return rc;
}

int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* always returns -1 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}

```
## 收获：
通过写这个lab，进步还是很快的，非常有利于了解Exceptional Control Flow，从一开始的不知道写啥-》看书-》抄书-》看ppt-》看pdf-》改-》找资料看别人的博客，代码-》读懂每一步为什么这么干，为什么不能去掉，为什么不能换成xx等等，使我对Exceptional Control Flow的理解更进一步。
## 建议：
看ppt，和pdf，看完要求之后看代码，翻书。有问题先翻书在看pdf在从ppt中找答案，实在找不到再去看技术博客。一下使我参考的关于这个lab的博客。
## 参考答案：
1. https://zhuanlan.zhihu.com/p/119034923
2. https://github.com/BlackDragonF/CSAPPLabs.git