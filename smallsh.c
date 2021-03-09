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

int STATUS = 0;

int inputSize(char buffer[]){
	int count = 1; //starts at 1 since
	int i = 0;
	bool charcount = false;
	for(i = 0; i < strlen(buffer); i++){ //checks if empty line
		if(isalpha(buffer[i]) || (buffer[i] == '$' && buffer[i+1] == '$')) charcount = true;
	}
	if(!charcount) return 0;

	for(i = 0; i < strlen(buffer); i++){ //counts number of spaces
		if(buffer[i] == ' ') count++;
	}
	count++; //used for null terminating at the end of array input

	return count;
}

//look at number of things in command line
//create array on that number
//add in things from command line into that array
void takeInput(int count, char buffer[], char** input){
	char* token; 
	//printf("%d\n", count );
	token = strtok(buffer, " ");
	int i = 0;
	//char* input[count];


	int pid = getpid(); //gets pid
	char mypid[6];   // ex. 34567
	sprintf(mypid, "%d", pid);
	//source: https://stackoverflow.com/questions/53230155/converting-pid-t-to-string
	//printf("token: %s\n", token); fflush(stdout);
	while(token != NULL){
		//printf("%d : %s\n", i, token);
		if(strcmp(token, "$$") == 0) {
			input[i] = mypid;
		} else {
			input[i] = token;
		}

		token = strtok(NULL, " ");
		i++;
	}

	input[count-1] = '\0'; //add NULL terminating character at the end

	//return input;
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
		res = getenv(input[1]);
	}
	
	chdir(res);


}

void status_(){
	printf("status: %d\n", STATUS); fflush(stdout);

}

void commandExecution(char* input[], pid_t* child_pids, int* child_pid_index){
	//fork a child
	//child will redirect i/o
	//if no issues with file, then run exec
	//redirect i/o

	pid_t spawnpid = -5;
	int ten = 10;
	spawnpid = fork();
	switch (spawnpid){
		case -1:
		perror("Hull Breach!");
		exit(1);
		break;
	case 0:
		ten = ten + 1;
		printf("I am the child! ten = %d\n", ten); fflush(stdout);

		if (execvp(input[0], input) < 0){
			perror("Cannot be executed.");
			STATUS = 1;
			exit(1);
		}

		//execvp(input[0], input);
		break;
	default:
		child_pids[*child_pid_index] = spawnpid;
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

	while(1){
		printf(": "); fflush(stdout);
		fgets(buffer, 516, stdin); //takes input and puts it in buffer
		buffer[strcspn(buffer, "\n")] = 0; //so we have to take it out so the input gets processed properly
		int count = inputSize(buffer);
		if(count == 0) continue;

		char* input[count];
		takeInput(count, buffer, input);

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
			return 0;
			
		} else if (strcmp(input[0], "cd") == 0){
			//printf("%s\n", "we are in cd"); fflush(stdout);
			cd_(count, input);

			
		} else if (strcmp(input[0], "status") == 0){
			//printf("%s\n", "we are in status"); fflush(stdout);
			status_();

			
		} else {
			//printf("Command Exec\n"); fflush(stdout);
			commandExecution(input, child_pids, &child_pid_index); //whenever this child fork finishes, we know to clean it up
			waitpid(child_pids[0], &childExitMethod, 0);
			//STATUS = childExitMethod;
			if (WIFEXITED(childExitMethod) != 0){ //if we exited normally, then we can continue with the exit status. otherwise, process it through signals
				printf("The process exited normally\n");
				STATUS = WEXITSTATUS(childExitMethod);
			} else if (WIFSIGNALED(childExitMethod) != 0){ 
				printf("The process was terminated by a signal\n");
			}

			printf("childExitMethod: %d\n", childExitMethod);
			if(STATUS != 0) printf("whyyyyyyyyyyyyyyyyyyyy\n"); fflush(stdout);
			printf("child pid at 0: %d\n", child_pids[0]); fflush(stdout); 
			//exit(0);
		}
	}

}