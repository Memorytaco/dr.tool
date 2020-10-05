SHELL= /bin/zsh
MYLIB=../vender/capstone/libcapstone.a
dtool: dtool.c main.c cmd.o disas.o file.o elf.o Makefile *.h capstone
	${CC} -Wall -std=c17 main.c cmd.o file.o disas.o elf.o ${MYLIB} -o dtool -isystem ../vender/capstone/include

cmd.o: cmd.c file.o

capstone:
	make -C ../vender/capstone/

file.o:
	make -C util
	cp util/*.o .

clean:
	@rm -f dtool *.o
	make -C util clean
