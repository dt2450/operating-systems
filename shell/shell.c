/* Code written by Devashi Tandon */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#define MAX_ARGUMENTS   1023
#define BUFFER_INCR_SIZE 256

struct path_node {
	struct path_node *next;
	char *path_dir;
};

const unsigned int special_cmds_num = 3;
char *special_cmds[] = {"exit", "cd", "path"};
struct path_node *path_head = NULL;

/* this function maintains its own character buffer
* which can expand in increments of BUFFER_INCR_SIZE
* as more data is added. The function keeps adding to
* the existing buffer until it is freed
* (new buffer will be allocated and used after free).
*/
char *update_buffer(char c, bool do_i_free)
{
	static unsigned int current_buffer_size;
	static unsigned int current_char_pos;
	static char *buffer;

	if (do_i_free) {
		/* just free the existing buffer and return */
		free(buffer);
		current_char_pos = 0;
		current_buffer_size = 0;
		buffer = NULL;
		return NULL;
	}
	if (current_char_pos >= current_buffer_size) {
		/* the buffer is full. need to reallocate */
		char *new_buffer = (char *)realloc(buffer, current_buffer_size +
				BUFFER_INCR_SIZE);
		if (new_buffer) {
			current_buffer_size = current_buffer_size +
				BUFFER_INCR_SIZE;
			buffer = new_buffer;
		} else {
			return NULL;
		}
	}
	buffer[current_char_pos++] = c;
	return buffer;
}

/* use update_buffer to copy a string to it's buffer */
char *copy_str_to_buffer(char *str)
{
	char *new_str = NULL;

	while (*str != '\0') {
		new_str = update_buffer(*str++, false);
		if (new_str == NULL)
			return NULL;
	}
	return new_str;
}

