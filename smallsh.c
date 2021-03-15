/***********************************************************
* Sharlena Luyen
* Operating Systems 1; CS 344 Winter 2020
* Program Description: This is a program that is meant to mimick
* certain aspects of a bash shell. Specifically, this program
* can properly execute cd, status, and exit. All other default
* commands can also be run from the default shell. File redirection
* and background/foreground speifications were made by CS 344.
************************************************************/

/***********************************************************
* 
* Parameters:
* Return Value:
* Description:
*
*
***********************************************************/

#include <sys/types.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

int STATUS = 0; //keeps track of the status of the program throughout running

struct iosymbols{
	char* inputFileName;
	char* outputFileName;
	bool process; //whether or not this is a background process (if & is included from user input)
	int index; //where the last arg is so that exec only takes input[0]
};

bool Zset = false; //boolean for deciding if a signal was sent from CTRL Z
bool sigstop = false; //boolean for message displays entering/exiting foreground mode

bool signalExitMethod = false; //boolean for deciding if a signal was sent from CTRL C

/***********************************************************
* inputSize Function
* Parameters: char buffer[], user input
* Return Value: int count, number of spaces inside of user input
* Description: simply counts the number of spaces plus 1 for
* the extra null character at the end of the user input.
* This function is primarily used to set the input array
* to the proper size to manipulate throughout the program.
***********************************************************/
int inputSize(char buffer[]){
	int count = 1; //starts at 1 since array is 0-based
	int i = 0;
	bool charcount = false;
	for(i = 0; i < strlen(buffer); i++){ //checks if empty line
		if(isalpha(buffer[i]) || (buffer[i] == '$' && buffer[i+1] == '$')) charcount = true;
	}
	if(!charcount) return 0;

	for(i = 0; i < strlen(buffer); i++){ //counts number of spaces in input
		if(buffer[i] == ' ') count++;
	}
	count++; //used for null terminating at the end of array input

	return count;
}

/***********************************************************
* takeInput Function
* Parameters:	int count, count of elements in user input
				char buffer[], user input straight from stdin
				char**input, user input where each element is a command/argument
				char mypid[], current working process id
				char dest[], destination of expansion of "$$"
* Return Value: N/A
* Description: Look through user input through space delimiter
* to insert each word of input user into each element of input[]
* which is the exact size of the input plus one for null character.
* Any "$$" from user input will be expanded. 
***********************************************************/
//look at number of things in command line
//create array on that number
//add in things from command line into that array
void takeInput(int count, char buffer[], char** input, char mypid[], char dest[]){
	char* token; 
	token = strtok(buffer, " ");
	int i = 0;

	int pid = getpid(); //gets pid ex. 34567
	sprintf(mypid, "%d", pid); //converts pid to string for char*input compatability
	//source: https://stackoverflow.com/questions/53230155/converting-pid-t-to-string
	while(token != NULL){
		//this if statement will properly process any input that contains "$$"
		if(strstr(token, "$$") != NULL) { //ie if command given is "mkdir thing$$"
			char* str = token; //thing$$ // $$
			char* res = strstr(token, "$$"); //$$ //$$
			int pos = res - str; //index of first "$" = 5 = 7-2
			if(pos !=0){			  
			   memset(dest, '\0', sizeof(dest));
			   strncpy(dest, token, pos); //sets dest to token up to pos (thing)
			   input[i] = strcat(dest, mypid); //concatenates thing and pid to thing34567 into input array
			} else {
				input[i] = mypid; //if pos == 0 (input is only "$$), add pid into input
			}
		} else {
			input[i] = token; //if input doesn't have "$$" at all, add into input
		}
		token = strtok(NULL, " "); //space delimiter
		i++; //increment through user input
	}

	input[count-1] = '\0'; //add NULL terminating character at the end of input
}

