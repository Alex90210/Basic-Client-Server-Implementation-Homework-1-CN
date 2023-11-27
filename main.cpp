#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CLIENT_WRITE_FIFO "MyTest_FIFO"
#define MAX_USERS 100
#define MAX_USERNAME_LENGTH 50

int main() {
    char s[300];
    int num, fd;

    mknod(CLIENT_WRITE_FIFO, S_IFIFO | 0666, 0);

    printf("Waiting for the client connection...\n");
    fd = open(CLIENT_WRITE_FIFO, O_RDONLY);
    printf("Client connected.\n");

    while (1) { // Infinite loop for continuous reading
        if ((num = read(fd, s, 300)) == -1) {
            perror("FIFO read error");
        } else if (num > 0) {
            s[num] = '\0';
            printf("%d bytes were read: %s\n", num, s);

            pid_t pid = fork();

            if (pid == -1) {
                perror("Fork error.");
                exit(1);
            } else if (pid == 0) {

                // Child process: Executable code here
                printf("Child process\n");





                exit(0); // Exit the child process
            } else {
                // Parent process: Continue reading from FIFO


                // begin login process
                FILE *userFile = fopen("usernames.txt", "r");
                if (userFile == NULL) {
                    perror("Error opening usernames.txt");
                    exit(1);
                }

                char usernames[MAX_USERS][MAX_USERNAME_LENGTH];
                int num_users = 0;

                while (fgets(usernames[num_users], MAX_USERNAME_LENGTH, userFile) != NULL) {
                    // Remove trailing newline character
                    usernames[num_users][strcspn(usernames[num_users], "\n")] = '\0';
                    num_users++;
                } // removable??


                printf("users: %d\n", num_users);
                for (int i = 0; i < num_users; i++) {
                    printf("User %d: %s\n", i + 1, usernames[i]);
                }



                char prefix[] = "login :";
                printf("string : %s", s);

                int valid_users = 0;
                int index = -1;
                if (strncmp(s, prefix, strlen(prefix)) == 0) {
                    // login is possible
                    printf("logging in...\n");
                    char* username = s + strlen(prefix);
                    printf("username: %s\n", username);
                    for (int i = 0; i < num_users; ++i) {
                        if (strcmp(username, usernames[i]) == 0) {
                            valid_users = 1;
                            index = i;
                            break;
                        }
                    }

                }
                if (valid_users == 1) {
                    printf("Logged in as: %s\n", usernames[index]);
                }
                else
                    printf("Invalid user.\n");

                // end login process


                wait(NULL); // Wait for child process to finish
            }
        }
    }

    return 0;
}
