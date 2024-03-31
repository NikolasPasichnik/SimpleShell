#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <fcntl.h>

/*
Assumptions Made: 

1. echo: The input given for this built-in command can both be with or without quotes at the edges
    ex: `echo "Hello World!"` and `echo Hello World!` will both print `Hello World!`

2. cd: The input given for this built-in command must be surrounded by quotation marks (""), otherwise
       it will fail to execute 
    ex: `cd "newDir"` will execute, while `cd NewDir` will throw an error

3. jobs and fg: Since the structure used to represent background jobs I chose to use was a Linked List, 
                there is no upper bound on the number of jobs that can run in the background, as long 
                as there is sufficient memory. 

4. Piping and redirection: I assumed that there will be no attemps to put these processes into the background with &, 

5. Commands passed in the shell do not exceed 20 arguments, and are at most 16 characters long (except for paths)
 
*/


// =================================== Linked List Struct and Functions =======================================

/*
Running Jobs is a Struct that holds information about a background job 

- isActive: used to determine if a job is currently active or if it has been caught by the parents
- jobNumber: holds the current number of the job (0-15)
- jobPID: holds the PID of the current process 
- jobName: holds the string of the command that the job is executing 
*/ 
struct RunningJobs {
    int jobNumber; 
    pid_t jobPID; 
    char jobName[16*20]; //20 arguments, each token being at most 16 chars! 
    struct RunningJobs *next; 
};

/*
Iterates over the linked list containing the background jobs and prints their ID, PID and the command itself
*/
void listRunningJobs(struct RunningJobs* head){
    // Listing all the elements in the background
    while (head != NULL){
        printf("[%d]        %ld        %s\n", head->jobNumber, (long) head->jobPID, head->jobName);
        head = head->next; 
    }
}

/*
Appends a new background job to the tail of the linked list
*/
void addNewRunningJob(struct RunningJobs** head, int jobNumber, pid_t jobPID, char jobName[]){

    // Allocating memory for the new jobList element
    struct RunningJobs *newJob = (struct RunningJobs*)malloc(sizeof(struct RunningJobs));
    
    // Filling this new struct with the inputted values
    newJob->jobNumber = jobNumber; 
    newJob->jobPID = jobPID; 
    strcpy(newJob->jobName, jobName);
    newJob->next = NULL;

    struct RunningJobs *tempJob = *head;

    // Checking whether the list contains any elements
    if (*head == NULL){
        *head = newJob;
        return; 
    }

    // Case where there are other elements in the list
    else{
        // Reaching the last element of the linked list
        while (tempJob->next != NULL){
            tempJob = tempJob->next; 
        }
        
        // Appending new element to the end of the list
        tempJob->next = newJob; 
        return;
    }
}

/*
Removes a job from the linked list on it based on a given ID. 
Returns: the PID of the job that is to be removed from the background
*/
pid_t removeRunningJob(struct RunningJobs** head, int jobNumber){

    struct RunningJobs *tempJob = *head;
    struct RunningJobs *prevJob = NULL;
    pid_t pidValue = 0; 


    // Case where there are no jobs running in the background, the list is empty (NULL) 
    if (*head == NULL){
        return -1; 
    }

    // Case where the head is the element we are looking for
    if ((tempJob != NULL && tempJob->jobNumber == jobNumber) || jobNumber == -1){
        *head = tempJob->next; 
        pidValue = tempJob->jobPID;

        printf("%s\n",(tempJob->jobName));
        free(tempJob);
        return pidValue; 
    }

    // Case where the jobs is not the head, need to look through the list to find it (if it exists)
    while (tempJob != NULL && tempJob->jobNumber != jobNumber){
        prevJob = tempJob; 
        tempJob = tempJob->next; 
    }

    // Case where the job with ID = jobNumber was not found
    if (tempJob == NULL){
        return -1;
    }

    // Case where the job with ID = jobNumber was found, need to remove from list
    else if (prevJob != NULL){
        prevJob->next = tempJob->next; 
        pid_t pidValue = tempJob->jobPID;

        printf("%s\n",(tempJob->jobName));
        free(tempJob);
        return pidValue;
    }

    return -1; 
}

