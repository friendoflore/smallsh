/******************************************************************************
 ** Filename: smallsh.c
 ** Author: Tim Robinson
 ** Date created: 7/27/2015
 ** Last modification date: 8/3/15 3:31 PM PST
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>


// These are used to track the currently running processes. These will be used
// to cleanup all child processes upon the user exiting the shell.
pid_t * opened_procs;
int proc_idx = 0;

/******************************************************************************
 ** Function: process_command
 ** Description: This function parses the user's command line input. If a
 **	function is a background process, the appropriate variables are set to 
 ** 	ensure the correct processing. This functions returns the number of 
 ** 	arguments of the command and fills an array of arguments with the command 
 ** 	line arguments	to be used upon returning. Upon finding a '#' character,
 **	the rest of the user's input is considered a comment and unprocessed.
 ** Parameters: char * newargv[], char * inputString, int * background. The 
 **	first argument is the empty array of arguments to be filled if necessary,
 **	the second is the user's input in the command line, the third is a 
 **	pointer that is used to track whether these arguments will run in the
 **	background or foreground.
 ** Preconditions: The argument array must be initialized with NULL values
 ** Postconditions: This function parses the user's command line arguments into
 **	an array. This function returns the number of arguments entered by the
 **	user.
 *****************************************************************************/

int process_command(char *newargv[], char * inputString, int *background) {
	
	// This is the number of command line arguments provided
	int newargc = 1;


	// This will help build the parsed commands as we iterate through the input
	// string
	char *tmpString = malloc(sizeof(char *) * 20);
	
	// This will store the argument number we are currently parsing, helping to
	// enter arguments into the argument array from the input string
	int newargv_step = 0;

	// These are iterators
	int i = 0;
	int j = 0;

	// Iterate through the entire input string
	for(i = 0; i < strlen(inputString); i++) {

		// If we encounter a '#' character, stop iterating through the string;
		// from that place forward is a comment
		if(inputString[i] == '#') {

			// Do not consider the comment a command, reduce the number of 
			// arguments by 1 (we start with the assumption of 1 command)
			newargc = newargc - 1;
			break;
		}

		// If we encounter a '&' character, that means we will be running this
		// process in the background. Set the appropriate variable.
		if(inputString[i] == '&') {

			// Do not consider this character a command, reduce the number of 
			// arguments by 1 (we start with the assumption of 1 command).
			newargc = newargc - 1;

			// This variable will be used later to determine how to run the child
			// process.
			*background = 1;

			// Do not iterate beyond the '&' character.
			break;
		}

		// If we encounter whitespace, we assume that another argument has been
		// encountered.
		if(inputString[i] == ' ') {
			newargc = newargc + 1;
		} else {

			// Clear the memory and allocate the contents of the helper string.
			tmpString = malloc(sizeof(char *) * 20);

			// Set the 'j' iterator to 0 to be used to parse each argument.
			j = 0;

			// If there are any characters remaining upon encountering a non-
			// whitespace character
			if((i + j) < strlen(inputString)) {

				while(1) {

					// If we encounter whitespace or a '#' character, stop parsing
					// this argument
					if(inputString[i + j] == ' ') { break; }
					if(inputString[i + j] == '#') { break; }

					// Add the character to the temporary build of the current
					// argument
					tmpString[j] = inputString[i + j];

					j = j + 1;

					// If we reach the end of the input string, stop parsing
					// that argument and make sure the helper string ends with a
					// '\0' character
					if((i + j) > strlen(inputString)) {
						tmpString[j - 1] = '\0';
						break;
					}
				}

				// This is used to ensure that a newline character is not included
				// in the argument array.
				char *no_new_line;
				if((no_new_line=strchr(tmpString, '\n')) != NULL) {
					*no_new_line = '\0';
				}

				// Terminate the argument with the '\0' character
				tmpString[j] = '\0';

				// Add this string as an argument to the argument array
				newargv[newargv_step] = tmpString;
				newargv_step = newargv_step + 1;

				// Continue iterating through the string starting after the
				// currently processed argument.
				i = i + j - 1;
			}
		}
	}
	return newargc;
}

/******************************************************************************
 ** Function: _userexit
 ** Description: This function kills all processes running in the shell and 
 **	causes the shell to exit.
 ** Parameters: None
 ** Preconditions: None
 ** Postconditions: All child processes are killed with a SIGKILL and the 
 **	program exits.
 *****************************************************************************/

