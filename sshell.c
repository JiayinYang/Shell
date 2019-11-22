#define TRUE 1
#define FALSE 0
#define CMDLINE_MAX 512
#define PIPE_MAX 256
#define ARGS_MAX 256
#define BACKGROUND_PROCESSES_MAX 256

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void completeProcess(int exitStatus, int n, char* sacrifice,
		char* beforeSacrifice, char* output, int nn, int* fdX,
		char* buf, int j, char** backgroundProcessFilename,
		int backgroundArrayIndex, char* filecopy);

int checkSleepers(int* sleepingProcess, int waitpidStatus, int childStatus,
		int n, char* sacrifice, char* output, char* beforeSacrifice,
		int nn, int* fdX, char* buf, int j,
		char** backgroundProcessFilename, int backgroundArrayIndex,
		char* filecopy);

struct command{
	char** argsz; // saves argument between spaces
};

int main(int argc, char *argv[])
{
	int isMislocatedBackgroundSign; 
	char* buf = (char*)calloc(sizeof(char), CMDLINE_MAX);
	int fdX[2];	//allows communication for pipping
	int pidChild;
	int childStatus;
	int waitpidStatus;
	size_t n = CMDLINE_MAX;
	int nn = CMDLINE_MAX;
	int isShellActive = TRUE;
	struct command **cmd = (struct command**)calloc(sizeof(struct command*),
			PIPE_MAX);
	for(int i = 0; i < ARGS_MAX; i++)
		cmd[i] = (struct command*)calloc(sizeof(struct command),
				ARGS_MAX);

	//These are four copies of input that are changed during
	//the process of strtoking
	char* output = (char*)calloc(sizeof(char), n);
	char* tokePipe = (char*)calloc(sizeof(char), n);
	char* beforeSacrifice = (char*)calloc(sizeof(char), n);
	char* tokeRedirection = (char*)calloc(sizeof(char), n);

	//used exclusively for printing background filename with &
	char* filecopy = (char*)calloc(sizeof(char), n);
	char* filePipe = (char*)calloc(sizeof(char), n);

	// after we toke < and >, the input is divided into three parts
	char** part1 = (char**)calloc(sizeof(char*), ARGS_MAX);
	char** part2 = (char**)calloc(sizeof(char*), ARGS_MAX);
	char** part3 = (char**)calloc(sizeof(char*), ARGS_MAX);

	//flags set if "<" or ">" in input
	int* in = (int*)calloc(sizeof(int), ARGS_MAX); 
	int* out = (int*)calloc(sizeof(int), ARGS_MAX);

	//a temporary string to retain integrity of input during strtok
	char* sacrifice = (char*)calloc(sizeof(char), CMDLINE_MAX); 

	//contains all return statuses during pipelining
	char* retStatusChar = (char*)calloc(sizeof(char), PIPE_MAX); 
	int isBackground;	//flag set if "&" at end of input
	int backgroundArrayIndex; //set if a sleeping process finally terminates

	//tracks active background processes
	int* sleepingProcess = (int*)calloc(sizeof(int),
			BACKGROUND_PROCESSES_MAX);

	//tracks the command entered for background processes
	char** backgroundProcessFilename = (char**)calloc(sizeof(char*),
			CMDLINE_MAX);

	for(int i = 0; i < CMDLINE_MAX; i++)
		backgroundProcessFilename[i] = (char*)calloc(sizeof(char),
				CMDLINE_MAX);
	
	//main user input
	char input[CMDLINE_MAX];

	for (int i = 0; i < ARGS_MAX; i++) {
		cmd[i]->argsz = (char**)calloc(sizeof(char*), n);
		for(int ii = 0; ii < n; ii++)
			cmd[i]->argsz[ii] = (char*)calloc(sizeof(char), n);
	}

	for (int i = 0; i < BACKGROUND_PROCESSES_MAX; i++) {
		sleepingProcess[i] = 0;
	}


	for (int i = 0; i < PIPE_MAX; i++) {
		part1[i] = (char*)calloc(sizeof(char), n);
		part2[i] = (char*)calloc(sizeof(char), n);
		part3[i] = (char*)calloc(sizeof(char), n);
	}


	do { //loops if muliple piped commands
		do { //loops if mislocated &
			isMislocatedBackgroundSign = FALSE;
			isBackground = FALSE;

			char *nl;

			printf("sshell$ ");

			fgets(input, CMDLINE_MAX, stdin);

			int length = strlen(input);
			int count = 0;
			int sleepProcessContinue = 0;
			
			// handles cases with only spaces and new line. If there is only
			// space and newline, we read from input again and printf $sshell.
			// If the background process should still continue, we set
			// sleepProcessContinue to 1 and break out of the loops.
			while(sleepProcessContinue == 0 && (strcmp(input, "\n") == 0 ||
									length == strlen(input))) {
				if(count != 0) {
					printf("sshell$ ");
					fgets(input, CMDLINE_MAX, stdin);
				} else if(count == 0 && strcmp(input, "\n") == 0) {
					for (int i = 0; i < 100; i++) {
						if (sleepingProcess[i] > 0) {
							sleepProcessContinue = 1;
							break;                        
						} //if				
					} //for



					printf("sshell$ ");
					fgets(input, CMDLINE_MAX, stdin);
				} //if

				count++;
				for(length = 0; length < strlen(input); length++) {
					if(input[length] != ' ' && input[length] != '\n')
						break;
				} //for
			} //while

			count = 0;
			int isvalid = 0;

			// handles cases with invalid < or >. If it is the invalid command
			// line, we print the error message and read from the input again.
			// If there is only new line and spaces, instead of printing the
			// error message, we just read from the input
			while(sleepProcessContinue == 0 && isvalid == 0)
			{
				for(length = 0; length < strlen(input); length++) {
					if(input[length] != ' ' &&
							input[length] != '\n' &&
							input[length] != '<' &&
							input[length] != '>') {
						isvalid = 1;
						break;
					} //if
				} //for
				if(count != 0 && length == strlen(input)) {
					count = 1;
					int j;
					for(j = 0; j < strlen(input); j++)
						if(input[j] != ' ' && input[j]!= '\n')
							break;
					if(j != strlen(input))
						fprintf(stderr, "Error: invalid command line\n");
					printf("sshell$ ");
					fgets(input, CMDLINE_MAX, stdin);
				} else if(isvalid == 0 && count == 0) {
					count = 1;
					int j;
					for(j = 0; j < strlen(input); j++)
						if(input[j] != ' ' && input[j]!= '\n')
							break;

					if(j != strlen(input))
						fprintf(stderr, "Error: invalid command line\n");
					
					printf("sshell$ ");
					fgets(input, CMDLINE_MAX, stdin);
				} //if
			} // while


			if (!isatty(STDIN_FILENO)) {
				printf("%s", input);
				fflush(stdout);
			}

			nl = strchr(input, '\n');
			if (nl)
				*nl = '\0';

			strcpy(filecopy, input);

			char* s;
			if ((s = strchr(input, '&')) != 0)
			{
				*s = '\0';
				s++;
				do {
					if (strcmp(s, "\0") == 0) {
						isBackground = TRUE;
						break;
					}
					if (strcmp(s, " ") != 0) {
						fprintf(stderr, "Error: mislocated background sign\n");
						isMislocatedBackgroundSign = TRUE;
						break;
					} //if
					s++;
				} while (1);
			} //if

		} while (isMislocatedBackgroundSign);	

		for (int i = 0; i < PIPE_MAX; i++) {
			free(part1[i]);
			free(part2[i]);
			free(part3[i]);
			part1[i] = (char*)calloc(sizeof(char), n);
			part2[i] = (char*)calloc(sizeof(char), n);
			part3[i] = (char*)calloc(sizeof(char), n);
			part1[i][0] = '\0';
			part2[i][0] = '\0';
			part3[i][0] = '\0';
		} //for

		strcpy(output, input);
		strcpy(beforeSacrifice, input);
		strcpy(tokeRedirection, input);
		for (int i = 0; i < 50; i++) {
			in[i] = 0;
			out[i] = 0;
		} //for

		for (int i = 0; i < ARGS_MAX; i++)
			for (int ii = 0; ii < ARGS_MAX; ii++)
				cmd[i]->argsz[ii] = NULL;

		//j represents number of commands seperated by pipes
		int j = 0;

		do {
			//filePipe will be sacrificed for tokens, input intact
			strcpy(filePipe, input);

			//need to be strtoked multiple times depending on # of loop
			tokePipe = strtok(filePipe, "|");

			for (int i = 0; i < j; i++)
				tokePipe = strtok(NULL, "|");

			if (tokePipe == NULL)
				break;

			strcpy(tokeRedirection, tokePipe);

			if (strchr(tokePipe, '<'))
				in[j] = 1;
			if (strchr(tokePipe, '>'))
				out[j] = 1;


			if (!in[j] && !out[j])
				strcpy(part1[j], tokePipe);
			else if (in[j] && !out[j] && tokePipe[0] != '<') {
				sacrifice = strtok(tokeRedirection, "<");
				strcpy(part1[j], sacrifice);
			} else if (!in[j] && out[j] && tokePipe[0] != '>') {
				sacrifice = strtok(tokeRedirection, ">");
				strcpy(part1[j], sacrifice);
			} else if (in[j] && out[j] && tokePipe[0] != '<') {
				sacrifice = strtok(tokeRedirection, "<");
				strcpy(part1[j], sacrifice);
			} //if

			if (out[j]) {
				//handles unusual case where nothing is before '>'
				if (tokePipe[0] != '>') {
					strtok(tokePipe, ">");
					sacrifice = strtok(NULL, ">");
					strcpy(part3[j], sacrifice);
				} else {
					sacrifice = strtok(tokePipe, ">");
					strcpy(part3[j], sacrifice);
				} //if
			} //if

			if (in[j]) {
				//handles unusual case where nothing is before '<'
				if (tokePipe[0] != '<') { 
					strtok(tokePipe, "<");
					sacrifice = strtok(NULL, "<");
					strcpy(part2[j], sacrifice);
				} else {
					sacrifice = strtok(tokePipe, "<");
					strcpy(part2[j], sacrifice);
				} //if
			} //if


			if (strlen(part1[j]) == 0)
				part1[j] = NULL;


			if (part1[j]) {
				int i = 0;

				cmd[j]->argsz[0] = strtok(part1[j], " ");
				do {
					cmd[j]->argsz[++i] = strtok(NULL, " ");
				} while (cmd[j]->argsz[i]);
			} //if

			if (part2[j][0] != '\0') {
				sacrifice = strtok(part2[j], " ");
				strcpy(part2[j], sacrifice);
			} //if

			if (part3[j][0] != '\0') {
				sacrifice = strtok(part3[j], " ");
				strcpy(part3[j], sacrifice);
			} //if

			j++;
		} while (1);

		//fdX allows communication during multiple commands piped
		pipe(fdX);

		j--;
		pidChild = fork();



		if (pidChild > 0) { //parent process running

			close (fdX[1]);

			if (isBackground) { //latest process is a background process
				if (strcmp(filecopy, "exit&") == 0) {
					fprintf(stderr, "Bye...\n");
					return 0;
				}
				for (int i = 0; i < 100; i++) {
					if (sleepingProcess[i] == 0 || sleepingProcess[i] == -1) {
						sleepingProcess[i] = pidChild;
						strcpy(backgroundProcessFilename[i], filecopy);
						break;
					}
				}
				if (checkSleepers(sleepingProcess, waitpidStatus, childStatus,
							n, sacrifice, output, beforeSacrifice, nn, fdX,
							buf, j, backgroundProcessFilename,
							backgroundArrayIndex, filecopy) == 0)
					return 0;

			} else { //latest process is not a background process

				while(1) { //staying here as long as a non sleeping child exists
					waitpidStatus = waitpid(pidChild, &childStatus, WNOHANG);

					if (waitpidStatus == -1) { //child has terminated
						if (WIFEXITED(childStatus) != 0) {//child success term
							if (WEXITSTATUS(childStatus) == 3) { //"exit"
								for (int i = 0; i < 100; i++) {
									if (sleepingProcess[i] > 0) {
										fprintf(stderr, "Error: active jobs still running\n");
										fprintf(stderr, "+ completed 'exit' [1]\n");
										break;
									} else if (sleepingProcess[i] == 0) {
										fprintf(stderr, "Bye...\n");
										return 0;
									}
								}
							} else {
								backgroundArrayIndex = -1;
								completeProcess(WEXITSTATUS(childStatus), n,
										sacrifice, beforeSacrifice,
										output, nn, fdX, buf, j,
										backgroundProcessFilename,
										backgroundArrayIndex, filecopy);
							}
							break;
						}
					} else { //check on sleeping children
						if (checkSleepers(sleepingProcess, waitpidStatus,
									childStatus, n, sacrifice, output,
									beforeSacrifice, nn, fdX, buf, j,
									backgroundProcessFilename,
									backgroundArrayIndex, filecopy) == 0) {

							return 0;
						}
					}
				}
			}
		} else if (pidChild == 0) { //child process running
			close(fdX[0]);

			int k = 0;

			do {
				int fdin, fdout;

				//kth command has an "<"
				if (in[k]) {
					if (k != 0)
						return 10;

					if (part2[k] == NULL) // "<" but no input file
						return 7;
					fdin = open(part2[k], O_RDONLY);
					if (fdin == -1) //problem with opening file
						return 6;
					dup2(fdin, STDIN_FILENO);
					close(fdin);
				}

				//kth command has an ">"
				if (out[k]) {
					if (k != j)
						return 11;

					if (part3[k] == NULL) // ">" but no output file
						return 9;

					fdout = open(part3[k], O_CREAT | O_WRONLY, 777);
					write(STDOUT_FILENO, part3[k], 512);
					dup2(fdout, STDOUT_FILENO);
					close(fdout);
				}

				//for "cd", "pwd" and "exit", must return to parent
				//immediately and let it handle
				if (strcmp(cmd[k]->argsz[0], "cd") == 0) {
					return 5; //"cd something something..i"
				} else if (strcmp(cmd[k]->argsz[0], "exit") == 0) {
					return 3; //"exit"
				} else if (strcmp(cmd[k]->argsz[0], "pwd") == 0) {
					return 12; //"pwd"
				}

				//k represents the kth command that is getting preprocessed
				//which means changing input and output if necessary as
				//well as short circuiting in the event of cd, pwd, or exit
				//so when k == j, we have finished preprocessing
				if (k == j) {
					if (j == 0)
						break;
					int status;
					int fd[2];
					int pChild;
					int waitpStatus;
					int inpipe = STDIN_FILENO;

					//i represents the ith command that is actually
					//getting executed by the child of the child
					for (int i = 0; i <= j - 1; i++) {
						pipe(fd);

						if ((pChild = fork()) == 0) { //child
							if (inpipe != 0) {
								dup2(inpipe, STDIN_FILENO);
								close(inpipe);
							}

							if (fd[1] != 1) {
								dup2(fd[1], STDOUT_FILENO);
								close(fd[1]);
							}
							execvp(cmd[i]->argsz[0], cmd[i]->argsz);
						}

						while(1) {
							waitpStatus = waitpid(pChild, &status, WNOHANG);
							if (waitpStatus == -1) { //child has terminated
								//child terminates successfully
								if (WIFEXITED(status) != 0) {
									retStatusChar[i] = WEXITSTATUS(status)
										+ '0';
									break;
								} //if
							} //if
						} //while


						close(fd[1]);
						inpipe = fd[0];
					}
					write(fdX[1], retStatusChar, 3 * sizeof(retStatusChar));

					if (inpipe != 0)
						dup2(inpipe,STDIN_FILENO);

					execvp(cmd[k]->argsz[0], cmd[k]->argsz);
				} //if
				k++;
			} while(k <= j);

			//exec() returns -1 if error and errno contains error
			if (j == 0)	
				if (execvp(cmd[k]->argsz[0], cmd[k]->argsz) == -1)
					return 4; //"command not found"
		} else if (pidChild < 0) { //process creation failed (memory?)
			//printf("process creation failed!!\n");
		}
	} while (isShellActive);
}

