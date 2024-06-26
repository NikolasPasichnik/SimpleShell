# SimpleShell

This project was done as part of the McGill Operating Systems course (ECSE427). It consists of a C program that implements a shell interface that accepts user commands and executes them in a separate process. The shell program assumes that the first string of the line gives the name of the command (like ls or echo), while the remaining strings in the line are considered arguments for the command (like -l or "hello world"). This shell also allows for processes to run in the background, or concurrently, by specifying an ampersand(`&`) at the end of the command.

# Built-in Commands 

A command is considered built-in, when all the functionality is implemented within the shell (i.e. calls a function instead of relying on an external program). All other commands, like `cat` or `ls`, call external programs to execute. Here is a list of the commands that have been implemented from scratch in this shell interface: 

- `echo` : Takes a string with spaces in between as an argument and prints string to the screen
- `cd` : Takes a single argument that is a string path and sets it as the current working directory. 
- `pwd` : Takes no argument and prints the current working directory.
- `exit` : Takes no argument, terminates the shell and any jobs 
that are running in the background.
- `fg` : Takes an optional integer parameter as an argument. The background can 
contain many jobs you could have started with & when calling them. The fg command should be called with a number (e.g., fg 3) and it will pick the job numbered 3 (assuming it exists) and put it in the foreground. 
- `jobs` :  Takes no argument and lists all the jobs that are running in the 
background with their number identifier.

# Other Features 

- Simple Output Redirection

This shell allows for basic output redirection using the `>` character with the `[source] > [destination]` format. 
If you type `ls > out.txt`, the output of the `ls` command should be sent to the out.txt file.

- Simple Command Piping 

This shell also allows for basic command piping using the '|' character with the `[command1] | [command2]` format. 
If you type `ls | wc -l`, the output of the `ls` command should be sent to the `wc -l`. 

# How to run

You can compile the SimpleShell.c file with `gcc -o SimpleShell SimpleShell.c`, and you can run the resulting and/or already existing executable with `./SimpleShell`