void _userexit() {
	int i;

	// Iterate through the global array of running processes, kill each process
	for(i = 0; i < proc_idx; i++) {
		printf("killing process: %d\n", opened_procs[i]);
		kill(opened_procs[i], SIGKILL);
	}

	printf("Quit!\n");

	exit(0);
}

/******************************************************************************
 ** Function: _userstatus
 ** Description: This function formats the error status and prints the exit
 **	value of the previous process completed in the shell.
 ** Parameters: int status, the variable that stores the exit value of the 
 **	previous process completed.
 ** Preconditions: None
 ** Postconditions: This function terminates with the exit value printed to the
 **	screen. If the previous command was terminated by a signal, that signal
 **	is printed to the screen.
 *****************************************************************************/

void _userstatus(int status) {

	// Process a exit value of 256 as and exit value of 1
	if(status == 256) {
		status = 1;
	}

	// If the status is greater than 1, a signal was received; print that
	// termination signal. If not, print either 0 or 1 as the exit value.
	if(status > 1) {
		printf("terminated by signal %d\n", status);
	} else {
		printf("exit value %d\n", status);
	}
}

/******************************************************************************
 ** Function: _check_user_args
 ** Description: This function checks to see if the user gave a built-in
 **	function as a command. If the user entered a built-in function, the
 **	appropriate function is called to complete that function without creating
 **	a child process to run that command.
 ** Parameters: char ** user_arg, int * status. The first parameter is the
 **	array storing the parsed user's command line input, the second is a 
 **	pointer to the variable storing the exit value of the previous command.
 ** Preconditions: None
 ** Postconditions: If the user entered a built-in command, this function 
 **	returns having completed that command and returns a 1. If the user did
 **	not enter a built-in command, this function returns a 0. This funciton
 **	also returns a 1 if there are no arguments given.
 *****************************************************************************/

int _check_user_args(char **user_arg, int *status) {

	// Return a 1 if the first argument is NULL, which is to say that there are
	// no arguments provided.
	if(user_arg[0] == NULL) {
		*status = 0;
		return 1;
	}

	// If the first argument is "exit", call the _userexit function.
	if(strcmp(user_arg[0], "exit") == 0) {
		_userexit();
	}

	// If the first argument is "status", call the _userstatus function and set
	// the status variable to 0.
	if(strcmp(user_arg[0], "status") == 0) {
		_userstatus(*status);
		*status = 0;
		return 1;
	}

	// If the first argument is "cd", call the appropriate function to change
	// directories and, if successful, set the status variable to 0.
	if(strcmp(user_arg[0], "cd") == 0) {

		// If no directory is provided, change directory to the home directory.
		if(user_arg[1] == NULL) {
			printf("Changing to home directory...\n");
			chdir(getenv("HOME"));
			*status = 0;
		} else {

			// Change the directory; if unsuccessful, print the error message and
			// set the status variable to 1.
			if(chdir(user_arg[1])) {
				printf("No directory found with name %s\n", user_arg[1]);
				*status = 1;
			} else {
				*status = 0;
			}
		}
		return 1;
	}
	return 0;
}

/******************************************************************************
 ** Function: _check_background_procs
 ** Description: This function checks to see if any processes running in the 
 **	background have terminated. If any have terminated, the process 
 **	number is printed to the screen. This function also calls the function
 **	to print the exit status of the child process as well as the termination
 **	signal if the child process was terminated by signal.
 ** Parameters: pid_t cpid, int * status. The first parameter is the variable
 **	that will store any child process number that ended, the second is a 
 **	pointer to the variable storing the exit value of the previous command.
 ** Preconditions: None
 ** Postconditions: If a child process has terminated, this function removes
 **	that process number from the global array of running processes and prints
 **	either the exit value or termination signal of that process.
 *****************************************************************************/

