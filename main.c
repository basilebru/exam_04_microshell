#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

void ft_putstr(char *av)
{
    int len = 0;
    while (av[len])
        len++;
    write(1, av, len);
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
    int ret;
    ret = execve(cmd[0], cmd, env);
    printf("error\n");
    exit(ret);
}

int exec_pipe(char **av, char **env)
{
    // cut av into cmds (separator = ;)
    char **cmd;
    int ret;
    int i = 1;
    int start = 0;
    int count = 0;
    int status;

    int ac = 0;
    while (av[ac])
        ac++;

    while(i < ac + 1)
    {
        if (av[i] == NULL || strcmp(av[i], "|") == 0)
        {
            count++;
            cmd = cut_av(av, start, i);
            // print_av(cmd);
            if (fork() == 0)
                ret = exec_cmd(cmd, env);
            start = i + 1;
        }
        i++;
    }
    i = 0;
    while (i < count)
    {
        waitpid(-1, &status, 0);
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

    while(i < ac + 1)
    {
        if (av[i] == NULL || strcmp(av[i], ";") == 0)
        {
            cmd = cut_av(av, start, i);
            ret = exec_pipe(cmd, env);
            start = i + 1;
        }
        i++;
    }
}