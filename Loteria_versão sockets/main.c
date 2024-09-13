#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define PORT 12345                         // Port number
#define BACKLOG 10                         // Maximum number of pending connections
#define NUM_CHILDREN 10                    // Number of child processes

volatile sig_atomic_t proceed = 0;

void error(const char *msg) {              // Error handling function
    perror(msg);                           // Print error message
    exit(1);                               // Exit with error code
}

void signal_handler(int signum) {
    proceed = 1;
}

int main() {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int pids_received[NUM_CHILDREN];
    int optval = 1;

    // Creating the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Error opening socket");     // Handle socket opening error

    // Setting socket options to reuse address
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        error("Error setting socket options"); // Handle setsockopt error

    // Configuring server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Binding the socket to an address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("Error binding socket");     // Handle binding error

    // Preparing to listen for connections
    if (listen(sockfd, BACKLOG) < 0)
        error("Error listening");          // Handle listening error

    printf("Parent process is listening for connections on port %d\n", PORT);

    // Creating child processes
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0)
            error("Error creating child process"); // Handle fork error
        if (pid == 0) {
            // Child process code
            int sockfd_child;
            struct sockaddr_in serv_addr_child;

            // Creating the child's socket
            sockfd_child = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_child < 0)
                error("Error opening child socket"); // Handle child socket error

            // Configuring the server address for the child
            serv_addr_child.sin_family = AF_INET;
            serv_addr_child.sin_addr.s_addr = inet_addr("127.0.0.1");
            serv_addr_child.sin_port = htons(PORT);

            // Connecting to the server
            if (connect(sockfd_child, (struct sockaddr *) &serv_addr_child, sizeof(serv_addr_child)) < 0)
                error("Error connecting child socket"); // Handle child connection error

            // Sending the child's PID
            pid_t child_pid = getpid();
            printf("Child process %d connected and sending PID\n", child_pid);
            if (write(sockfd_child, &child_pid, sizeof(child_pid)) < 0)
                error("Error sending PID"); // Handle PID sending error

            // Closing the child's socket after sending the PID
            close(sockfd_child);

            // Waiting for the signal from the parent
            signal(SIGUSR1, signal_handler);
            while (!proceed) pause(); // Wait for the signal

            // Connecting again to receive the drawn PID
            sockfd_child = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_child < 0)
                error("Error opening child socket"); // Handle child socket error

            // Configuring the server address for the child
            serv_addr_child.sin_family = AF_INET;
            serv_addr_child.sin_addr.s_addr = inet_addr("127.0.0.1");
            serv_addr_child.sin_port = htons(PORT);

            // Connecting to the server
            if (connect(sockfd_child, (struct sockaddr *) &serv_addr_child, sizeof(serv_addr_child)) < 0)
                error("Error connecting child socket"); // Handle child connection error

            // Receiving the drawn PID
            pid_t drawn_pid;
            if (read(sockfd_child, &drawn_pid, sizeof(drawn_pid)) < 0)
                error("Error receiving drawn PID"); // Handle drawn PID receiving error

            // If the drawn PID matches the child's PID, wait for the final signal
            if (drawn_pid == child_pid) {
                signal(SIGUSR2, signal_handler);
                while (!proceed) pause(); // Wait for the final signal
                printf("\n%d: FUI SORTEADO!\n", child_pid);
            }

            // Closing the child's socket
            close(sockfd_child);

            // Exit the child process
            exit(0);
        }
    }

    // Receiving connections from children
    for (int i = 0; i < NUM_CHILDREN; i++) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("Error accepting connection"); // Handle connection acceptance error

        // Receiving the child's PID
        pid_t received_pid;
        if (read(newsockfd, &received_pid, sizeof(received_pid)) < 0)
            error("Error reading PID"); // Handle PID reading error
        printf("Parent process received PID %d\n", received_pid);
        pids_received[i] = received_pid;

        // Closing the connection with the child
        close(newsockfd);
    }

    // Drawing a PID
    srand(time(NULL));
    int drawn_index = rand() % NUM_CHILDREN;
    pid_t drawn_pid = pids_received[drawn_index];
    printf("\nPID SORTEADO: %d\n\n", drawn_pid);

    // Sending a signal to all child processes to proceed
    for (int i = 0; i < NUM_CHILDREN; i++) {
        kill(pids_received[i], SIGUSR1);
    }

    // Informing the drawn PID to children
    for (int i = 0; i < NUM_CHILDREN; i++) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("Error accepting connection"); // Handle connection acceptance error

        // Sending the drawn PID to the child
        if (write(newsockfd, &drawn_pid, sizeof(drawn_pid)) < 0)
            error("Error sending drawn PID"); // Handle drawn PID sending error

        // Closing the connection with the child
        close(newsockfd);
    }

    // Sending the final signal to the drawn child process to proceed
    sleep(1); // Ensuring all previous communications are complete before sending the final signal
    kill(drawn_pid, SIGUSR2);

    // Closing the parent's socket
    close(sockfd);

    return 0;
}
