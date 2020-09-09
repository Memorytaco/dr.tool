SHELL=/bin/zsh
dtool: dtool.c main.c cmd.c disas.o file.o Makefile
	@clang main.c cmd.c file.o disas.o -lcapstone -o dtool -Icapstone/include

cmd.o: cmd.c file.o

file.o:
	make -C util
	cp util/*.o .

clean:
	@rm -f dtool *.o
	make -C util clean
