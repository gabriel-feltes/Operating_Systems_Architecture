#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS 128
#define INCREMENTS_PER_THREAD 1000

// Global counter
int counter(0);

// Mutex to protect the counter from race conditions
pthread_mutex_t counter_mutex;

// Thread function to simulate client requests
void* increment_counter(void* arg) 
{
    int thread_id = *(int*)arg;
    
    // Each thread will increment the global counter "INCREMENTS_PER_THREAD" times
    for (int i(0); i < INCREMENTS_PER_THREAD; i++) {
        pthread_mutex_lock(&counter_mutex);                     // Lock the mutex before accessing the counter to prevent race conditions
        counter++;
        pthread_mutex_unlock(&counter_mutex);                   // Unlock the mutex after updating the counter
    }
    
    printf("Thread %d completed.\n", thread_id);
    return NULL;
}

int main() 
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    clock_t start, end;
    double cpu_time_used(0);

    if (pthread_mutex_init(&counter_mutex, NULL) != 0) {       // Initialize the mutex
        fprintf(stderr, "Error initializing mutex\n");
        return 1;
    }

    printf("Starting server simulation...\n");
    printf("Creating %d threads, each one simulating %d requests...\n", NUM_THREADS, INCREMENTS_PER_THREAD);

    start = clock();                                           // Record the start time

    // Create threads
    for (int i(0); i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, increment_counter, &thread_ids[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return 1;
        }
        printf("Thread %d created.\n", i + 1);                 // Print a message indicating the thread has been created
    }

    // Wait for all threads to complete
    for (int i(0); i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        printf("Thread %d finished.\n", i + 1);                // Print a message indicating the thread has finished
    }

    end = clock();                                             // Record the end time
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC; // Calculate the total CPU time used

    pthread_mutex_destroy(&counter_mutex);                     // Destroy the mutex as it is no longer needed

    printf("Simulation complete.\n");
    printf("Final counter value: %d\n", counter);              // Print the final value of the counter
    printf("Processing time: %.2f seconds\n", cpu_time_used);  // Print the total processing time

    return 0;
}
