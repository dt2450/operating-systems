# A Simple Shell for Linux

This is the readme file for Devashi's shell. 

Please remember to add path using special command 'path', before trying to run any shell commands.

# Notes:

1. Assumption: The shell supports at most 1023 arguments (last argument is NULL so the array of pointers fits a page on a 32-bit system, hence this number was chosen). 
2. pipe is also treated as another argument and will be counted while counting the total number of arguments.
3. If the total arguments including the pipes are within the limit, the shell can process a line of any size.
4. Assumption: A new line in the input buffer is treated as end of command. If there is any input after a new line it will be treated as a new command. 
   This has a limitation since when very large line is pasted onto the shell, I was observing a new line getting inserted in between leading to the failure of the command.
5. path [+/- /some/dir]: multiple directories cannot be specified in a single command.
6. Shell has been tested for recursion as well and seen to work fine as displayed in the testing results below. Both bash and our shell can be run from within the shell 
   and they work fine as expected. On exiting the recursive shell we return to the previous shell.
7. FD Leak has been fixed so that all FDs get closed (besides 0,1 and 2) once the command with/without pipes has been executed. Without the fix, every command leaves an 
   open read fd per pipe in the command.
8. The list of paths is implemented as a linked list. Duplicates are not handled so the path can contain duplicate paths if they are added again. 
   The command to be executed is searched from the first path onwards, so if a binary is present in more than one paths, the first path in the list that has 
   it will be taken and the binary will be executed from that path. To pick the binary from a different path, the path of the binary can be specified in the 
   shell command or the list of paths can be updated (delete/re-add paths) to be in the order in which they should be taken.
9. If a very large path is specified for the binary in the command line, the access function may fail and the shell will not be able to process the command.
10.Pipe is not supported as an argument to the shell itself. So the command ./w4118_sh|grep abc won't work. It will lead to the shell getting stuck.
   This behaviour is similar to "sh" shell. I tried to fix this but wasn't very successful. However, commands like ./w4118_sh abc will return appropriate errors. 

# References:

1. http://stackoverflow.com/questions/1138508/in-c-on-unix-how-can-a-process-tell-what-permissions-it-has-to-a-file-without-o
2. http://www.vim.org/scripts/script.php?script_id=4369
3. http://web.cse.ohio-state.edu/~mamrak/CIS762/pipes_lab_notes.html
4. http://stackoverflow.com/questions/8082932/connecting-n-commands-with-pipes-in-a-shell
5. Extensively used Linux Man pages

# Code Documentation:

# Files: 
     shell.c (This file contains the complete code for the custom shell)

# Functions:

1.  char *update_buffer(char c, bool do_i_free)
    This function maintains a buffer and a call to the function can add a character c to the buffer or free the buffer if do_i_free is true.
    At a given time the function maintains a single buffer. Hence two buffers cannot be maintained simultaneously with this function.
    The buffer grows in size if the current size is not sufficient to hold the data. It returns a pointer to the buffer or NULL on error or when buffer is freed.

2.  char *copy_str_to_buffer(char *str)
    This function supplements the update_buffer function. It adds a string to the buffer instead of adding a character. It uses the same buffer
    as update_buffer. It returns a pointer to the buffer or NULL on error.

3.  int parse_args(char **arguments)
    This function parses the input stream and returns the number of arguments found. It also updates the arguments array passed to the function.
    Memory is allocated for each argument and the arguments array contains the pointers to those strings. Once the processing is done, the memory should
    be freed. This function only frees the memory on error.

4.  void print_path(void)
    This function prints the paths currently held in the system (which were added by the path command)
    
5.  void process_path_cmd(char **arguments)
    This function processes the special path command i.e it adds or deletes paths from the list.

6.  bool process_special_cmds(char **arguments, unsigned int arg_num)
    This function checks if the input command is a special command and if yes it processes it and continues. Pipes are not supported for special commands.
    It returns true if the command was a special command, otherwise it returns false.

7.  bool verify_binary(char *binary_name, char **buffer)
    This function checks whether the specified command(s) in the shell command line exist in one of the paths and has execute permissions. If the path was specified
    in the command line then it looks for the binary in that path instead of looking in the list of paths. It returns true if the binary was found and has execute
    permissions.

8.  char **more_commands(char **current_args)
    This function checks if the list of arguments passed to it in current_args array, contains pipe and more commands after pipe. It sets the pointer at the first
    pipe to NULL, and returns the pointer to the array containing the arguments after the first pipe it found. If there are no more pipes, it returns NULL.

9.  bool process_regular_cmds(char **arguments, unsigned int arg_num)
    This function processes the regular commands (which are not special commands) and executes them. If there are pipes, it executes the commands in series, creating
    pipes for each process and managing the data flow through the fd's. It returns true on success and false on failure.

10. int main(int argc, char **argv)
    This is the main function of the shell which runs an infinite while loop looking for more commands in the command line. The process terminates only on exit command
    otherwise it displays a prompt to the user, waiting for the next set of inputs.    

# Testing Results:

