SHELL=/bin/zsh
dtool:
	@clang dtool.c -l capstone -o dtool

clean:
	@rm *.o
