set verbose on
dir ./lib-src/glibc-2.24/posix/
set follow-fork-mode child

#set args --workers=1 0 exec --tty python program.py
set args --workers=1 0 proxy localhost:10080

#run