// ============================================= Helper Functions =============================================

const int numberOfBuiltInCommands = 6; //Holds the number of built in commands

/*
This function obtains input from the user and "tokenizes" it
Returns: the number of arguments the user has passed to the shell
NOTE: This code was taken from the assignment handout
*/
int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL; 
    size_t linecap = 0;

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

    if (length <= 0) {
        exit(-1);
    }

    if ((loc = index(line, '&')) != NULL) {
        *background = 1; 
        *loc = ' '; 
    } else {
        *background = 0; 
    }

    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (int j = 0; j < strlen(token); j++){
            if (token[j] <= 32) {
                token[j] = '\0';
            }
        }
        if (strlen(token) > 0) {
            args[i++] = token;
        }  
    }

    return i;
}

/*
This function takes as a argument a shell command (string) and verifies whether it's part of the built-in commands
Output: 1 if it's a built-in function, 0 otherwise 
*/
int isBuiltIn(char *commandString, char **builtInCommandArray){
    int validCommand = 0; 

    // Iterating over the array of built-in command names and matching strings
    for (int i = 0; i<numberOfBuiltInCommands; i++){
        if (strcmp(commandString, builtInCommandArray[i]) == 0){
            validCommand = 1; 
        }
    }

    return validCommand; 
}

/*
This function iterates over the arguments of the command and identify whether output redirection is requested (">")
Output: 1 if output redirection was requested, 0 otherwise
*/
int redirectRequested(char **argsArray, int numberOfArguments){
    int isRedirected = 0; 

    // Iterating over the arguments and matching string to find if ">" is part of the args 
    for (int i = 0; i<numberOfArguments; i++){
        if (strcmp(argsArray[i], ">") == 0){
            return 1; 
        }
    }
    return isRedirected; 
}

/*
This function iterates over the arguments of the command and identify whether piping is requested ("|")
Output: 1 if piping was requested, 0 otherwise
*/
int pipeRequested(char **argsArray, int numberOfArguments){
    int isPiped = 0; 

    // Iterating over the arguments and matching string to find if "|" is part of the args 
    for (int i = 0; i<numberOfArguments; i++){
        if (strcmp(argsArray[i], "|") == 0){
            return 1; 
        }
    }
    return isPiped; 
}

/*
This function updates the list of background jobs when a new one is requested 
Output: 0 if the job was placed and 1 if 16 processes are already running in the bg 
*/
int addJobToBackgroundList(struct RunningJobs **jobArray, pid_t jobPID, char **givenArguments, int numberOfArgs, int* jobCount){

    //Assuming that each arg is at most 16 chars and there's at most 20 arguments!
    char command[16*20] = "";  
    
    // Make a single string out of the givenArguments (for the jobName field)
    for (int i = 0; i<numberOfArgs; i++){
        strcat(command, givenArguments[i]); 
        strcat(command, " "); 
    }

    //Adding the ampersand, as it gets removed in the getcmd
    strcat(command, "&"); 

    // Appending job to list of bg jobs
    addNewRunningJob(jobArray, *jobCount, jobPID, command);

    // Incrementing job count, used for the ID of each job
    *jobCount = *jobCount + 1;
    return 1; 
}

// ============================================= Native Implementations =============================================

// Native echo implementation 
int echoFunction(char **givenArguments, int numberOfArguments){

    for (int i = 1; i< numberOfArguments; i++){

        // Case where the first character is a " (we remove and print)
        if (i == 1 && givenArguments[i][0] == '"'){
            // Case where it's only 1 word surrounded by "", must remove from both sides
            if (givenArguments[i][strlen(givenArguments[i])-1] == '"'){
                givenArguments[i][strlen(givenArguments[i])-1] = '\0';
                printf("%s ", ++givenArguments[i]);
            }
            // Case where it's not only 1 word, only remove the first char (which is a ")
            else {
                printf("%s ", ++givenArguments[i]);
            }
        }
        // Case where the last character is a " (we remove and print)
        else if (i == (numberOfArguments-1) && givenArguments[i][strlen(givenArguments[i])-1] == '"'){
            // Removing the " at the end of the arguments
            givenArguments[i][strlen(givenArguments[i])-1] = '\0';

            printf("%s", givenArguments[i]);
        }
        // Normal case 
        else{
            printf("%s ", givenArguments[i]);
        }
    }

    // Adding a newline char
    printf("\n");

    return 0; 
}

