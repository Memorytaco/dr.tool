SHELL=/bin/zsh
dtool: dtool.c main.c cmd.o disas.o file.o elf.o Makefile *.h
	${CC} -Wall -std=c17 main.c cmd.o file.o disas.o elf.o -lcapstone -o dtool -Icapstone/include

cmd.o: cmd.c file.o

file.o:
	make -C util
	cp util/*.o .

clean:
	@rm -f dtool *.o
	make -C util clean