void completeProcess(int exitStatus, int n, char* sacrifice,
		char* beforeSacrifice, char* output, int nn, int* fdX,
		char* buf, int j, char** backgroundProcessFilename,
		int backgroundArrayIndex, char* filecopy) {
	char* dir = (char*)malloc(sizeof(n));

	if (exitStatus == 4) {
		fprintf(stderr, "Error: command not found\n");
	} else if (exitStatus == 5) {
		getcwd(dir, n);							
		strcpy(sacrifice, beforeSacrifice);
		char* dirs = strtok(sacrifice, " ");

		while (dirs) {
			dirs = strtok(NULL, "/");
			if (dirs) {
				if (strcmp(dirs, "..") == 0) {	
					for (int i = strlen(dir) - 1; i; i--)
						if (dir[i] == '/') {
							dir[i] = '\0';
							break;
						}
				} else {
					strcat(dir, "/");
					strcat(dir, dirs);
				}

			} else
				break;

		}

		if (chdir(dir) == -1) {
			fprintf(stderr, "Error: no such directory\n");
			fprintf(stderr, "+ completed '%s' [1]\n", output);
		} else {
			fprintf(stderr, "+ completed '%s' [0]\n", output);
		}

	} else if (exitStatus == 6) {
		fprintf(stderr, "Error: cannot open input file\n");
	} else if (exitStatus == 7) {
		fprintf(stderr, "Error: no input file\n");
	} else if (exitStatus == 8) {
		fprintf(stderr, "Error: cannot open output file\n");
	} else if (exitStatus == 9) {
		fprintf(stderr, "Error: no output file\n");
	} else if (exitStatus == 10) {
		fprintf(stderr, "Error: mislocated input redirection\n");
	} else if (exitStatus == 11) {
		fprintf(stderr, "Error: mislocated output redirection\n");
	} else if (exitStatus == 12) {
		getcwd(dir, n);
		printf("%s\n", dir);
		fprintf(stderr, "+ completed '%s' [0]\n", output);
	} else {
		char* retCodes = (char*)calloc(sizeof(char), nn*3);
		read(fdX[0], buf, 512);

		if (j != 0)
			buf[j] = exitStatus + '0';
		buf[j + 1] = '\0';
		for (int i = 0; i <= j; i++) {
			retCodes[i * 3] = '[';
			retCodes[i * 3 + 1] = buf[i];
			retCodes[i * 3 + 2] = ']';
		}
		retCodes[(j + 1) * 3] = '\0';

		if (backgroundArrayIndex != -1) { //background process
			if (j != 0){
				//printf("if \n");
				fprintf(stderr, "+ completed '%s' %s\n", 
						backgroundProcessFilename[backgroundArrayIndex],
						retCodes);
			} else {
				fprintf(stderr, "+ completed '%s' [%d]\n",
						backgroundProcessFilename[backgroundArrayIndex],
						exitStatus);
			}
		} else { //non-background process
			if (j != 0){
				//printf("if \n");
				fprintf(stderr, "+ completed '%s' %s\n", 
						output, retCodes);
			} else {
				fprintf(stderr, "+ completed '%s' [%d]\n", output,  exitStatus);
			}
		}
	}
}


int checkSleepers(int* sleepingProcess, int waitpidStatus, int childStatus,
		int n, char* sacrifice, char* output, char* beforeSacrifice,
		int nn, int* fdX, char* buf, int j,
		char** backgroundProcessFilename, int backgroundArrayIndex,
		char* filecopy) {
	for (int i = 0; i < 100; i++) {
		if (sleepingProcess[i] != 0) {
			if (sleepingProcess[i] != -1) {
				waitpidStatus = waitpid(sleepingProcess[i], &childStatus,
						WNOHANG);

				if (waitpidStatus == -1) { //child has terminated
					if (WIFEXITED(childStatus) != 0) {//child success term
						if (WEXITSTATUS(childStatus) == 3) { //"exit"
							fprintf(stderr, "Bye...\n");
							return 0;
						} else {
							backgroundArrayIndex = i;
							completeProcess(WEXITSTATUS(childStatus), n,
									sacrifice, beforeSacrifice,
									output, nn, fdX, buf, j,
									backgroundProcessFilename,
									backgroundArrayIndex, filecopy);
							sleepingProcess[i] = -1;
						}
					}
				}
			}
		} else {
			break;
		}
	}
	return 1;
}