// Native cd implementation 
int cdFunction(char *pathString){

    // Since it's a string, we need to remove the last "
    pathString[strlen(pathString)-1] = '\0';

    // Starting at the next character, to avoid the first "
    if (chdir(++pathString) != 0){
        perror("Error: "); 
        return 1; 
    }

    return 0; 
}

// Native pwd implementation
int pwdFunction(){
    // Buffer to hold the current path 
    char pathBuffer[PATH_MAX]; 

    // Calling getcwd and printinf the current working directory
    if (getcwd(pathBuffer, sizeof(pathBuffer)) != NULL) {
        printf("%s\n", pathBuffer); 
    } 
    // case where getcwd() fails, return non-zero return status
    else { 
        return 1; 
    }

    return 0; 
}

// Native exit implementation - FREE
void exitFunction(struct RunningJobs *jobList){

    struct RunningJobs *tempJob = NULL;
    pid_t PID = 0; 

    // Free everything? 
    while (jobList != NULL){

        // Getting PID and terminating the child process
        PID = jobList->jobPID;
        kill(PID, SIGKILL);

        // Freeing the memory allocated for the linked list element
        tempJob = jobList; 
        jobList = jobList->next; 
        free(tempJob); 
    }

    // Exiting the entire program
    exit(0); 
}

// Native fg implementation
void fgFunction(struct RunningJobs **jobList, int jobNumber, int *jobCount){
    pid_t tempPID = 0; 

    // If no input was given to fg, bring the first job in the list to the foreground;
    if (jobNumber == -1){
        tempPID = removeRunningJob(jobList, -1);
    }

    // Input was given, need to remove job at ID, if it exists
    else{
        tempPID = removeRunningJob(jobList, jobNumber);
    }

    // a background job was found, so we will bring it to the foreground
    if (tempPID != -1){
        waitpid(tempPID, NULL,0);
    }
}

// Native jobs implementation 
void jobsFunction(struct RunningJobs *jobArray){
    listRunningJobs(jobArray);
}

// ================================================= Main =================================================