int parse_args(char **arguments)
{
	unsigned int i = 0;
	unsigned char current_char = '\0';
	unsigned char previous_char = '\0';
	unsigned int arg_num = 0;
	bool has_forward_pipe = false;
	bool last_argument = false;
	char *arg_buffer = update_buffer('\0', true);

	/* parse the arguments */
	current_char = getchar();
	if (current_char == '\n')
		/* no input */
		return 0;
	do {
		if (current_char == '\t')
			/* treat tab as space */
			current_char = ' ';
		if (current_char == '|') {
			/* check if pipe is at a valid position */
			if ((arg_num == 0 && i == 0) ||
					(previous_char == current_char) ||
				(previous_char == ' ' &&
				arg_num > 0 &&
				!strcmp(arguments[arg_num-1], "|"))) {
				printf(
				"error: syntax error unexpected token '|'\n");
				goto error;
			} else if (previous_char != ' ') {
				/* pipe is within the current argument */
				current_char = ' ';
				previous_char = '|';
				has_forward_pipe = true;
			} else
				previous_char = current_char;
		} else if (current_char == '\n') {
			/* we have reached the last argument */
			current_char = ' ';
			previous_char = '\n';
			last_argument = true;
		} else
			previous_char = current_char;
		if (i == 0 && current_char == ' ') {
			/* first character is space. Ignore this argument */
			if (last_argument)
				break;
			continue;
		} else {
			/* add the character to the buffer */
			arg_buffer = update_buffer(current_char, false);
			if (arg_buffer == NULL) {
				printf("error: memory allocation failed\n");
				goto error;
			}
			i++;
		}

		if (current_char == ' ') {
			/* end of parsing of current argument */
			if (arg_num >= MAX_ARGUMENTS) {
				printf("error: Only %d arguments are allowed",
						MAX_ARGUMENTS);
				printf(" in this shell\n");
				goto error;
			}

			if (i == 0) {
				/* safety check. code should never reach here */
				printf("error: Failure in ");
				printf(" parsing argument %d\n"
						, arg_num+1);
				goto error;
			}
			arg_buffer[i-1] = '\0';

			if (arg_buffer[0] == '|') {
				/* the argument starts with pipe. split it */
				arguments[arg_num] = (char *)
					malloc(sizeof(char)*(2));
				if (arguments[arg_num] == NULL) {
					printf("error: %s\n", strerror(errno));
					goto error;
				}
				arguments[arg_num][0] = '|';
				arguments[arg_num][1] = '\0';
				arg_num++;
				/* actual argument after pipe */
				if (i > 2) {
					/* there is some string beyond "|" */
					arguments[arg_num] = (char *)
						malloc(sizeof(char)*(i-1));
					if (arguments[arg_num] == NULL) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
					strncpy(arguments[arg_num],
							arg_buffer+1, i-1);
					if (arguments[arg_num][i-2] != '\0') {
						printf("error: Failure in ");
						printf(" parsing argument %d\n"
								, arg_num+1);
						goto error;
					}
					arg_num++;
				}
			} else {
				/* copy the current argument
				* to the argument list
				*/
				arguments[arg_num] = (char *)
					malloc(sizeof(char)*(i));
				if (arguments[arg_num] == NULL) {
					printf("error: %s\n", strerror(errno));
					goto error;
				}
				strncpy(arguments[arg_num], arg_buffer, i);
				if (arguments[arg_num][i-1] != '\0') {
					printf("error: Failure in ");
					printf(" parsing argument %d\n"
							, arg_num+1);
					goto error;
				}
				arg_num++;
			}
			if (has_forward_pipe == true) {
				/* there is a pipe at the end of the current
				* argument. add it as the next argument */
				if (arg_num >= MAX_ARGUMENTS) {
					printf("error: Only %d arguments are",
							MAX_ARGUMENTS);
					printf(" allowed in this shell\n");
					goto error;
				}
				arguments[arg_num] = (char *)
					malloc(sizeof(char)*(2));
				if (arguments[arg_num] == NULL) {
					printf("error: %s\n", strerror(errno));
					goto error;
				}
				arguments[arg_num][0] = '|';
				arguments[arg_num][1] = '\0';
				arg_num++;
				has_forward_pipe = false;
			}
			/* read from start of buffer for the next argument */
			i = 0;
			arg_buffer = update_buffer('\0', true);
		}
		if (last_argument)
			break;
	} while ((current_char = getchar()) != EOF);

	if (previous_char == 255) {
		printf("error: unexpected end of file\n");
		goto error;
	}
	if (arg_num && arguments[arg_num-1][0] == '|') {
		/* last argument is a pipe */
		printf("error: command cannot end with pipe\n");
		goto error;
	}

	return arg_num;

error:
	/* free the allocated memory */
	for (i = 0; i < arg_num; i++) {
		if (arguments[i] != NULL) {
			free(arguments[i]);
			arguments[i] = NULL;
		}
	}
	/* flush rest of the input */
	if (previous_char != '\n' && previous_char != EOF &&
			previous_char != 255) {
		while ((current_char = getchar()) != '\n'
				&& current_char != EOF)
			;
	}
	/* return error */
	return -1;
}

void print_path(void)
{
	/* print the path */
	struct path_node *temp = path_head;

	while (temp) {
		printf("%s", temp->path_dir);
		if (temp->next)
			printf(":");
		temp = temp->next;
	}
	printf("\n");
}

void process_path_cmd(char **arguments)
{
	static struct path_node *path_tail;

	if (strcmp(arguments[1], "+") == 0) {
		struct path_node *temp_node = NULL;
		/* verify the directory exists */
		if (access(arguments[2], F_OK)
				!= 0) {
			printf("error: %s\n", strerror(errno));
			return;
		}
		/* add the specified directory
		* to the path list */
		temp_node = (struct path_node *)
			malloc(sizeof(struct path_node));
		if (temp_node == NULL) {
			printf("error: %s\n",
					strerror(errno));
		} else {
			temp_node->next = NULL;
			temp_node->path_dir = strdup(arguments[2]);
			if (path_head == NULL) {
				path_head = temp_node;
				path_tail = path_head;
			} else {
				path_tail->next = temp_node;
				path_tail = temp_node;
			}
		}
	} else if (strcmp(arguments[1], "-") == 0) {
		/* remove the specified directory
		* from the path list */
		struct path_node *temp_node = path_head;
		struct path_node *prev_node = NULL;
		bool found = false;

		while (temp_node) {
			if (strcmp(arguments[2],
						temp_node->path_dir) == 0) {
				found = true;
				if (temp_node == path_head) {
					path_head = path_head->next;
					free(temp_node->path_dir);
					free(temp_node);
					temp_node = path_head;
					if (path_head == NULL)
						path_tail = NULL;
					continue;
				} else {
					if (path_tail == temp_node)
						path_tail = prev_node;
					prev_node->next = temp_node->next;
					free(temp_node->path_dir);
					free(temp_node);
					temp_node = prev_node->next;
					continue;
				}
			}
			prev_node = temp_node;
			temp_node = temp_node->next;
		}
		if (!found) {
			printf("error: The directory is not present in the");
			printf(" path list\n");
		}
	} else {
		/* Invalid second argument */
		printf("error: Usage for path command: path [+/- /some/dir]\n");
	}
}


