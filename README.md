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
