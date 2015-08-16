I. File list
------------
smallsh.c			Shell implementation
readme.txt			This file


II. Program Instructions
------------------------

A. Compiling and running the shell program

This program only requires the compilation of one file, i.e. the file by the 
name of "smallsh.c". The object file name I will reference throughout this
document and used during testing goes by the name "smallsh". In order to 
compile this program, use a bash shell and enter the following (throughout 
this document, ignore all '"' characters for entries into the command line,
both in the bash shell and in the shell program):

	"gcc -o <object filename> smallsh.c"

So during testing I used the exact command:

	"gcc -o smallsh smallsh.c"

The shell program is then run using the object file by entering the following
in a bash shell:

	"./smallsh"

This starts the shell which will await user commands to be given. The user is
prompted with only " : " given as a prompt.

B. Argument structure and limitations

In this document, all user input separated by whitespace is considered an
"argument". This will be qualified below in II.C, where we'll see that '#' and
'&' are considered special characters (not arguments) to be handled as 
specified below. Thus user input can take the form of the following:

	"[arg1 arg2 ...] [< input_file] [> output_file] [&]"

User input is limited to 2048 characters and a total of 512 arguments 
(including '<', '>', and file names). Upon entering a command falling within
the size constraints, the shell will run the program as specified and return to
the shell to accept further user commands.


C. Built-in functions and special characters

There are three built in commands in this shell program, namely the "exit",
"cd", and "status" commands. There are two special characters recognized by the
program, namely the '#' character and the '&' character.

The "exit" command terminates all processes currently running that were created
in the shell and exits the shell program.

The "cd" command allows a user to change directories. The "cd" command takes
the form of the following:

	"cd [child directory | directory path]"

If no directory or directory path is provided, the program redirects the user
to the home directory of the user. If the directory provided is not valid, an
error is given and calling the "status" command will show an exit value of 1.
If a valid directory or directory path is provided, the user's working
directory is changed and the "status" command will show an exit value of 0.

The "status" command prints the exit value of the last completed process. If
that process was terminated by a signal, the signal number that terminated it
is printed to the screen. An exit value of 0 means that the previous process
was successful; an exit value of 1 means that previous process was 
unsuccessful.

The '#' character denotes that all characters following are to be considered a
comment and will not be considered arguments to the shell program. The '#'
character can be used to denote a whole line as a comment or a partial line as
a comment, all depending on the '#' characters placement within the user's
command.

The '&' character is only to be used as the final character in a command. The
'&' character denotes that the current command is to be run as a background
process and thus the shell will not wait for the processes created by that
command to terminate before allowing further user input. If the '&' character
is not provided as the final character of a command, the process created by
that command will cause the shell to wait for that process to finish before
allowing further user input.


D. File manipulation

If the user provides either '>' or '<' as arguments, the shell program will
open the specified output or input file (respectively) and complete the command
provided by the user.

In order to read from a file, the user must provide an appropriate argument
that will read from the specified file. If an argument is not provided, nothing
happens, though the exit value is still 0, as no error occurred.

In order to write to a file, the user must provide either an appropriate
argument that will write to the specified file, or use no leading arguments
or characters before the '>' character. In the first case, the shell program 
will execute the specified function and write its output to the specified file.
In the second case, the shell program will open the file for writing, truncate
all of the contents (thus emptying the file) and close the file. In either 
case the exit value of the command is 0.

In either case, if the specified file does not exist, the shell creates that
file and carries on with its action as specified above.


E. Signal handling and exiting the shell program

Only signals given while running a foreground process are acknowledged and 
delivered. Signals given to the shell program, with or without any background 
processes currently running, are ignored. If a process is explicitly killed, 
say using the kill command, the status function will return the signal number
that terminated that process. If a process running in the foreground is sent, 
say, a SIGINT signal, the status function will likewise return the signal 
number that terminated that process.

The only defined way of exiting the shell program internally is by use of the
"exit" command. Using the "exit" command will cause the shell to terminate all
processes created by the shell currently running and exit the program.


III. References
---------------

A. https://oregonstate.instructure.com/courses/1524722/discussion_topics/7567864
	(login required)
	Post by user Benjamin Brewster on 7/29/2015 at 10:31 AM PST

	1. This resource was used as a reference to understand the function of 
	ignoring and reacknowledging of signals.

	2. This resource was used to implement the ignoring of signals while 
	the user is in the shell, with or without background processes running, and
	acknowledging signals while the user is in a running foreground process.
