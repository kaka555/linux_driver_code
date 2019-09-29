# linux_driver_code
define connect
#   target remote /dev/ttyS0
    b panic
    b sys_sync
end

define find_task
    if ((unsigned)$arg0 > (unsigned)&end)
        set $t=(struct task_struct *)$arg0
    else
        set $t=&init_task
        if (init_task.pid != (unsigned)$arg0)
            find_next_task $t
            while (&init_task!=$t && $t->pid != (unsigned)$arg0)
                find_next_task $t
            end
            if ($t == &init_task)
                printf "couldn't find task; using init_task\n"
            end
        end
    end
    printf "task \"%s\":\n", $t->comm
end

set nocp
set ru
syntax on
set number
set tabstop=4
set autoindent
set shiftwidth=4
filetype on
set showmatch
set matchtime=5
set cindent
let Tlist_Ctags_Cmd="/usr/local/bin/ctags"
map <F1> <Esc>:TlistToggle<Cr>
set tags=tags;
set autochdir
nmap <F5> :!find . -iname '*.c' -o -iname '*.cpp' -o -iname '*.h' -o -iname '*.hpp' > cscope.files<CR>
                        \ :!cscope -b -i cscope.files -f cscope.out<CR>
                                                \ :cs reset<CR>
set hlsearch
set tags=/my/lizhe/kernel-4.4/tags
cs add /my/kernel-4.4/cscope.out /my/kernel-4.4/

nmap <C-\>s :cs find s <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>g :cs find g <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>c :cs find c <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>t :cs find t <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>e :cs find e <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>f :cs find f <C-R>=expand("<cfile>")<CR><CR>
nmap <C-\>i :cs find i ^<C-R>=expand("<cfile>")<CR>$<CR>
nmap <C-\>d :cs find d <C-R>=expand("<cword>")<CR><CR>
    
    
#通知编译器我们要编译模块的哪些源码
#这里是编译itop4412_hello.c这个文件编译成中间文件itop4412_hello.o
obj-m += gpios.o 

#源码目录变量，这里用户需要根据实际情况选择路径
#作者是将Linux的源码拷贝到目录/home/topeet/android4.0下并解压的
KDIR := /home/topeet/android4.0/iTop4412_Kernel_3.0

#当前目录变量
PWD ?= $(shell pwd)

#make命名默认寻找第一个目标
#make -C就是指调用执行的路径
#$(KDIR)Linux源码目录，作者这里指的是/home/topeet/android4.0/iTop4412_Kernel_3.0
#$(PWD)当前目录变量
#modules要执行的操作
all:
    make -C $(KDIR) M=$(PWD) modules
        
#make clean执行的操作是删除后缀为o的文件
clean:
    rm -rf *.o


https://blog.csdn.net/msccreater/article/details/9998255
~/.bash_profile
# .bashrc

# User specific aliases and functions

alias rm='rm -i'
alias cp='cp -i'
alias mv='mv -i'

PS1="\[\e[36;1m\]\u:\[\e[33;1m\]\w \[\$\e[m\] "

# Source global definitions
if [ -f /etc/bashrc ]; then
    . /etc/bashrc
fi
