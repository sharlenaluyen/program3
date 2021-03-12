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

int STATUS = 0; //status of the program

struct iosymbols{
	char* inputFileName;
	char* outputFileName;
	bool process; //whether or not this is a background process
	int index; //where the last arg is so that exec only takes input[0]
};

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

//look at number of things in command line
//create array on that number
//add in things from command line into that array
void takeInput(int count, char buffer[], char** input, char mypid[]){
	char* token; 
	//printf("%d\n", count );
	token = strtok(buffer, " ");
	int i = 0;

	int pid = getpid(); //gets pid
	//char mypid[6];   // ex. 34567
	sprintf(mypid, "%d", pid);
	//source: https://stackoverflow.com/questions/53230155/converting-pid-t-to-string
	//printf("token: %s\n", token); fflush(stdout);
	while(token != NULL){
		//printf("%d : %s\n", i, token);
		if(strstr(token, "$$") != NULL) { 
			char* str = token; //thing$$ // $$
			char* res = strstr(token, "$$"); //$$ //$$
			int pos = res - str; //index of first $ = 5 
			if(pos !=0){

				//int substringlength = strlen(str) - pos; // 7 - 5 = 2

			   //char src[40];//= input[i];
			   char dest[40];
			  
			   memset(dest, '\0', sizeof(dest));
			   //strcpy(src, token); //src = input[i]
			   strncpy(dest, token, pos);
			   //dest = thing
			   //
			   input[i] = strcat(dest, mypid);
			} else {

				input[i] = mypid;
			}

			//printf("input at %d : %s\n", i, input[i]); fflush(stdout);
		} else {
			input[i] = token;
		}

		token = strtok(NULL, " ");
		i++;
	}

	input[count-1] = '\0'; //add NULL terminating character at the end
				//int i = 0;
	for(i = 0; i < count; i++){
		printf("input at %d : %s\n", i, input[i]); fflush(stdout);
	}
}

void takeinFiles(int count, char* input[], struct iosymbols* fileinput){
	if(count < 4) return;
	int i = 0;
	fileinput->index = count;

	if(strcmp(input[count-2],"&") == 0){
		fileinput->process = true; 
		//input[count-2] = '\0'; 
		fileinput->index = count-2;
	}

	for(i = count-2; i > count-7 || i > 0; i--){ //count-2 to avoid accessing the null character at the end of input array; we are only checking 5 elements at any time
	//printf("I'm inside the for loop\n"); fflush(stdout);
		if(strcmp(input[i],"<") == 0){
			fileinput->inputFileName = input[i+1];
			if(fileinput->index > i) fileinput->index = i;
		} else if(strcmp(input[i],">") == 0){
			fileinput->outputFileName = input[i+1];
			if(fileinput->index > i) fileinput->index = i;
			//printf("I did it\n"); fflush(stdout);
		} 
	}

	//printf("output: %s\n input: %s\n", fileinput->outputFileName, fileinput->inputFileName); fflush(stdout);
	//return fileinput;
	//command < file > file & \0

}

void exit_(){
	//kill my shell 
	//kill all processes

}

void cd_(int count, char* input[]){
/*	if input[1] = empty
		return the home (like echo $HOME)
	else change the directry to input[1]*/
	char* res;

	if(count-1 == 1){
		res = getenv("HOME");
	} else {
		printf("%s\n", input[1]);
		setenv(input[1], input[1], 0); //do not overwrite
		res = getenv(input[1]);
	}
	
	chdir(res);
}

void status_(){
	printf("exit value %d\n", STATUS); fflush(stdout);

}

