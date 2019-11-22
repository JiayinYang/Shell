sshell : sshell.o
	gcc -Wall -Werror -g -o sshell sshell.o

sshell.o : sshell.c
	gcc -Wall -Werror -g -c sshell.c

clean :
	rm sshell sshell.o