int main(void)
{
    // Initializing variables 
    char *builtInCommands[] = {"echo", "cd", "pwd", "exit", "fg", "jobs"}; //Array holding built-in command strings 
    char *args[20]; //Holding the arguments of each inputted command
    char* temp_args[10]; //Used to store the left and right side of a pipe (cmd1) | (cmd2)
    int bg;  //Boolean indicating whether a process should be run in bg(1) or fg(0)
    int fd[2]; //Used for piping and output redirection
    int processCount = 0; 

    // Initializing the head of the joblist linked list
    struct RunningJobs* jobList = NULL; 

    pid_t pid; //PID of the child process that is created when fork() is called 
    pid_t secondChildPID; //PID of the second child process that is created when a pipe is requested 

    // Polling for user input
    while(1) {
        bg = 0;

        // Clearing the content of the args array to avoid reusing the previous arguments
        memset(args, 0, 20 * sizeof(char));

        int cnt = getcmd("\n>> ", args, &bg); 
        
        if (cnt > 0){
            // Case where a built-in function is being called 
            if (isBuiltIn(args[0], builtInCommands) == 1) {
                if (strcmp(args[0], "echo") == 0){
                    echoFunction(args, cnt);
                }
                else if (strcmp(args[0], "cd") == 0){
                    if (cdFunction(args[1]) == 0){
                        pwdFunction(); 
                    }
                }
                else if (strcmp(args[0], "pwd") == 0){
                    pwdFunction();
                }
                else if (strcmp(args[0], "exit") == 0){
                    exitFunction(jobList);
                }
                else if (strcmp(args[0], "fg") == 0){
                    // Case where no parameters were given to fg (bring first job in the list to fg)
                    if (args[1] == NULL){
                        fgFunction(&jobList, -1, &processCount); 
                    }
                    // Case where a number was given to fg
                    else{
                        fgFunction(&jobList, atoi(args[1]), &processCount);
                    }
                }
                else{
                    jobsFunction(jobList);
                }
            }
            // Case where an external shell function is called 
            else{ 
                pid = fork();

                // Case where the forking failed
                if (pid < 0){
                    perror("Fork Failed");
                    return 1; 
                }
                // Case where we're in the child process
                else if (pid == 0){
                    
                    // Sub Case: Output redirection was requested
                    if (redirectRequested(args, cnt) == 1){
                        
                        close(1); 
                        int fd = open(args[cnt-1], O_CREAT|O_WRONLY|O_TRUNC, 0666); 

                        // need to remove all the characters after, and including ">"
                        for (int i = 0; i<cnt; i++){
                            if (strcmp(args[i], ">") == 0){
                                args[i] = NULL; 
                                cnt = i; 
                            }
                        }

                        execvp(args[0], args);
                        perror("Command failed");
                        break; //Avoids getting stuck in the loop if invalid command is being redirected
                         
                    }

                    // Sub Case: Piping was requested
                    else if (pipeRequested(args, cnt) == 1){
                        
                        // Creating the pipe structure
                        if (pipe(fd) == -1){
                            perror("Error occurred during piping\n");
                            return 2; 
                        }

                        secondChildPID = fork(); 

                        if (secondChildPID < 0){
                            perror("Fork failed (piping)");
                            return 3; 
                        }

                        // Inside of the second child
                        if (secondChildPID == 0){
                            
                            // Clearing content of temp_args to avoid reusing old arguments 
                            memset(temp_args, 0, 10 * sizeof(char));

                            // Obtaining the arguments for the command before the pipe ***(cmd1)** | (cmd2)
                            for (int i = 0; i < cnt; i++){
                                if (strcmp(args[i], "|") == 0){
                                    temp_args[i] = NULL;
                                    break;
                                }
                                else{
                                    temp_args[i] = args[i];
                                }
                            }

                            // Rerouting the pipe
                            close(1); 
                            dup(fd[1]);
                            close(fd[0]);

                            execvp(temp_args[0],temp_args);
                            perror("Command failed");
                            break; //Avoids getting stuck in the loop if invalid command is being piped
                        }

                        // back to the first child (parent of second child)
                        else{

                            // Rerouting the pipe
                            close(0); 
                            dup(fd[0]);
                            close(fd[1]);
                            
                            // Clearing content of temp_args from the previous usage 
                            memset(temp_args, 0, 10 * sizeof(char));

                            int temp_index = 0; 
                            int foundRedirection = 0; 

                            // Obtaining the arguments for the command before the pipe (cmd1) | ***(cmd2)***
                            for (int i = 0; i < cnt; i++){
                                
                                // Identifying when the pipe has been encountered 
                                if (strcmp(args[i], "|") == 0){
                                    foundRedirection = 1; 
                                }
                                // Obtaining the command after the pipe 
                                else if (foundRedirection == 1){
                                    temp_args[temp_index] = args[i];
                                    temp_index = temp_index + 1; 
                                }
                            }
                            temp_args[temp_index] = NULL;

                            execvp(temp_args[0],temp_args);
                            perror("Command failed");
                            break; //Avoids getting stuck in the loop if invalid command is being piped
                        }
                    }
                    // Normal execution (no pipes or output redirection, just external shell command)
                    else{
                        args[cnt] = NULL; 
                        execvp(args[0], args); 
                        perror("Command failed");
                    }                   
                }
                // Case where we're in the parent process
                else{
                    // Case where we want to run the child process in the background 
                    if (bg == 1){
                        addJobToBackgroundList(&jobList, pid, args, cnt, &processCount);
                    }
                    // Case where we want the parent to wait on the child process to end 
                    else{
                        waitpid(pid, NULL, 0); 
                    }
                }
            }
        }
    }
}


