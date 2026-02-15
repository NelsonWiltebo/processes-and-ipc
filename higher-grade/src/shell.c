#include "parser.h" // cmd_t, position_t, parse_commands()

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h> //fcntl(), F_GETFL

#define READ 0
#define WRITE 1

/**
 * For simplicitiy we use a global array to store data of each command in a
 * command pipeline .
 */
cmd_t commands[MAX_COMMANDS];

/**
 *  Debug printout of the commands array.
 */
void print_commands(int n)
{
  for (int i = 0; i < n; i++)
  {
    printf("==> commands[%d]\n", i);
    printf("  pos = %s\n", position_to_string(commands[i].pos));
    printf("  in  = %d\n", commands[i].in);
    printf("  out = %d\n", commands[i].out);

    print_argv(commands[i].argv);
  }
}

/**
 * Returns true if file descriptor fd is open. Otherwise returns false.
 */
int is_open(int fd)
{
  return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

void fork_error()
{
  perror("fork() failed)");
  exit(EXIT_FAILURE);
}

/**
 *  Fork a proccess for command with index i in the command pipeline. If needed,
 *  create a new pipe and update the in and out members for the command..
 */
void fork_cmd(int i, int *prev_read)
{
  pid_t pid;

  cmd_t command = commands[i];

  int fd[2];
  if ((command.pos == first || command.pos == middle) && pipe(fd) == -1)
  {
    perror("Creation of pipe failed");
    exit(EXIT_FAILURE);
  }

  switch (pid = fork())
  {
  case -1:
    fork_error();
  case 0:
    // Child process after a successful fork().

    if (command.pos != single)
    {
      if (command.pos != last)
      {
        close(fd[READ]);
      }

      if (command.pos == first)
      {
        command.out = fd[WRITE];
        if (dup2(command.out, STDOUT_FILENO) == -1)
        {
          perror("Redirecting of stdout to first pipeline write failed!");
          exit(EXIT_FAILURE);
        }
        close(fd[WRITE]);
      }
      else if (command.pos == middle)
      {
        command.in = *prev_read;
        command.out = fd[WRITE]; // current pipelines write
        if (dup2(command.in, STDIN_FILENO) == -1)
        {
          perror("Redirecting of stdin to middle pipeline write failed!");
          exit(EXIT_FAILURE);
        }
        close(command.in);
        if (dup2(command.out, STDOUT_FILENO) == -1)
        {
          perror("Redirecting of stdout to middle pipeline write failed!");
          exit(EXIT_FAILURE);
        }
        close(fd[WRITE]);
      }
      else if (command.pos == last)
      {
        command.in = *prev_read;
        if (dup2(command.in, STDIN_FILENO) == -1)
        {
          perror("Redirecting of stdin to last pipeline write failed!");
          exit(EXIT_FAILURE);
        }
        close(command.in);
      }
      else
      {
        // Do nothing
      }
    }
    else
    {
      // Do nothing
    }

    // Execute the command in the contex of the child process.
    execvp(commands[i].argv[0], commands[i].argv);

    // If execvp() succeeds, this code should never be reached.
    fprintf(stderr, "shell: command not found: %s\n", commands[i].argv[0]);
    exit(EXIT_FAILURE);

  default:
    // Parent process after a successful fork().

    if (*prev_read != -1)
    {
      close(*prev_read);
    }

    if (command.pos != last && command.pos != single)
    {
      *prev_read = fd[READ];
    }
    close(fd[WRITE]);

    break;
  }
}

/**
 *  Fork one child process for each command in the command pipeline.
 */
void fork_commands(int n)
{
  int prev_read = -1;
  for (int i = 0; i < n; i++)
  {
    fork_cmd(i, &prev_read);
  }
}

/**
 *  Reads a command line from the user and stores the string in the provided
 *  buffer.
 */
void get_line(char *buffer, size_t size)
{
  getline(&buffer, &size, stdin);
  buffer[strlen(buffer) - 1] = '\0';
}

/**
 * Make the parents wait for all the child processes.
 */
void wait_for_all_cmds(int n)
{
  for (int i = 0; i < n; ++i)
  {
    wait(NULL);
  }
}

int main()
{
  int n;             // Number of commands in a command pipeline.
  size_t size = 128; // Max size of a command line string.
  char line[size];   // Buffer for a command line string.

  while (true)
  {
    printf(" >>> ");

    get_line(line, size);

    n = parse_commands(line, commands);

    fork_commands(n);

    wait_for_all_cmds(n);
  }

  exit(EXIT_SUCCESS);
}