/***********************************************************
* takeinFiles function
* Parameters: int count, count of elements in user input
			  char* input[], cleaned up user input where each element is a user's command/argument
			  struct iosymbols* fileinput, if the user puts in files to be redirected,
			  the out/input file names will be stored in this struct
* Return Value: N/A
* Description: According to the user's input, if there is a < >, the next
* elements will be respectively stored as in/output file names. 
* Additionally, if a user specifies a "&" at the end, this means the 
* process should be a background process. The fileinput->process 
* variable specifies this. Then, the index/count is adjusted
* so that the command is executed properly.
***********************************************************/
void takeinFiles(int count, char* input[], struct iosymbols* fileinput){
	if(count < 4) return; //if there are no files specified, don't run this function
	int i = 0;
	fileinput->index = count; //sets the starting point to find the index where the file names start

	if(strcmp(input[count-2],"&") == 0){ //if "&" is in the input, then it is a background process
		fileinput->process = true; 
		if(Zset == true){				//if a CRTL Z signal was sent, everything is a foreground process 
			fileinput->process = false;
		}
		fileinput->index = count-2; //sets index to not include end \0 character and &
	}

	for(i = count-2; i > count-7 || i > 0; i--){ //count-2 to avoid accessing the null character at the end of input array; 
												//we are only checking 5 elements at any time
		if(strcmp(input[i],"<") == 0){ //sets filenames based off of < or >
			fileinput->inputFileName = input[i+1];
			if(fileinput->index > i) fileinput->index = i; //sets index to command for execvp to work at input[0]
		} else if(strcmp(input[i],">") == 0){
			fileinput->outputFileName = input[i+1];
			if(fileinput->index > i) fileinput->index = i;
		} 
	}
}

/***********************************************************
* exit_ Function
* Parameters: N/A
* Return Value: exit(0) - terminates  program
* Description: kills all processes via TERM signal. Since
* this signal prints "Terminate" to the screen, redirect
* stdout to stderr to avoid printing.
***********************************************************/
void exit_(){
	fflush(stdout);
	dup2(1, 2); //redirecting anything from stdout to stderr
	raise(SIGTERM); //since this prints "Terminate"
	exit(0);
}

/***********************************************************
* cd_ Function
* Parameters: int count, number of elements in user inpout
			  char* input[], entire user's input
* Return Value: N/A
* Description: Changes user's directory based on given
* path. If no path is given, sets directory to HOME.
***********************************************************/
void cd_(int count, char* input[]){
	char* res;

	//sets res to upcoming directory
	if(count-1 == 1){ //if there is no path given, set res to HOME directory
		res = getenv("HOME");
	} else {
		//printf("%s\n", input[1]); fflush(stdout);//otherwise, set res to given directory
		setenv(input[1], input[1], 0); //do not overwrite
		res = getenv(input[1]);
	}
	
	chdir(res); //change to res directory
}

/***********************************************************
* status_ Function
* Parameters: N/A
* Return Value: N/A
* Description: Prints status of program. The status is global
* in order to print the proper message to user.
***********************************************************/
void status_(){ //used status as global variable due to background/forground processes
	if(signalExitMethod){
		printf("terminated by signal %d\n", STATUS); fflush(stdout);
	} else {
		printf("exit value %d\n", STATUS); fflush(stdout);
	}
}

/***********************************************************
*  redirectFiles Function
* Parameters: char* input[], user's input
			  struct iosymbols* fileinput, user in/output files' names
* Return Value: N/A
* Description: Redirects files based on < > characters and 
* input/output file names given from user. < > Characters
* handled by default shell. This code was based off of lecture slides.
***********************************************************/
void redirectFiles(char* input[], struct iosymbols* fileinput){
	if(fileinput->inputFileName){ //if there is an inputfilename
		int targetFDin = open(fileinput->inputFileName, O_RDONLY); //opens the file for READ ONLY
		if (targetFDin == -1) { printf("cannot open %s for input\n", fileinput->inputFileName); fflush(stdout); exit(1); }

		int resultin = dup2(targetFDin, 0); //redirects the stuff to stdin (0)
		if (resultin == -1) { perror("dup2"); exit(2); }
	} else if(fileinput->process) { //if there was no outputfile and was a background process, set stdout to /dev/null
		dup2(open("/dev/null", 0), 0);
	}

	if(fileinput->outputFileName){ //if there is an outputfilename
		int targetFDout = open(fileinput->outputFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644); //opens the file to write
		if (targetFDout == -1) { printf("cannot open %s for output\n", fileinput->outputFileName); fflush(stdout); exit(1); }

		int resultout = dup2(targetFDout, 1); //redirects the stuff to stdout (1)
		if (resultout == -1) { perror("dup2"); exit(2); }
	} else if(fileinput->process) { //if there was no outputfile and was a background process, set stdout to /dev/null
		dup2(open("/dev/null", 0), 1);
	}
}


