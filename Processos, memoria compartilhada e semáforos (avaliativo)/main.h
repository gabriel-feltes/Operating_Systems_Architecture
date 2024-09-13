#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define SEM_NAME "/my_semaphore"

void random_sleep() {
    int sleep_time = (rand() % 10) + 1;
    sleep(sleep_time);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_processes>\n", argv[0]);
        return 1;
    }

    int nprocs = atoi(argv[1]);
    if (nprocs <= 0) {
        fprintf(stderr, "The number of processes must be greater than 0.\n");
        return 1;
    }

    // Create shared memory for the id
    int* id = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (id == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    *id = 0;

    // Initialize semaphore
    sem_t* sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        munmap(id, sizeof(int));
        return 1;
    }

    srand(time(NULL));

    for (int i = 1; i <= nprocs; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            munmap(id, sizeof(int));
            sem_unlink(SEM_NAME);
            sem_close(sem);
            return 1;
        } else if (pid == 0) { // Child process
            random_sleep();
            sem_wait(sem);
            int my_id = ++(*id);
            sem_post(sem);
            printf("Processo %d criado\n", my_id);
            munmap(id, sizeof(int));
            sem_close(sem);
            exit(0);
        }
    }

    // Wait for all child processes to finish
    for (int i = 1; i <= nprocs; i++) {
        wait(NULL);
    }

    // Clean up
    munmap(id, sizeof(int));
    sem_unlink(SEM_NAME);
    sem_close(sem);

    return 0;
}
