set verbose on
dir ./lib-src/glibc-2.24/posix/

#set args --workers=1 0 exec --tty python program.py
#set args --workers=1 0 proxy localhost:10080
set args --workers=1 0 exec sh

set follow-fork-mode child

break handle_request
commands 1
set follow-fork-mode parent
continue
end

break pass_traffic 
run