/***********************************************************
* getPIDs Function
* Parameters: pid_t* child_pids, ptr to an array of child process ids
			  int* child_pid_index, ptr to index of child_pids
* Return Value: N/A
* Description: This function resets the child_pids array based off of
* whether or not a child process terminated or is still running.
* The index is updated as such as well.
***********************************************************/
void getPIDs(pid_t* child_pids, int* child_pid_index){
	int size = 0;
	int i = 0;
	pid_t temp[*child_pid_index]; 

	for(i = 0; i < *child_pid_index; i++){ //transfers over active child_pids (non-active pids were set to \0 NULL in setpids)
		if(child_pids[i] == 0) continue; //if there are no active pids, move on
		temp[size] = child_pids[i];
		child_pids[i] = 0;
		size++; //capture new size
	}	
	*child_pid_index = size; //reset the index to the new size of the active pids
	for(i = 0; i < size; i++){ //transfer over local temp variable's contents to child_pids 
		child_pids[i] = temp[i];
	}
}

/***********************************************************
* setStatus Function
* Parameters: int* childExitMethod, describes how the child exited 
			  (usually 1 or 0) or the signal number
			  bool background_process, whether or not the current process is in the background
			  cannot use fileinput->process since this boolean can be true when
			  we set PIDs, but not always
			  struct iosymbols* fileinput, 
* Return Value: N/A
* Description: Sets the status of the program based on a signal or not.
***********************************************************/
void setStatus(int* childExitMethod, bool background_process){ //generically sets status
	if (WIFEXITED(*childExitMethod) != 0){ //if we exited normally, then we can continue with the exit status. otherwise, process it through signals
		STATUS = WEXITSTATUS(*childExitMethod); //set status to 0 or 1 usually based on if the command was successfully executed or not
		signalExitMethod = false; //reset signal ExitMethod so the correct print statement prints
		if(background_process){
			status_(); //print message only
		}
	} else if (WIFSIGNALED(*childExitMethod) != 0){ 
		STATUS = WTERMSIG(*childExitMethod); //sets status based on signal capture
		printf("terminated by signal %d\n", STATUS); fflush(stdout);
		signalExitMethod = true; //reset signal ExitMethod so the correct print statement prints
	}	
}

/***********************************************************
* setPIDs Function
* Parameters: pid_t* child_pids,
			  int* child_pid_index,
			  int* childExitMethod,
			  struct iosymbols* fileinput,

* Return Value: N/A
* Description: Ultimately, sets child_pids to only active PIDs.
* If a process has finished, call setStatus and set the child_pid[i] to 0.
* After updating the current PIDs and setting the non-current
* or non-active PIDs to NULL, send child_pids to getPIDs to
* reset the size and take out the NULLS so that child_pids
* only contains active PIDs.
***********************************************************/
void setPIDs(pid_t * child_pids, int* child_pid_index, int* childExitMethod, struct iosymbols* fileinput){
	//get the process ids adjust that

	//if process has finished,
		//call set status
		//set spot to 0
	int i = 0;
	for(i = 0; i < *child_pid_index; i++){
		if((waitpid(child_pids[i], childExitMethod, WNOHANG) != 0)){ //if processed has finished
			printf("background pid %d is done: ", child_pids[i]); fflush(stdout); //print to terminal
			setStatus(childExitMethod, true); //set the status
			child_pids[i] = 0; //set actual pid to 0
		}
	}
	getPIDs(child_pids, child_pid_index); //reset child_pids to exclude NULLs and have correct size 
}