void _check_background_procs(pid_t cpid, int *status) {
	int iter;
	int iter2;

	// Check to see if any processes have completed but do not wait for them if
	// they have not completed.
	cpid = waitpid(-1, status, WNOHANG);

	// cpid will be positive if any process has completed, populated with the
	// process number of the completed process.
	if(cpid > 0) {
		printf("background pid %d is done: ", cpid);

		// Iterate through the global array of running processes. Remove the 
		// completed process number from the array.
		for(iter = 0; iter < proc_idx; iter++) {
			if(opened_procs[iter] == cpid) {
				for(iter2 = iter + 1; iter2 < proc_idx; iter2++) {
					opened_procs[iter] = opened_procs[iter2];
				}
				proc_idx = proc_idx - 1;
				break;
			}
		}

		// If the process exited, call the function to print the status.
		if(WIFEXITED(*status)) {
			_userstatus(*status);

		// If the process was terminated, print the signal that terminated the
		// process.
		} else if(WIFSIGNALED(*status)) {
			printf("terminated by signal %d\n", WTERMSIG(*status));
		}
	}
}

/******************************************************************************
 ** Function: _check_user_input
 ** Description: This function validates the user's command line input. If the 
 **	input is greater than 2048 characters, that command is considered to long
 **	and the function returns a 1. If the user's command line input is empty,
 **	this function also returns a 1.
 ** Parameters: char * input, int * status. The first parameter contains the
 **	user's command line input, the second is a pointer to the variable 
 **	storing the exit value of the previous command.
 ** Preconditions: None
 ** Postconditions: The function returns a 1 if the user's command is too short
 **	(nothing) or too long (greater than 2048 characters), otherwise this 
 **	function returns a 0.
 *****************************************************************************/

int _check_user_input(char * input, int *status) {
	if(strlen(input) > 2048) {
		printf("That command is too long!\n");
		*status = 1;
		return 1;
	}

	if(strlen(input) == 0) { return 1; }

	// Return 0 if the user input's size is valid.
	return 0;
}


