compiler = cc
cflags = -Wall -Wextra -Werror
libs = -lcurses
objects = 8085_main.o curses_ui.o
installpath = /usr/bin

dirty : $(objects)
	@$(compiler) -o dirty $(objects) $(libs) $(cflags)
	@echo -e "success\nas superuser type 'make install' to install\nor run dirty from current directory"

8085_main.o : 8085_main.c 8085.h
	@$(compiler) 8085_main.c -c $(cflags) -o 8085_main.o
curses_ui.o : curses_ui.c 8085.h
	@$(compiler) curses_ui.c -c $(cflags) -o curses_ui.o

.PHONY : clean install remove cleanbak
clean : 
	@rm dirty $(objects)
	@echo "cleaned"
install :
	@cp -f dirty $(installpath)
	@echo -e "installed in "$(installpath) "\ntype dirty in the terminal to run" 
remove :
	@rm -f /usr/bin/dirty
	@echo "removed from "$(installpath)
cleanbak :
	@rm -f rm *~
	@echo "backup files cleaned"