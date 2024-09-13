#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

long x;

// Check for prime factors in a given range
void* is_prime(void* arg) 
{
    long start = ((long*)arg)[0];
    long end = ((long*)arg)[1];
    for (int i(start); i <= end; i++) {
        if (x % i == 0)
            pthread_exit((void*)0); // Exit the current thread, indicating non-prime
    }
    pthread_exit((void*)1);         // Exit the current thread, indicating no divisors found in this range
}

int main()
{
    scanf("%ld", &x);   // Read the number from the user

    if (x <= 1) {
        printf("0");    // Numbers <= 1 are not prime
        return 0;
    }

    pthread_t thread1, thread2;
    long args1[2] = {2, x/2};   // Arguments for the first thread
    long args2[2] = {x/3, x-1}; // Arguments for the second thread
    void* thread1_result;
    void* thread2_result;

    // Create the threads
    pthread_create(&thread1, NULL, is_prime, (void*)args1);
    pthread_create(&thread2, NULL, is_prime, (void*)args2);

    // Wait for the threads to finish
    pthread_join(thread1, &thread1_result);
    pthread_join(thread2, &thread2_result);

    // Determine if the number is prime based on thread results
    if ((long)thread1_result == 1 && (long)thread2_result == 1)
        printf("1"); // The number is prime
    else
        printf("0"); // The number is not prime

    return 0;
}
