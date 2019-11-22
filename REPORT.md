# Program Overview

We get the user's input and divide it up into three parts.
	-Part1: input command along with any options ie "ls -a -l"
	-Part2: Input redirection ie "<" sets in[k]
	-Part3: Output redirection ie ">" sets out[k]

We process the parts to remove any excess spaces and also split up part one
into seperate argument strings.  We search for a misplaced background symbol
and immediately return to the command prompt if it is incorrectly placed.
We use multiple copies of the input to allow us to perform
destructive operations on the input such as strtoking and removing the &.

Now we fork and pipe a communication link in order to allow the child to
execute mulitiple piped commands and to return all of their exit statuses
to the parent as a single string.

In the child, we check flags in[k] and out[k] and compare them to their
locations in the piping of commands.  This ensures that only the first
piped command can have "<" and only the last can have ">".  An error will
immediately terminate.  We then check if we are dealing with a change of
directory, a print working directory or an exit so that we don't continue any
more piped commands.

Now we determine if we have only one command.  If so, we handle that
as a special case because we won't be piping.  Otherwise, we have piped commands
so we enter a for loop and create a pipe of communication before forking again.
The original child now has its own child to execute the commands one at a time.
The return statuses are appended to an array.  Once all commands except the last
one have been executed, the array is sent to the parent and the final command
is executed.

Now the original child has terminated and we are returned to the parent.
The parent's first question is if the command the user entered is supposed
to run in the background.  If it is, then the parent wants to return to
the command line but first it checks if any other background processes
have finished and need to be completed.  If the user hadn't run the command
in the background, then the parent is stuck and will remain in a while loop
until that child is done.  Normally it would be idle and not accomplishing
anything but instead it now spends its time checking the background
processes.  If one of them finishes while the parent is waiting, it will
take care of it.  The parent will only return to the command prompt once
its non background child completes.

# Project Testing

Primarily, we used the test cases outlined in Project1.html and compared
our outputs to sshell_ref.  We also used the testing scripts as well as our
own custom tests based on the Project1.html tests.

# Current Limitations

The processes in background feature is not fully functional.

# Sources

03.syscalls.pdf Copyright © 2017 Joël Porquet
http://man7.org/linux/man-pages/
https://www.gnu.org/software/libc/manual/
https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
http://
stackoverflow.com/questions/8082932/connecting-n-commands-with-pipes-in-a-shell
http://pubs.opengroup.org/onlinepubs/009695399/functions/open.html
https://www.tutorialspoint.com/c_standard_library/
https://linux.die.net/man/
https://
support.sas.com/documentation/onlinedoc/sasc/doc700/html/lr2/zid-6908.htm
