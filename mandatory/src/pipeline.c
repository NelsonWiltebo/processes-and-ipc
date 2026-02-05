#include <stdio.h>    // puts(), printf(), perror(), getchar()
#include <stdlib.h>   // exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>   // getpid(), getppid(), fork()
#include <sys/wait.h> // wait()

#define READ 0
#define WRITE 1

void child_a(int fd[])
{
  close(fd[READ]); // only writes
  if(dup2(fd[WRITE], STDOUT_FILENO) == -1) {
    perror("Redirecting of stdout to pipeline write failed!");
    exit(EXIT_FAILURE);
  }
  execlp("ls", "ls", "-F", "-1", NULL);
  perror("Exec 'ls' of child A failed!");
  exit(EXIT_FAILURE);
}

void child_b(int fd[])
{
  close(fd[WRITE]); // only reades
  if(dup2(fd[READ], STDIN_FILENO) == -1) {
    perror("Redirecting of stdin to pipeline read failed!");
    exit(EXIT_FAILURE);
  }
  execlp("nl", "nl", NULL);
  perror("Exec 'nl' of child B failed!");
  exit(EXIT_FAILURE);
}

int main(void)
{
  int fd[2];

  if (pipe(fd) == -1)
  {
    perror("Creation of pipe failed");
    exit(EXIT_FAILURE);
  }

  int pid0 = fork();
  if (pid0 == 0)
  {
    child_a(fd);
  }
  else if (pid0 == -1)
  {
    perror("Child A fork unsuccessful");
    exit(EXIT_FAILURE);
  }
  else {}

  int pid1 = fork();
  if (pid1 == 0)
  {
    child_b(fd);
  }
  else if (pid1 == -1)
  {
    perror("Child B fork unsuccessful");
    exit(EXIT_FAILURE);
  }
  else {}

  close(fd[READ]);
  close(fd[WRITE]);

  wait(NULL);
  wait(NULL);

  exit(EXIT_SUCCESS);
}