/***********************************************************
* commandExecution Function
* Parameters:	almost everything in the program
* Return Value: N/A
* Description: Overall handler for commands that are NOT built-in.
* Also handles child's signals and file redirection. This 
* function is detailed line by line below.
***********************************************************/
void commandExecution(int count, char* input[], int* childExitMethod, pid_t* child_pids, int* child_pid_index, struct iosymbols* fileinput){
	pid_t spawnpid = -5;
	struct sigaction ignore_action = {0}, SIGINT_action = {0};
	spawnpid = fork(); //makes a whole copy of my program and starts onward as child

	switch (spawnpid){
	case -1: //SOMETHING IS VERY WRONG
		perror("Hull Breach!");
		exit(1);
		break;
	case 0:	//child process
		//ignore signal so that the CTRL Z signal handler defined in main can run
		ignore_action.sa_handler = SIG_IGN;
		sigaction(SIGTSTP, &ignore_action, NULL);

		//handle CTRL C signal
		if(fileinput->process){ //if background process
			SIGINT_action.sa_handler = SIG_IGN;
			sigaction(SIGINT, &SIGINT_action, NULL); //do the specified command
		} else {				//if NOT background process
			SIGINT_action.sa_handler = SIG_DFL; //do the default action
			sigaction(SIGINT, &SIGINT_action, NULL);
		}

		redirectFiles(input, fileinput); //redirect files

		int i = 0;
		if(fileinput->index != 0){ //to avoid null-ing out the arguments if there is no file redirection
			for(i = fileinput->index; i < count-1; i++){ //null out arguments after file redirection
				input[i] = '\0';
			}
		}

		if (execvp(input[0], input) < 0){ //run the command (the user's first input from terminal)
			//if the command is nonexistent
			printf("%s: ", input[0]); fflush(stdout); //print the user's erroneous input
			perror(""); //error out
			STATUS = 1; //set the status to 1 (error)
			exit(1); //exit
		}
		break;
	default: //parent process
	//waitpid here for parent to wait for child to finish
		if(!fileinput->process){ //foreground
			waitpid(spawnpid, childExitMethod, 0); //not hanging, forcibly wait for child to terminate
			setStatus(childExitMethod, fileinput->process); //set the status from child's Exit Method
		} else { //for background
			printf("background pid is %d\n", spawnpid); fflush(stdout);
			child_pids[*child_pid_index] = spawnpid; //add a child_pid since it's current
			*child_pid_index += 1; //increase index from the above addition
		}
		break;
	}
}

/***********************************************************
* catchSIGTSTP Function
* Parameters: int signo
* Return Value: N/A
* Description: If a CTRL Z signal is sent, manage these global
* booleans to display the proper display messages.
***********************************************************/
void catchSIGTSTP(int signo){
	Zset = !Zset; //switches every time we run it so that we know when we enter and exit foreground mode
	sigstop = true; //boolean for message displays entering/exiting foreground mode
}


void main(){
	char buffer[516];
	memset(buffer, '\0', 516);

	pid_t child_pids[516];
	memset(child_pids, '\0', 516);

	int child_pid_index = 0;
	int childExitMethod = -5;

	char* message;
	char mypid[6];
	char dest[40];
	memset(mypid, '\0', 6);
	memset(dest, '\0', 40);
	
	struct iosymbols fileinput;
	struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};

	//parent is ignoring CTRL C!!
	SIGINT_action.sa_handler = SIG_IGN;
	sigaction(SIGINT, &SIGINT_action, NULL);

	//catches signal from crtl z
	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;

	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	while(1){
		memset(buffer, '\0', 516); //reset buffer every time a user can put input
		fileinput.inputFileName = NULL; //initialize and reset fileinput every time a user can put input
		fileinput.outputFileName = NULL;
		fileinput.process = false;
		fileinput.index = 0;

		//handles display messages based on CTRL Z signal
		if(Zset == true && sigstop == true){
			sigstop = false; //reset sigstop
			message = "\nEntering foreground-only mode (& is now ignored)\n"; 
			write(STDOUT_FILENO, message, 56);
		} else if(Zset == false && sigstop == true){
			sigstop = false; //reset sigstop
			message = "\nExiting foreground-only mode\n"; 
			write(STDOUT_FILENO, message, 30);
		}

		setPIDs(child_pids, &child_pid_index, &childExitMethod, &fileinput);

		printf(": "); fflush(stdout);
		fgets(buffer, 516, stdin); //takes input and puts it in buffer
		buffer[strcspn(buffer, "\n")] = 0; //so we have to take it out so the input gets processed properly

		int count = inputSize(buffer);
		if(count == 0) continue; //if blank line

		char* input[count];
		memset(input, '\0', count);

		takeInput(count, buffer, input, mypid, dest); //takes user input and populates the input into input[] for proper manipulation

		//switch handles comment, built-in commands, or given command
		if(strcmp(input[0], "#") == 0){ //user input is comment
			continue;
		}
			else if (strcmp(input[0], "exit") == 0){
			exit_();
			
		} else if (strcmp(input[0], "cd") == 0){
			cd_(count, input);

			
		} else if (strcmp(input[0], "status") == 0){
			status_();

			
		} else {
			takeinFiles(count, input, &fileinput); //parse user input for file names
			commandExecution(count, input, &childExitMethod, child_pids, &child_pid_index, &fileinput); 
			//handles the user's input
			//whenever this child fork finishes, we know to clean it up
		}
	}
}