w4118@w4118:~/hmwk1/shell$ ./w4118_sh 
$    
$     
$                                   
$
$ls
error: path is empty
$/bin/ls
a.out  Makefile  README  shell.c  shell.o  w4118_sh
$ps
error: path is empty
$path + abcd
error: No such file or directory
$path + /abcd
error: No such file or directory
$path - /abcd
error: The directory is not present in the path list
$path + /bin
$path - /abcd
error: The directory is not present in the path list
$path - /bin
$path

$path + /usr/sbin
$path + /bin
$path + /usr/bin
$path    
/usr/sbin:/bin:/usr/bin
$path - /bin
$path
/usr/sbin:/usr/bin
$ls
error: No such file or directory
$path + /bin
$ls
a.out  Makefile  README  shell.c  shell.o  w4118_sh
$pwd
/home/w4118/hmwk1/shell
$cd ..
$pwd
/home/w4118/hmwk1
$cd shell
$ls
a.out  Makefile  README  shell.c  shell.o  w4118_sh
$pwd
/home/w4118/hmwk1/shell
$cd
error: Directory path is missing for the cd command
$     ls       
a.out  Makefile  README  shell.c  shell.o  w4118_sh
$        ls
a.out  Makefile  README  shell.c  shell.o  w4118_sh
$ls        
a.out  Makefile  README  shell.c  shell.o  w4118_sh
$ls        shell.c
shell.c
$ls      shell
ls: cannot access shell: No such file or directory
$     ls     shell.c
shell.c
$|
error: syntax error unexpected token '|'
$|ls
error: syntax error unexpected token '|'
$ls|
error: command cannot end with pipe
$    ls   |
error: command cannot end with pipe
$    |    ls
error: syntax error unexpected token '|'
$ls|grep shell
shell.c
shell.o
$ls   |    grep   shell
shell.c
shell.o
$ls||||grep shell
error: syntax error unexpected token '|'
$ls||grep shell
error: syntax error unexpected token '|'
$ls|grep shell|grep c|grep shell
shell.c
$ls|grep shell|grep c|xargs grep Devashi
/* Code written by Devashi Tandon */
$ls|grep shell|grep c|xargs grep for
	bool has_forward_pipe = false;
				has_forward_pipe = true;
			if (has_forward_pipe == true) {
				has_forward_pipe = false;
			/* read from start of buffer for the next argument */
	for (i = 0; i < arg_num; i++) {
		printf("Usage for path command: path [+/- /some/dir]\n");
	for (i = 0; i < special_cmds_num; i++) {
					"error: No arguments are expected for"
					printf("missing for the cd command\n");
					printf("expected for cd command\n");
					printf("Usage for path command:");
			int pid = fork();
				* stdin for the next command
				/* fork failed */
	for (i = 0; i < level; i++) {
	for (i = 0; i < level; i++) {
		/* run this loop forever to keep processing commands
		for (i = 0; i < arg_num; i++) {
$ls|grep shell|grep c|xargs grep "for "     
grep: ": No such file or directory
$cd ~
error: No such file or directory
$ls|grep shell|grep c|xargs grep for|grep (
			if (has_forward_pipe == true) {
	for (i = 0; i < arg_num; i++) {
		printf("Usage for path command: path [+/- /some/dir]\n");
	for (i = 0; i < special_cmds_num; i++) {
					printf("missing for the cd command\n");
					printf("expected for cd command\n");
					printf("Usage for path command:");
			int pid = fork();
	for (i = 0; i < level; i++) {
	for (i = 0; i < level; i++) {
		for (i = 0; i < arg_num; i++) {
$ls|grep shell|grep c|xargs grep Devashi   
/* Code written by Devashi Tandon */
$ls|grep shell|grep c|xargs grep Devashi|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written
/* Code written by Devashi Tandon */
$ls|grep shell|grep c|xargs grep Devashi|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written|grep written
/* Code written by Devashi Tandon */
$ls|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell
shell.c
shell.o
$ls|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell|grep shell
error: No such file or directory
$error: Only 1023 arguments are allowed in this shell
$/usr/bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/../bin/nslookup www.google.com
Server:		192.168.185.2
Address:	192.168.185.2#53

Non-authoritative answer:
Name:	www.google.com
Address: 173.194.123.19
Name:	www.google.com
Address: 173.194.123.18
Name:	www.google.com
Address: 173.194.123.17
Name:	www.google.com
Address: 173.194.123.16
Name:	www.google.com
Address: 173.194.123.20


$bash
w4118@w4118:~/hmwk1/shell$ ls
a.out  Makefile  README  shell.c  shell.o  w4118_sh
w4118@w4118:~/hmwk1/shell$ pwd
/home/w4118/hmwk1/shell
w4118@w4118:~/hmwk1/shell$ exit
exit
$./w4118_sh
$ls
error: path is empty
$path + /bin
$ls
a.out  Makefile  README  shell.c  shell.o  w4118_sh
$ls|grep shell
shell.c
shell.o
$exit
$ls
a.out  Makefile  README  shell.c  shell.o  w4118_sh
$path
/bin:/usr/bin
$exit
w4118@w4118:~/hmwk1/shell$ 
