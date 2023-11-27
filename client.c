#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


#define CLIENT_WRITE_FIFO "CLIENT_SEND_FIFO"
#define SERVER_WRITE_FIFO "SERVER_WRITE_FIFO"

int main() {

    char s[100];
    int fd1, fd2;
    int num;

    printf("Waiting for the server...\n");
    fd1 = open(CLIENT_WRITE_FIFO, O_WRONLY);

    printf("Server online, start typing.\n");

    fd2 = open(SERVER_WRITE_FIFO, O_RDONLY);
    while (fgets(s, sizeof(s), stdin) != NULL) {
        if (write(fd1, s, strlen(s)) == -1)
            perror("FIFO write error.\n");

        char response[10000];
        if ((num = read(fd2, response, sizeof (response))) == -1)
            perror("Client read error\n");
        else {
            response[num] = '\0';
            if (strcmp(response, "Program terminated.") == 0) {
                printf("[Client] %d bytes: %s\n", num, response);
                return 0;
            }
            else
                printf("[Client] %d bytes: %s\n", num, response);
        }
    }
}

/*else if (strcmp(response, "Authentication successful.") == 0){
                char message[] = "logged";
                if (write(fd1, message, strlen(message)) == -1)
                    perror("FIFO write error.\n");
                printf("%s\n", response);
            }
            else if (strcmp(response, "Logging out...") == 0) {
                char message[] = "not logged";
                if (write(fd1, message, strlen(message)) == -1)
                    perror("FIFO write error.\n");
                printf("%s\n", response);
            }*/
// write the response back code