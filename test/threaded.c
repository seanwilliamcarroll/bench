#include <math.h>
#include <pthread.h>
#include <stdlib.h>

/* Thread 1: integer accumulation with call depth */
static long sum_range(long start, long end) {
    volatile long acc = 0;
    for (long i = start; i < end; i++) {
        acc += i;
    }
    return acc;
}

static void integer_work() {
    volatile long total = 0;
    for (int i = 0; i < 10; i++) {
        total += sum_range(i * 10000000L, (i + 1) * 10000000L);
    }
}

static void *thread_integer(void *arg) {
    (void)arg;
    while (1) {
        integer_work();
    }
    return NULL;
}

/* Thread 2: floating point via sqrt */
static double sqrt_sum(long n) {
    volatile double acc = 0.0;
    for (long i = 1; i <= n; i++) {
        acc += sqrt((double)i);
    }
    return acc;
}

static void *thread_float(void *arg) {
    (void)arg;
    while (1) {
        sqrt_sum(40000000L);
    }
    return NULL;
}

/* Thread 3: prime counting (division-heavy) */
static int is_prime(long n) {
    if (n < 2) {
        return 0;
    }
    for (long i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

static long count_primes(long limit) {
    volatile long count = 0;
    for (long i = 2; i < limit; i++) {
        if (is_prime(i)) {
            count++;
        }
    }
    return count;
}

static void *thread_primes(void *arg) {
    (void)arg;
    while (1) {
        count_primes(4000000L);
    }
    return NULL;
}

/* Main thread: insertion sort on shuffled array */
static void shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

static void insertion_sort(int *arr, int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

int main() {
    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, thread_integer, NULL);
    pthread_create(&t2, NULL, thread_float, NULL);
    pthread_create(&t3, NULL, thread_primes, NULL);

    int n = 8000;
    int *arr = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        arr[i] = i;
    }
    while (1) {
        shuffle(arr, n);
        insertion_sort(arr, n);
    }

    free(arr);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    return 0;
}