bool process_special_cmds(char **arguments, unsigned int arg_num)
{
	int i = 0;

	for (i = 0; i < special_cmds_num; i++) {
		if (strcmp(special_cmds[i], arguments[0]) == 0) {
			/* we have a match. Process the command */
			switch (i) {
			case 0:
				/* exit command */
				if (arg_num == 1) {
					exit(0);
				} else {
					printf(
					"error: No arguments are expected for"
					);
					printf(" exit command\n");
				}
				break;
			case 1:
				if (arg_num == 2) {
					/* process cd command */
					if (chdir((const char *)arguments[1])
							!= 0) {
						printf("error: %s\n",
							strerror(errno));
					}
				} else if (arg_num == 1) {
					printf("error: Directory path is ");
					printf("missing for the cd command\n");
				} else {
					printf("error: Only one argument is ");
					printf("expected for cd command\n");
				}
				break;
			case 2:
				/* path command */
				if (arg_num == 1) {
					print_path();
				} else if (arg_num == 3) {
					process_path_cmd(arguments);
				} else {
					printf("error: Usage for path command");
					printf(": path [+/- /some/dir]\n");
				}
				break;
			default:
				printf("error: Unknown special command\n");
				break;
			}
			return true;
		}
	}
	return false;
}

bool verify_binary(char *binary_name, char **buffer)
{
	struct path_node *binary_path_node = path_head;

	update_buffer('\0', true);

	if (strchr(binary_name, '/')) {
		/* path is specified in the command line */
		if (access(binary_name, X_OK) == 0) {
			/* the binary file exist and has execute permissions */
			*buffer = copy_str_to_buffer(binary_name);
			if (*buffer == NULL) {
				printf("error: memory allocation failed\n");
				return false;
			}
			*buffer = update_buffer('\0', false);
			if (*buffer == NULL) {
				printf("error: memory allocation failed\n");
				return false;
			}
		}
	} else {
		if (!binary_path_node) {
			printf("error: path is empty\n");
			return false;
		}
		/* check the existence/execute permission of binary
		* in the list of paths that we have
		*/
		while (binary_path_node) {
			*buffer =
			copy_str_to_buffer(binary_path_node->path_dir);
			if (*buffer == NULL) {
				printf("error: memory allocation failed\n");
				return false;
			}
			*buffer = update_buffer('/', false);
			if (*buffer == NULL) {
				printf("error: memory allocation failed\n");
				return false;
			}
			*buffer = copy_str_to_buffer(binary_name);
			if (*buffer == NULL) {
				printf("error: memory allocation failed\n");
				return false;
			}
			*buffer = update_buffer('\0', false);
			if (*buffer == NULL) {
				printf("error: memory allocation failed\n");
				return false;
			}
			if (access(*buffer, X_OK) == 0)
				/* the binary file exist and
				* has execute permissions */
				break;

			*buffer = update_buffer('\0', true);
			binary_path_node = binary_path_node->next;
		}
	}
	return true;
}

/* this function returns the next argument after pipe
* in case there are more commands. Otherwise it returns NULL
*/
char **more_commands(char **current_args)
{
	int i = 0;

	if (current_args == NULL)
		return NULL;

	while (current_args[i] != NULL) {
		if (!strcmp(current_args[i], "|")) {
			free(current_args[i]);
			current_args[i] = NULL;
			/* return the next command */
			return (current_args + i + 1);
		}
		i++;
	}
	return NULL;
}

