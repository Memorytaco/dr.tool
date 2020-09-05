SHELL=/bin/zsh
dtool: dtool.c main.c cmd.c
	@clang main.c dtool.c cmd.c -Lcapstone -lcapstone -o dtool -Icapstone/include

clean:
	@rm -f dtool
