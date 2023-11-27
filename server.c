#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <utmp.h>
#include <time.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define CLIENT_WRITE_FIFO "CLIENT_SEND_FIFO"
#define SERVER_WRITE_FIFO "SERVER_WRITE_FIFO"
/*#define LOGIN_CHILD_FIFO "LOGIN_CHILD_FIFO"
#define LOGIN_PARENT_FIFO "LOGIN_PARENT_FIFO"*/

char last_message[100];

int main() {
    int num, fd1;

    mknod(CLIENT_WRITE_FIFO, S_IFIFO | 0666, 0);
    mknod(SERVER_WRITE_FIFO, S_IFIFO | 0666, 0);
    /*mknod(LOGIN_CHILD_FIFO, S_IFIFO | 0666, 0);
    mknod(LOGIN_PARENT_FIFO, S_IFIFO | 0666, 0);*/

    printf("Waiting for the client connection...\n");
    fd1 = open(CLIENT_WRITE_FIFO, O_RDONLY);
    printf("Client connected.\n");

    int fd2;
    fd2 = open(SERVER_WRITE_FIFO, O_WRONLY);

    int shmid;
    key_t key = 1234;

    shmid = shmget(key, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    int *logged = shmat(shmid, NULL, 0);
    if (logged == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    *logged = 0;
    struct utmp* entry;
    int iterations = 0;
    do {
        printf("this is the %d iteration of the while loop\n", iterations);
        fflush(stdout);
        ++iterations;

        char s[1000];

        /*int fd2;
        fd2 = open(SERVER_WRITE_FIFO, O_WRONLY);*/
        if ((num = read(fd1, s, 1000)) == -1) {
            perror("FIFO read error");
        } else if (num > 0) {
            s[num] = '\0';
            printf("initial string: %s\n", s);

            char login_child_prefix[] = "login : ";
            if (strncmp(s, login_child_prefix, strlen(login_child_prefix)) == 0) {

                int pipe_fd[2];
                if (pipe(pipe_fd) == - 1) {
                    perror("Pipe error.\n");
                    exit(1);
                }

                pid_t pid;
                if ((pid = fork()) == -1) {
                    perror("Login fork error.\n");
                    exit(1);
                }

                if (pid == 0) {
                    if (*logged == 1) {
                        char response[] = "logged";
                        if (write(pipe_fd[1], response, sizeof (response)) == -1) {
                            perror("Login child pipe write error.\n");
                            exit(1);
                        }
                    }
                    else {

                        close(pipe_fd[0]);
                        FILE *userFile = fopen("usernames.txt", "r");
                        if (userFile == NULL) {
                            perror("Error opening usernames.txt");
                            exit(1);
                        }

                        char usernames[100][50];
                        int num_users = 0;

                        while (fgets(usernames[num_users], 50, userFile) != NULL) {
                            num_users++;
                        }

                        int valid_users = 0;

                        char *username = s + strlen(login_child_prefix);
                        for (int i = 0; i < num_users; ++i) {
                            if (strcmp(username, usernames[i]) == 0) {
                                valid_users = 1;
                                break;
                            }
                        }

                        if (valid_users == 1) {
                            char response[] = "biborteni";
                            *logged = 1;

                            if (write(pipe_fd[1], response, sizeof(response)) == -1) {
                                perror("Child pipe write error.\n");
                                exit(1);
                            }
                        }

                        close(pipe_fd[1]);
                    }
                }
                else {
                    // parent process
                    // sleep(1);
                    close(pipe_fd[1]);
                    char child_response[100];

                    if (read(pipe_fd[0], child_response, sizeof (child_response)) == -1) {
                        perror("Parent pipe read error.\n");
                        exit(1);
                    }

                    if (strcmp(child_response, "logged") == 0) {
                        char status1[] = "You are already logged.";
                        if (write(fd2, status1, sizeof(status1)) == -1)
                            perror("Server write error.\n");
                    }

                    else if (strcmp(child_response, "biborteni") == 0) {
                        *logged = 1;
                        char status2[] = "Authentication successful.";
                        if (write(fd2, status2, strlen(status2)) == -1)
                            perror("Server write error.\n");
                    }
                    else {
                        char status3[] = "Authentication denied.";
                        if (write(fd2, status3, sizeof(status3)) == -1)
                            perror("Server write error.\n");
                    }

                    close(pipe_fd[0]);
                }
                // continue;
            }
            else if (strncmp(s, "get-logged-users", 16) == 0) {
                // get logged process, using socket pair

                // socket pair for the others
                int sockp[2];
                if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) < 0) {
                    perror("Socketpair error.\n");
                    exit(1);
                }

                pid_t pid;
                if ((pid = fork()) == -1) {
                    perror("Get logged fork error.\n");
                    exit(1);
                }

                if (pid == 0) {
                    close(sockp[0]);

                    if (*logged == 0) {
                        char text[] = "not logged";
                        if(write(sockp[1], text, sizeof (text))  == -1) {
                            perror("Get logged child write error.\n");
                            exit(1);
                        }
                    }
                    else {
                        setutent();

                        char text[10000];
                        while ((entry = getutent()) != NULL) {
                            if(entry->ut_type == USER_PROCESS) {
                                char temp[500];
                                strcat(text, "\n");

                                time_t timestamp = entry->ut_tv.tv_sec;
                                struct tm *timeinfo = localtime(&timestamp);
                                char buffer[80];
                                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

                                snprintf(temp, sizeof(temp),
                                         "Username: %s\n"
                                         "Hostname: %s\n"
                                         "Time entry: %s\n",
                                         entry->ut_user, entry->ut_host, buffer);
                                strcat(text, temp);
                            }
                        }

                        endutent();

                        // sent text
                        if (write(sockp[1], text, sizeof(text)) == -1) {
                            perror("Get logged child write error.\n");
                            exit(1);
                        }

                        close(sockp[1]);
                    }
                }
                else {
                    // parent process
                    sleep(1);
                    close(sockp[1]);

                    char received_text[10000];
                    if (read(sockp[0], received_text, sizeof(received_text)) == -1) {
                        perror("Get logged parent read error.\n");
                        exit(1);
                    }

                    // received data until here, through the socket pair

                    // send data through the fifo, only the text

                    if (strcmp(received_text, "not logged") == 0) {
                        char message[] = "You cannot use this command without being logged in.";
                        if (write(fd2, message, sizeof(message)) == -1)
                            perror("Server write error.\n");
                    }
                    else
                    if (write(fd2, received_text, sizeof(received_text)) == -1)
                        perror("Server write error.\n");
                    close(sockp[0]);
                }
                //close(fd2);
                // continue;
            }
            else if (strncmp(s, "get-proc-info : ", 16) == 0) {

                pid_t pid;

                // socket pair for the others
                int sockp[2];
                if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) < 0) {
                    perror("Socketpair error.\n");
                    exit(1);
                }

                pid = fork();
                if (pid == 0) {
                    if (*logged == 0) {
                        char text[] = "not logged";
                        if(write(sockp[1], text, sizeof (text))  == -1) {
                            perror("Get proc child write error.\n");
                            exit(1);
                        }
                    }
                    else {
                        close(sockp[0]);

                        char* c_pid_to_search = s + strlen("get-proc-info : ");
                        int pid_to_search = atoi(c_pid_to_search);
                        char filepath[200];
                        snprintf(filepath, 200, "/proc/%d/status", pid_to_search);
                        printf("Filepath: %s\n", filepath);

                        FILE *file = fopen(filepath, "r");

                        if (file) {
                            char line[300];
                            dprintf(sockp[1], "Filepath: %s\n", filepath);
                            while (fgets(line, sizeof(line), file)) {
                                int value;
                                char field[2000];

                                if (sscanf(line, "Name: %s", field) == 1) {

                                    dprintf(sockp[1], "Name: %s\n", field);

                                } else if (sscanf(line, "State: %s", field) == 1) {

                                    dprintf(sockp[1], "State: %s\n", field);

                                } else if (sscanf(line, "PPid: %d", &value) == 1) {
                                    char str[30];
                                    sprintf(str, "%d", value);

                                    dprintf(sockp[1], "PPid: %s\n", str);

                                } else if (sscanf(line, "Uid: %d", &value) == 1) {
                                    char str[30];
                                    sprintf(str, "%d", value);

                                    dprintf(sockp[1], "Uid: %s\n", str);

                                } else if (sscanf(line, "VmSize: %d", &value) == 1) {
                                    char str[30];
                                    sprintf(str, "%d", value);
                                    strcat(str, " KB");
                                    dprintf(sockp[1], "VmSize: %s\n", str);

                                }
                            }
                            fclose(file);
                        }
                        else {
                            char message[] = "fileerr";
                            if (write(sockp[1], message, sizeof(message)) == -1) {
                                perror("Get proc child write error.\n");
                                exit(1);
                            }
                        }

                        close(sockp[1]);
                    }
                }
                else {
                    sleep(1);
                    close(sockp[1]);

                    char received_text[10000];
                    if (read(sockp[0], received_text, sizeof (received_text)) == - 1) {
                        perror("Get proc parent read error.\n");
                        exit(1);
                    }


                    if(strcmp(received_text, "not logged") == 0) {
                        char message[] = "You cannot use this command without being logged in.";
                        if (write(fd2, message, sizeof(message)) == -1)
                            perror("Server write error.\n");
                    }


                    if (strcmp(received_text, "fileerr") == 0) {
                        char message[] = "Invalid pid.";
                        if (write(fd2, message, sizeof(message)) == -1)
                            perror("Server write error.\n");
                    }


                    else if (write(fd2, received_text, sizeof(received_text)) == -1)
                        perror("Server write error.\n");

                    close(sockp[0]);
                }
                //close(fd2);
                // continue;
            }
            else if (strncmp(s, "logout", 6) == 0) {

                pid_t pid;
                pid = fork();

                // socket pair for the others
                int sockp1[2];
                if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp1) < 0) {
                    perror("Socketpair error.\n");
                    exit(1);
                }

                if (pid == 0) {
                    close(sockp1[0]);

                    printf("logged status: %d\n", *logged);
                    printf("logged status before logging out: %d\n", *logged);

                    if (*logged == 1) {
                        *logged = 0;
                        char response[] = "logged";
                        if (write(sockp1[1], response, sizeof (response)) == -1) {
                            perror("Child pipe write error.\n");
                            exit(1);
                        }
                    }
                    else if (*logged == 0){
                        char response[] = "not logged";
                        if (write(sockp1[1], response, sizeof(response)) == -1) {
                            perror("Child pipe write error.\n");
                            exit(1);
                        }
                    }
                    close(sockp1[1]);
                }
                else {
                    sleep(1);
                    close(sockp1[1]);

                    char child_response[1000];
                    if (read(sockp1[0], child_response, sizeof (child_response)) == -1) {
                        perror("Parent pipe read error.\n");
                        exit(1);
                    }

                    printf("CHILD RESPONSE: %s\n", child_response);

                    if(strcmp(child_response, "logged") == 0) {
                        *logged = 0;
                        char status[] = "Logging out...";
                        if (write(fd2, status, strlen(status) + 1))
                            perror("Server write error.\n");
                    }
                    else if(strcmp(child_response, "not logged") == 0) {
                        char status[] = "You are not logged.";
                        if (write(fd2, status, sizeof(status)) == -1)
                            perror("Server write error.\n");
                    }

                    close(sockp1[0]);
                }
                // close(fd2);
                // continue;
            }
            else if (strncmp(s, "quit", 4) == 0) {
                char status[] = "Program terminated.";
                if (write(fd2, status, sizeof(status)) == -1)
                    perror("Server write error.\n");

                close(fd1);
                close(fd2);
                shmdt(logged);
                shmctl(shmid, IPC_RMID, NULL);

                return 0;
            }
            else {
                char buffer[] = "Unknown command.";
                if (write(fd2, buffer, sizeof (buffer)) == -1)
                    perror("Server write error.\n");
                // close(fd2);
                // continue;
            }
        }
    } while (num > 0);
}