bool process_regular_cmds(char **arguments, unsigned int arg_num)
{
	char *full_binary_path = NULL;
	char **new_args = arguments;
	char **next_args = NULL;
	unsigned int level = 0;
	int i = 0;
	bool last_command = false;

	if (arguments == NULL)
		return false;
	int pipe_fd[2] = {-1};
	int read_fd = fileno(stdin);
	int pending_fd_list[MAX_ARGUMENTS] = {-1};

	do {
		next_args = more_commands(new_args);

		if (next_args == NULL)
			last_command = true;
		/* process the current command */
		if (verify_binary(new_args[0], &full_binary_path) == false)
			goto error;

		if (full_binary_path != NULL) {
			if (pipe(pipe_fd) == -1) {
				printf("error: %s\n", strerror(errno));
				goto error;
			}

			int pid = fork();

			if (pid > 0) {
				/* parent process */
				int status = 0;

				/* Close the write fd in parent as it never
				* writes to the pipe */
				if (close(pipe_fd[1]) == -1) {
					printf("error: %s\n", strerror(errno));
					goto error;
				}

				/* save the read fd. This will be duplicated as
				* stdin for the next command
				*/
				read_fd = pipe_fd[0];
				pending_fd_list[level] = read_fd;

				if (wait(&status) == -1) {
					printf("error: %s\n", strerror(errno));
					goto error;
				}

				update_buffer('\0', true);
			} else if (pid == 0) {
				/* child process */
				if (level == 0 && !last_command) {
					/* first command in the series */
					if (dup2(pipe_fd[1], fileno(stdout))
							== -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
					if (close(pipe_fd[1]) == -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
					if (close(pipe_fd[0]) == -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
				} else if (level == 0 && last_command) {
					/* no pipes. this is the only command */
					if (close(pipe_fd[0]) == -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
					if (close(pipe_fd[1]) == -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
				} else if (last_command) {
					/* last command in the series */
					if (dup2(read_fd, fileno(stdin))
							== -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
					if (close(read_fd) == -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
					if (close(pipe_fd[1]) == -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
				} else {
					/* middle command */
					if (dup2(read_fd, fileno(stdin))
							== -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
					if (dup2(pipe_fd[1], fileno(stdout))
							== -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
					if (close(read_fd) == -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
					if (close(pipe_fd[1]) == -1) {
						printf("error: %s\n",
							strerror(errno));
						goto error;
					}
				}

				execv(full_binary_path, new_args);
				/* the following code will execute
				* only if execv fails
				*/
				printf("error: %s\n", strerror(errno));
				exit(-1);
			} else {
				/* fork failed */
				printf("error: %s\n", strerror(errno));
				goto error;
			}
		} else {
			/* verify_binary failed */
			printf("error: %s\n", strerror(errno));
			goto error;
		}
		new_args = next_args;
		level++;
	} while (new_args);

	for (i = 0; i < level; i++) {
		/* we need to close all the read fds we created in parent
		* so that they don't keep adding up with every command
		* execution
		*/
		if ((pending_fd_list[i] != -1) &&
			close(pending_fd_list[i]) == -1) {
			printf("error: %s\n", strerror(errno));
			return false;
		}
	}
	return true;

error:
	update_buffer('\0', true);
	for (i = 0; i < level; i++) {
		/* we need to close all the read fds we created in parent
		* so that they don't keep adding up with every command
		* execution
		*/
		if ((pending_fd_list[i] != -1) &&
			close(pending_fd_list[i]) == -1) {
			printf("error: %s\n", strerror(errno));
			return false;
		}
	}
	return false;
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		printf("error: The shell doesn't expect any arguments\n");
		return -1;
	}

	while (1) {
		/* run this loop forever to keep processing commands
		* as they are entered by user. only exit command will
		* exit the program
		*/
		char *args_list[MAX_ARGUMENTS + 1] = {0};
		int arg_num;
		int i = 0;

		/* display prompt */
		printf("dtshell-2.3$");

		arg_num = parse_args(args_list);

		if (arg_num <= 0) {
			/* No input or error in parsing arguments*/
			continue;
		}

		if (process_special_cmds(args_list, arg_num) == true) {
			/* we processed the command already */
			goto next;
		}

		process_regular_cmds(args_list, arg_num);

next:
		/* free the allocated memory */
		for (i = 0; i < arg_num; i++) {
			if (args_list[i] != NULL) {
				free(args_list[i]);
				args_list[i] = NULL;
			}
		}
	}
	return 0;
}