void redirectFiles(char* input[], struct iosymbols* fileinput){
	//int res = 0;
	//res = dup2()


	if(fileinput->inputFileName){
		int targetFDin = open(fileinput->inputFileName, O_RDONLY);
		if (targetFDin == -1) { printf("cannot open %s for input\n", fileinput->inputFileName); fflush(stdout); exit(1); }
		//printf("targetFDin == %d\n", targetFDin); fflush(stdout); // Written to terminal

		//printf("targetFDin == %d, resultin == %d\n", targetFDin, resultin); // Written to file
		int resultin = dup2(targetFDin, 0);
		if (resultin == -1) { perror("dup2"); exit(2); }
	} else if(fileinput->process) {
		dup2(open("/dev/null", 0), 0);
	}

	if(fileinput->outputFileName){
		int targetFDout = open(fileinput->outputFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (targetFDout == -1) { printf("cannot open %s for output\n", fileinput->outputFileName); fflush(stdout); exit(1); }
		//printf("targetFDout == %d\n", targetFDout); fflush(stdout); // Written to terminal

		//printf("targetFDout == %d, resultout == %d\n", targetFDout, resultout); fflush(stdout); // Written to file
		int resultout = dup2(targetFDout, 1);
		if (resultout == -1) { perror("dup2"); exit(2); }
	} else if(fileinput->process) {
		dup2(open("/dev/null", 0), 1);
	}
}


//gets the pids 
void getPIDs(pid_t* child_pids, int* child_pid_index){
	int size = 0;
	int i = 0;
	pid_t temp[*child_pid_index];
	//printf("CHILD_PID_INDEX: %d\n", *child_pid_index); fflush(stdout);


	for(i = 0; i < *child_pid_index; i++){
		if(child_pids[i] == 0) continue;
		temp[size] = child_pids[i];
		//printf("temp: %d \t child_pids: %d\n", temp[size], child_pids[i]);
		child_pids[i] = 0;
		size++;
	}	
	//printf("in getPIDS\n"); fflush(stdout);

	*child_pid_index = size;
	for(i = 0; i < size; i++){
		child_pids[i] = temp[i];
	}
}

void setStatus(int* childExitMethod, bool background_process){ //generically sets status
	//printf("I'm in setStatus\n"); fflush(stdout);
		//STATUS = childExitMethod;
	if (WIFEXITED(*childExitMethod) != 0){ //if we exited normally, then we can continue with the exit status. otherwise, process it through signals
		//printf("The process exited normally\n"); fflush(stdout);
		STATUS = WEXITSTATUS(*childExitMethod);
		if(background_process){
			status_();
		}
	} else if (WIFSIGNALED(*childExitMethod) != 0){ 
		//printf("The process was terminated by a signal\n"); fflush(stdout);
		if(background_process){
			printf("terminated by signal %d\n", STATUS); fflush(stdout);
		}
	}


	//printf("childExitMethod: %d\n", *childExitMethod);
	//if(STATUS != 0) printf("Error something is wrong in childprocess\n"); fflush(stdout);
	//printf("child pid at 0: %d\n", child_pids[0]); fflush(stdout); 
	//exit(0);
	
}


void setPIDs(pid_t * child_pids, int* child_pid_index, int* childExitMethod, struct iosymbols* fileinput){
	//get the process ids adjust that
	//setStatus(child_pids, fileinput->process);

	//if process has finished,
		//call set status
		//set spot to 0

	//call getpids to reset
	int i = 0;
	for(i = 0; i < *child_pid_index; i++){
		//printf("CHILD_PID_INDEX: %d\n", *child_pid_index); fflush(stdout);
		if(waitpid(child_pids[i], childExitMethod, WNOHANG) != 0){
			printf("background pid %d is done: ", child_pids[i]); fflush(stdout);

			setStatus(childExitMethod, true);
			child_pids[i] = 0;
		}
	}
			//printf("end of setPIDS\n"); fflush(stdout);

	getPIDs(child_pids, child_pid_index);
}

void commandExecution(int count, char* input[], int* childExitMethod, pid_t* child_pids, int* child_pid_index, struct iosymbols* fileinput){
	//fork a child
	//child will redirect i/o
	//if no issues with file, then run exec
	//redirect i/o

	pid_t spawnpid = -5;
	spawnpid = fork(); //makes a whole copy of my program and starts onward as child

	switch (spawnpid){
	case -1:
		perror("Hull Breach!");
		exit(1);
		break;
	case 0:		
		redirectFiles(input, fileinput);

		int i = 0;
		if(fileinput->index != 0){ //to avoid null-ing out the arguments if there is no file redirection
			for(i = fileinput->index; i < count-1; i++){
				input[i] = '\0';
			}
		}

		if (execvp(input[0], input) < 0){
			perror("Cannot be executed.");
			STATUS = 1;
			exit(1);
		}

		break;
	default:
	//waitpid here for parent to wait for child to finish
	//no spawnpid
		if(!fileinput->process){ //foreground
			waitpid(spawnpid, childExitMethod, 0); //not hanging, forcibly wait for child to terminate
			setStatus(childExitMethod, fileinput->process);
		} else { //for background
			printf("background pid is %d\n", spawnpid); fflush(stdout);
			child_pids[*child_pid_index] = spawnpid;
			*child_pid_index += 1;	
			//setPIDs(child_pids, child_pid_index, &childExitMethod, fileinput);		
		}
		//setPIDs(child_pids, child_pid_index, &childExitMethod, fileinput);
		
		//if(){
		//} else
		//printf("child pid index: %d \t spawnpid: %d \n", *child_pid_index, spawnpid); fflush(stdout);
		//*child_pid_index +=1; // *child_pid_index + 1;
		//printf("%d\n", spawnpid); fflush(stdout);
		break;
	}
	//exit(0);

}

void printInput(int count, char* input[]){
	int i = 0;
	//printf("size of input %ld \t size of char** %ld\n", sizeof(input), sizeof(char**));
	for(i = 0; i < count-1; i++){ //count-1 to avoid printing the NULL character at the end
		printf("%s\n", input[i]); fflush(stdout);
	}
}


void main(){
	char buffer[516];
	memset(buffer, '\0', 516);
	pid_t child_pids[516];
	int child_pid_index = 0;
	int stat = 0; //status
	int childExitMethod = -5;

	char mypid[6];
	memset(mypid, '\0', 6);
	
	struct iosymbols fileinput;


	while(1){
		fileinput.inputFileName = NULL;
		fileinput.outputFileName = NULL;
		fileinput.process = false;
		fileinput.index = 0;

		setPIDs(child_pids, &child_pid_index, &childExitMethod, &fileinput);

		printf(": "); fflush(stdout);
		fgets(buffer, 516, stdin); //takes input and puts it in buffer
		buffer[strcspn(buffer, "\n")] = 0; //so we have to take it out so the input gets processed properly
		int count = inputSize(buffer);
		if(count == 0) continue;

		char* input[count];
		memset(input, '\0', count);

		takeInput(count, buffer, input, mypid);
		int i =0;


/*		printf("%ld\n", sizeof(input)); fflush(stdout);
		int i = 0;
		for(i = 0; i < sizeof(input)/sizeof(char*); i++){
			printf("%d %s\n", i, input[i]); fflush(stdout);
		}*/
		//printf("%s\n", input[0]); fflush(stdout);

		//printInput(count, input);


		if(strcmp(input[0], "#") == 0){
			continue;
		}
			else if (strcmp(input[0], "exit") == 0){
			//printf("%s\n", "exit"); fflush(stdout);
			exit_();
			//return 0;
			
		} else if (strcmp(input[0], "cd") == 0){
			//printf("%s\n", "we are in cd"); fflush(stdout);
			cd_(count, input);

			
		} else if (strcmp(input[0], "status") == 0){
			//printf("%s\n", "we are in status"); fflush(stdout);
			status_();

			
		} else {
			//printf("Command Exec\n"); fflush(stdout);
			//if (count>5) 


			takeinFiles(count, input, &fileinput);


			commandExecution(count, input, &childExitMethod, child_pids, &child_pid_index, &fileinput); //whenever this child fork finishes, we know to clean it up
			//redirectFiles() //redirect inside child process, not parent since this is called after exec in commandExecution
			
		}
	}

}
