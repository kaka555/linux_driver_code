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
cs add /my/kernel-4.4/cscope.out /my/kernel-4.4/

nmap <C-\>s :cs find s <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>g :cs find g <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>c :cs find c <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>t :cs find t <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>e :cs find e <C-R>=expand("<cword>")<CR><CR>
nmap <C-\>f :cs find f <C-R>=expand("<cfile>")<CR><CR>
nmap <C-\>i :cs find i ^<C-R>=expand("<cfile>")<CR>$<CR>
nmap <C-\>d :cs find d <C-R>=expand("<cword>")<CR><CR>
