#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#define READ_END 0
#define WRITE_END 1
#define EXIT_FAILURE_SYSTEM -1

void ft_putstr(char *av)
{
    int len = 0;
    while (av[len])
        len++;
    write(1, av, len);
}

void ft_putstr_err(char *av)
{
    int len = 0;
    while (av[len])
        len++;
    write(2, av, len);
}

void print_av(char **av)
{
    int i = 0;
    ft_putstr("-- cmd --\n");
    while(av[i])
    {
        ft_putstr(av[i]);
        ft_putstr("\n");
        i++;
    }
}

char **cut_av(char **av, int start, int stop)
{
    int i;
    i = 0;
    // printf("%d : %s \n", start, av[start]);
    // printf("%d : %s \n", stop, av[stop]);
    while (i + start < stop)
    {
        av[i] = av[start + i];
        i++;
    }
    av[i] = 0;
    return av;
}

int exec_cmd(char **cmd, char **env)
{
    execve(cmd[0], cmd, env);
    ft_putstr_err("error: cannot execute ");
    ft_putstr_err(cmd[0]);
    ft_putstr_err("\n");
    exit(EXIT_FAILURE); // exit 1 in case of error
}


int exit_fatal()
{
    ft_putstr_err("error fatal\n");
    return (EXIT_FAILURE_SYSTEM);
}

void close_pipes(int fd[][2], int num)
{
    int i = 0;
    while (i < num)
    {
        close(fd[i][0]);
        close(fd[i][1]);
        i++;
    }
}

int exec_pipe(char **av, char **env)
{
    char **cmd;
    int ret;
    int i = 1;
    int start = 0;
    int count = 0;
    int status;
    int pid;
    int nb_pipes = 0;

    // 0 - get size of av
    int ac = 0;
    while (av[ac])
        ac++;
    if (ac == 0) // case of sucessive ";", or ";" followed or preceded by nothing
        return(EXIT_SUCCESS);
    
    // 1 - handle cd
    if (strcmp(av[0], "cd") == 0)
    {
        ret = chdir(av[1]);
        if (ac != 2)
        {
            ft_putstr_err("error: cd: bad arguments");
            ft_putstr_err("\n");
            return (EXIT_FAILURE);
        }
        if (ret == -1)
        {
            ft_putstr_err("error: cd: cannot change directory to ");
            ft_putstr_err(av[1]);
            ft_putstr_err("\n");
            return (EXIT_FAILURE);
        }
        return (EXIT_SUCCESS);
    }

    // 2 - count pipes
    i = 0;
    while (av[i])
    {
        if (strcmp(av[i], "|") == 0)
            nb_pipes++;
        i++;
    }

    int fd[2];
    int fd_prev[2];

    // 3 - cut av into commands using "|" as a separator
    // for each command: pipe, fork dup and exec
    i = 0;
    while(i < ac + 1)
    {
        if (av[i] == NULL || strcmp(av[i], "|") == 0)
        {
            cmd = cut_av(av, start, i);

            // each subprocess has maximum two sets of pipes (4 fds) open:
            // - fd
            // - fd_prev
            // (the 1st subprocess has only fd open)
            fd_prev[READ_END] = fd[READ_END];
            fd_prev[WRITE_END] = fd[WRITE_END];
            pipe(fd);

            if ((pid = fork()) == 0)
            {
                if (nb_pipes != 0) // no dup if there is no pipe
                {
                    if (count == 0)
                    {
                        if (dup2(fd[WRITE_END], STDOUT_FILENO) == -1)
                            return (exit_fatal());
                    }
                    else if (count == nb_pipes)
                    {
                        if (dup2(fd_prev[READ_END], STDIN_FILENO) == -1)
                            return (exit_fatal());
                    }
                    else
                    {
                        if (dup2(fd[WRITE_END], STDOUT_FILENO) == -1)
                            return (exit_fatal());
                        if (dup2(fd_prev[READ_END], STDIN_FILENO) == -1)
                            return (exit_fatal());
                    }
                }

                // we close all 4 fds
                close(fd[0]);
                close(fd[1]);
                if (count)
                {
                    close(fd_prev[0]);
                    close(fd_prev[1]);
                }
                // Rq: fd is still "active", as we have duped one end to stdout (a pipe remains active as long as one of its fd is open)
                // --> the next subprocess will be able to receive data from this pipe
                // it will remain active until the next subprocess is done reading
                ret = exec_cmd(cmd, env);
            }

            if (pid == -1)
                return (exit_fatal());

            // we close only fd_prev (as we still need fd for the next pipe !)
            if (count)
            {
                close(fd_prev[0]);
                close(fd_prev[1]);
            }
            // if its the last pipe, we can close fd, as we no longer need it
            if (count == nb_pipes)
            {
                close(fd[0]);
                close(fd[1]);
            }
            start = i + 1;
            count++;
        }
    
        i++;
    }

    // 4 - wait for all the child processes, and store the return value of the last one
    i = 0;
    while (i < count)
    {
        if (waitpid(-1, &status, 0) == pid)
            ret = WEXITSTATUS(status);
        i++;
    }

    return ret;
}

int main(int ac, char **av, char **env)
{
    // cut av into cmds (separator = ;)
    char **cmd;
    int ret;
    int i = 1;
    int start = 1;

    if (ac == 1)
        return (EXIT_SUCCESS);

    while(i < ac + 1)
    {
        if (av[i] == NULL || strcmp(av[i], ";") == 0)
        {
            cmd = cut_av(av, start, i);
            ret = exec_pipe(cmd, env);
            if (ret == EXIT_FAILURE_SYSTEM) // attention, ne pas continuer si rencontre un exit_failure du a un appel systeme: - "Si un appel systeme, sauf execve et chdir, retourne une erreur votre programme devra immédiatement afficher dans STDERR "error: fatal" suivi d'un '\n' et sortir"
                return EXIT_FAILURE;
            start = i + 1;
        }
        i++;
    }
    // printf("cwd: %s\n", getcwd(NULL, 0));
    return ret;
}