int main(int argc, char*argv[]) {
	printf("smallsh waiting for command...\n");

	// Allocate memory for the global array storing the running processes.
	opened_procs = malloc(sizeof(pid_t) * 16);

	// I used Benjamin Brewster's method of signal handling so that SIGINT only
	// terminates child processes running in the foreground.
	struct sigaction sa;

	// Ignore signals given in the shell process.
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_RESTART;

	if(sigaction(SIGINT, &sa, NULL) == -1) {
		perror("server sigaction failure");
		exit(1);
	}


	while(1) {

		// This will be used in the getline function.
		size_t input_limit = 2400;

		// This will store the command line argument
		char * inputString = malloc(sizeof(char * ) * 2400);
		
		// This will store the exit value of completed processes.
		int status;

		// This will be a 1 if the current process is to run in the background.
		int background_flag = 0;

		// This will store a 0 for a child process and the pid of a child process
		// for the parent process.
		pid_t spawnpid = -5;

		// This is used to store a child's process ID if a child process 
		// completes in the background.
		pid_t cpid = -5;

		// This is used to correctly fork processes given file input or output.
		int dont_process_further = 1;

		// These will store file descriptors if the user gives a '>' or '<'
		// character as an argument.
		int fd_in = -1;
		int fd_out = -1;

		// These are iterators.
		int i;
		int k;
		int iter;
		int iter2;

		fflush(stdout);

		// Check to see if any background processes have completed.
		_check_background_procs(cpid, &status);
		
		// Get user input from the command line of the shell.
		printf(" : ");
		getline(&inputString, &input_limit, stdin);

		// Validate the length of user input. Get user input again if the input
		// is not valid.
		if(_check_user_input(inputString, &status)) { continue; }

		// This array will store arguments given by the command line.
		char * shargv[512];

		// Initialize the argument array with NULL values.
		for(i = 0; i < 512; i++) {
			shargv[i] = NULL;
		}

		// This function parses the user input into the shargv array and returns
		// the number of arguments provided by the user.
		int shargc = process_command(shargv, inputString, &background_flag);

		// If the user provided more than 512 arguments, print the error message
		// and get the user input again.
		if(shargc > 512) {
			printf("That is too many arguments!\n");
			status = 1;
			continue;
		}

		// This function checks to see if user input involved built-in commands,
		// calls them if so, and gets user input again.
		if(_check_user_args(shargv, &status)) { continue; }

		// This checks to see if the user provided '>' or '<' characters as
		// arguments.
		for(k = 0; k < shargc; k++) {

			// If the user wants to open a file for writing...
			if(strcmp(shargv[k], ">") == 0) {

				// Open the file by the name given by the argument after the '>'
				// character.
				fd_out = open(shargv[k + 1], O_WRONLY|O_CREAT|O_TRUNC, 0644);

				// This allows us to run the appropriate execution of functions
				// which are different when opening files for writing.
				dont_process_further = 0;

				spawnpid = fork();

				// Child process
				if(spawnpid == 0) {

					// If the file descriptor was not changed, an error has occurred.
					// Set the exit value to 1 and exit the child process.
					if(fd_out == -1) {
						printf("smallsh: cannot open %s for output\n", shargv[k + 1]);
						status = 1;
						exit(1);
					}

					// Point the standard output to the file given by the user.
					fd_out = dup2(fd_out, 1);

					// If the '>' was given as the first argument, do not execute a
					// function, just exit the child process with the file provided
					// as emptied.
					if(k == 0) {
						exit(0);
					} else {

						// Execute the argument before the '>' character using the 
						// file provided by the argument after the '>' character.
						execlp(shargv[k - 1], shargv[k + 1], NULL);
						exit(1);
					}

				// Parent process
				} else if(spawnpid > 0) {

					// Writing to a file will run in the foreground; wait for the
					// child process to complete.
					cpid = waitpid(spawnpid, &status, 0);
				
				// There was an error in forking the child process.
				} else {
					perror("fork for writing failure!\n");
					status = 1;
				}

			// If the user wants to open a file for reading...
			} else if(strcmp(shargv[k], "<") == 0) {

				// This allows us to run the appropriate execution of functions
				// which are different when opening files for reading.
				dont_process_further = 0;
				
				// Open the file by the name given by the argument after the '<'
				// character.				
				fd_in = open(shargv[k + 1], O_RDONLY);	

				spawnpid = fork();

				// Child process
				if(spawnpid == 0) {

					// If the file descriptor was not changed, an error has occurred.
					// Set the exit value to 1 and exit the child process.					
					if(fd_in == -1) {
						printf("smallsh: cannot open %s for input\n", shargv[k + 1]);
						status = 1;
						exit(1);
					}
					
					// Point the standard input to the file given by the user.					
					fd_in = dup2(fd_in, 0);

					// If the '<' character is the first argument, do nothing.
					if(k == 0) {
						exit(0);
					} else {

						// Execute the argument before the '<' character using the 
						// file provided by the argument after the '<' character.						
						execlp(shargv[k - 1], shargv[k + 1], NULL);
						exit(1);
					}

				} else if(spawnpid > 0) {
					// Reading from a file will run in the foreground; wait for the
					// child process to complete.
					cpid = waitpid(spawnpid, &status, 0);

				// There was an error in forking the child process.
				} else {
					perror("fork for reading failure!\n");
					status = 1;
				}
			}
		}

		// If the user's argument did not include writing to or reading from a
		// file...
		if(dont_process_further) {
			
			spawnpid = fork();
	
			// Child process
			if(spawnpid == 0) {

				// If this process is running in the foreground, allow normal
				// signal handling.
				if(background_flag == 0) {
					sa.sa_handler = SIG_DFL;
					sigaction(SIGINT, &sa, NULL);
				}

				// Execute the arguments provided by the user.
				execvp(shargv[0], shargv);
				exit(1);

			// Parent process
			} else if(spawnpid > 0) {

				// If the arguments provided are to run in the background...
				if(background_flag == 1) {

					// Add the background child's process ID to the global array of
					// currently running processes and increase the index for future
					// background process IDs.
					opened_procs[proc_idx] = spawnpid;
					proc_idx = proc_idx + 1;

					// Print the background process ID.
					printf("background pid is %d\n", spawnpid);

				// The arguments provided are to run in the foreground...
				} else {

					// Wait for the process to complete.
					cpid = waitpid(spawnpid, &status, 0);

					// If an invalid argument is given as the only argument, print
					// the appropriate error message.
					if((status == 256) && (shargc == 1)) {
						printf("%s: no such file or directory\n", shargv[0]);
					}
				}

			// There was an error in forking the child process.		
			} else {
				perror("fork() failure!\n");
			}
		}

		// Close the files opened by the user.
		close(fd_in);
		close(fd_out);
	}

	return 0;
}