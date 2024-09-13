#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define N 5                        // Number of philosophers
#define THINKING 'P'               // State for thinking
#define HUNGRY 'F'                 // State for hungry
#define EATING 'C'                 // State for eating
#define LEFT (N - 1)               // Value representing the left neighbor offset
#define RIGHT (N - LEFT)           // Value representing the right neighbor offset
#define STATES 3                   // Number of states (THINKING, HUNGRY, EATING)

char state[N];                     // Array to track the state of each philosopher
pthread_mutex_t mutex;             // Mutex to protect the critical section
pthread_mutex_t philosophers[N];   // Array of mutexes, one for each philosopher
pthread_t thread_id[N];            // Array of thread identifiers for each philosopher

// Function to print the states of all philosophers
void print_states() 
{
    for (int i(0); i < N; i++)
        printf("%c", state[i]);
    printf("\n");
}

// Function to test if philosopher i can start eating
void test(int i) 
{
    // Check if philosopher i is hungry and both neighbors are not eating
    if (state[i] == HUNGRY && state[(i + LEFT) % N] != EATING && state[(i + RIGHT) % N] != EATING) {
        state[i] = EATING;             // Set the state to eating
        print_states();                // Print the states
        pthread_mutex_unlock(&philosophers[i]); // Unlock the philosopher's mutex
    }
}

// Function for philosopher i to take forks (start eating)
void take_forks(int i) 
{
    pthread_mutex_lock(&mutex);        // Lock the global mutex
    state[i] = HUNGRY;                 // Set the state to hungry
    print_states();                    // Print the states
    test(i);                           // Test if the philosopher can eat
    pthread_mutex_unlock(&mutex);      // Unlock the global mutex
    pthread_mutex_lock(&philosophers[i]); // Lock the philosopher's mutex
}

// Function for philosopher i to put down forks (stop eating)
void put_forks(int i) 
{
    pthread_mutex_lock(&mutex);        // Lock the global mutex
    state[i] = THINKING;               // Set the state to thinking
    print_states();                    // Print the states
    test((i + LEFT) % N);              // Test if the left neighbor can eat
    test((i + RIGHT) % N);             // Test if the right neighbor can eat
    pthread_mutex_unlock(&mutex);      // Unlock the global mutex
}

// Function for the philosopher thread
void* philosopher(void* num) 
{
    int i = *(int*)num;                // Get the philosopher number
    printf("Creating philo %d with forks %d and %d\n", i, i, (i + RIGHT) % N);
    while (1) {                        // Infinite loop
        sleep(rand() % STATES + 1);    // Philosopher is thinking
        take_forks(i);                 // Philosopher takes forks (starts eating)
        sleep(rand() % STATES + 1);    // Philosopher is eating
        put_forks(i);                  // Philosopher puts down forks (stops eating)
    }
}

int main() 
{
    int philosopher_ids[N];            // Array to hold philosopher IDs

    pthread_mutex_init(&mutex, NULL);  // Initialize the global mutex
    for (int i(0); i < N; i++) {
        pthread_mutex_init(&philosophers[i], NULL); // Initialize each philosopher's mutex
        pthread_mutex_lock(&philosophers[i]);       // Lock each philosopher's mutex initially
        state[i] = THINKING;            // Set each philosopher's state to thinking
        philosopher_ids[i] = i;         // Assign IDs to philosophers
    }

    print_states();                     // Print initial states

    for (int i(0); i < N; i++)
        pthread_create(&thread_id[i], NULL, philosopher, &philosopher_ids[i]); // Create philosopher threads

    for (int i(0); i < N; i++)
        pthread_join(thread_id[i], NULL); // Wait for all philosopher threads to finish

    for (int i(0); i < N; i++)
        pthread_mutex_destroy(&philosophers[i]); // Destroy each philosopher's mutex
        
    pthread_mutex_destroy(&mutex);      // Destroy the global mutex

    return 0;                       
}
