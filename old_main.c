#include <string.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <stdio.h>
#include <unistd.h>

#define PIPE_WRITE 1
#define PIPE_READ 0
#define STDIN 0
#define STDOUT 1

char **cut_av(char **av, int start, int stop);

void    print_cmd(char **cmd)
{
    int i;
    i = 0;
    while(cmd[i])
    {
        printf("cmd%d: %s\n", i, cmd[i]);
        i++;
    }
    printf("endofcmd\n");
}

void exec_cmd(char **cmd, char **env)
{
    execve(cmd[0], cmd, env);
    printf("problem\n");
}

int main(int ac, char **av, char **env)
{
    int i;
    int start;
    int num_cmd;
    char **cmd;
    (void)ac;
    i = 1;
    num_cmd = 0;
    start = 0;
    int fd[2];
    // int fda[2];
    pipe(fd);
    // pipe(fda);
    while (av[i])
    {
        if (strcmp(av[i], "|") == 0 && i > start + 1)
        {
            cmd = cut_av(av, start + 1, i - 1);
            if (fork() == 0)
            {
                dup2(fd[PIPE_WRITE], STDOUT_FILENO);
                close(fd[PIPE_READ]);
                close(fd[PIPE_WRITE]);
                exec_cmd(cmd, env);
            }
            start = i;
            num_cmd++;
        }
        i++;
    }
    cmd = cut_av(av, start + 1, i - 1);
    if (fork() == 0)
    {
        dup2(fd[PIPE_READ], STDIN_FILENO);
        close(fd[PIPE_READ]);
        close(fd[PIPE_WRITE]);
        exec_cmd(cmd, env);
    }
    close(fd[0]);
    close(fd[1]);
    i = 0;
    while(i < num_cmd)
    {
        waitpid(-1, NULL, 0);
        i++;
    }
}

char **cut_av(char **av, int start, int stop)
{
    int i;
    i = 0;
    while (i + start <= stop)
    {
        av[i] = av[i + start];
        i++;
    }
    av[i] = NULL;
    return(av);
}