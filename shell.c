#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    int i;

    while (1) {
        printf("CS340Shell%% ");
        fflush(stdout);

        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0)
            continue;

    
        int argc = 0;
        char *token = strtok(input, " ");
        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;

        if (argc == 0)
            continue;

        if (strcmp(args[0], "exit") == 0) {
            exit(0);
        }

    
        if (strcmp(args[0], "cd") == 0) {
            if (argc < 2) {
                printf("cd: need an argument\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
            continue;
        }

        if (strcmp(args[0], "time") == 0) {
            time_t t = time(NULL);
            printf("%s", ctime(&t));
            continue;
        }

        
        int num_pipes = 0;
        for (i = 0; i < argc; i++) {
            if (strcmp(args[i], "|") == 0)
                num_pipes++;
        }

        if (num_pipes > 0) {
            char **cmds[MAX_ARGS];
            int cmd_start[MAX_ARGS];
            int num_cmds = 0;

            cmd_start[0] = 0;
            for (i = 0; i < argc; i++) {
                if (strcmp(args[i], "|") == 0) {
                    args[i] = NULL; 
                    num_cmds++;
                    cmd_start[num_cmds] = i + 1;
                }
            }
            num_cmds++; 

            for (i = 0; i < num_cmds; i++) {
                cmds[i] = &args[cmd_start[i]];
            }

           
            int pipes[MAX_ARGS][2];
            for (i = 0; i < num_pipes; i++) {
                if (pipe(pipes[i]) < 0) {
                    perror("pipe");
                    break;
                }
            }

            pid_t pids[MAX_ARGS];

            for (i = 0; i < num_cmds; i++) {
                char *infile = NULL;
                char *outfile = NULL;
                int j;
                for (j = 0; cmds[i][j] != NULL; j++) {
                    if (strcmp(cmds[i][j], ">") == 0) {
                        outfile = cmds[i][j+1];
                        cmds[i][j] = NULL;
                        break;
                    }
                    if (strcmp(cmds[i][j], "<") == 0) {
                        infile = cmds[i][j+1];
                        cmds[i][j] = NULL;
                        break;
                    }
                }

                pids[i] = fork();
                if (pids[i] == 0) {
                    if (infile != NULL) {
                        int fd = open(infile, O_RDONLY);
                        dup2(fd, 0);
                        close(fd);
                    } else if (i > 0) {
                        dup2(pipes[i-1][0], 0);
                    }

                
                    if (outfile != NULL) {
                        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        dup2(fd, 1);
                        close(fd);
                    } else if (i < num_cmds - 1) {
                        dup2(pipes[i][1], 1);
                    }

                    
                    int k;
                    for (k = 0; k < num_pipes; k++) {
                        close(pipes[k][0]);
                        close(pipes[k][1]);
                    }

                    execv(cmds[i][0], cmds[i]);
                    perror("execv");
                    exit(1);
                }
            }

            for (i = 0; i < num_pipes; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

           
            for (i = 0; i < num_cmds; i++) {
                waitpid(pids[i], NULL, 0);
            }

        } else {
         
            char *infile = NULL;
            char *outfile = NULL;
            char *filtered_args[MAX_ARGS];
            int new_argc = 0;

            for (i = 0; i < argc; i++) {
                if (strcmp(args[i], "<") == 0) {
                    infile = args[i+1];
                    i++; // skip filename
                } else if (strcmp(args[i], ">") == 0) {
                    outfile = args[i+1];
                    i++;
                } else {
                    filtered_args[new_argc++] = args[i];
                }
            }
            filtered_args[new_argc] = NULL;

            pid_t pid = fork();
            if (pid == 0) {
              
                if (infile != NULL) {
                    int fd = open(infile, O_RDONLY);
                    if (fd < 0) {
                        perror("open");
                        exit(1);
                    }
                    dup2(fd, 0);
                    close(fd);
                }
                if (outfile != NULL) {
                    int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) {
                        perror("open");
                        exit(1);
                    }
                    dup2(fd, 1);
                    close(fd);
                }

                execv(filtered_args[0], filtered_args);
                perror("execv");
                exit(1);
            } else {
                waitpid(pid, NULL, 0);
            }
        }
    }

    return 0;
}