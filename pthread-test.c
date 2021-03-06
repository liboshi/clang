#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NUM_THREADS 5
#define N 1000
#define MEGEXTRA 1000
#define VECLEN 100

typedef struct _thread_data {
        int thread_id;
        int sum;
        char *message;
} thread_data;

typedef struct {
        double *a;
        double *b;
        double sum;
        int veclen;
} dotdata;

thread_data thread_data_array[NUM_THREADS];
pthread_attr_t attr;
pthread_mutex_t mutexsum;
dotdata dotstr;

void*
printHello(void *threadid) {
        long tid;
        tid = (long)threadid;
        printf("Hello world, I am thread - #%ld\n", tid);
        pthread_exit(NULL);
}

void*
sayHello(void *threadargs) {
        thread_data *my_data;
        my_data = (thread_data *)threadargs;
        int taskid = my_data->thread_id;
        int sum = my_data->thread_id;
        char *hello_msg = my_data->message;
        printf("Message: %s, I am thread %d, total number is %d\n",
               hello_msg, taskid, sum);
        pthread_exit(NULL);
}

void*
busyWork(void *t) {
        int i;
        long tid;
        double result = 0.0;
        tid = (long)t;

        printf("Thread %ld starting...\n", tid);
        for (i = 0; i < 10000000; i++) {
                result = result + sin(i) * tan(i);
        }
        printf("Thread %ld done. Result = %e \n", tid, result);
        pthread_exit((void *)t);
}

void*
dowork(void *threadid) {
        double A[N][N];
        int i, j;
        long tid;
        size_t mystacksize;

        tid = (long)threadid;
        pthread_attr_getstacksize(&attr, &mystacksize);
        printf("Thread %ld: stack size = %li bytes \n", tid, mystacksize);
        for (i = 0; i < N; i++) {
                for (j = 0; j < N; j++) {
                        A[i][j] = ((i * j) / 3.452) + (N - i);
                }
        }
        pthread_exit((void *)threadid);
}

void*
dotprod(void *arg) {
        int i, start, end, len;
        long offset;
        double mysum, *x, *y;
        offset = (long)arg;

        len = dotstr.veclen;
        start = offset * len;
        end = start + len;
        x = dotstr.a;
        y = dotstr.b;

        mysum = 0;
        for (i = start; i < end; i++) {
                mysum += (x[i] * y[i]);
        }

        pthread_mutex_lock(&mutexsum);
        dotstr.sum += mysum;
        pthread_mutex_unlock(&mutexsum);

        pthread_exit((void*) 0);
}

void
main_a() {
        pthread_t threads[NUM_THREADS];
        char *messages[NUM_THREADS] = {
                                       "Hello 1",
                                       "Hello 2",
                                       "Hello 3",
                                       "Hello 4",
                                       "Hello 5"};
        int rc;
        int sum = 0;
        long t;
        long *taskids[NUM_THREADS];
        for(t = 0; t < NUM_THREADS; t++) {
                taskids[t] = (long *)malloc(sizeof(long));
                *taskids[t] = t;
                sum += t;
                thread_data_array[t].thread_id = t;
                thread_data_array[t].sum = sum;
                thread_data_array[t].message = messages[t];
                printf("In main: creating thread %ld\n", t);
                /* How to pass argument to a thread */
                /* rc = pthread_create(&threads[t], NULL, printHello, (void *)taskids[t]); */
                /* How to pass arguments to a thread via sturcture */
                rc = pthread_create(&threads[t], NULL, sayHello, (void *)&thread_data_array[t]);
                if(rc) {
                        printf("ERROR: return code from pthread_create() is %d\n", rc);
                        exit(-1);
                }
        }
}

void
main_b() {
        pthread_t thread[NUM_THREADS];
        pthread_attr_t *attr;
        int rc;
        long t;
        void *status;

        /* Initialize and set thread detached attribute */
        pthread_attr_init(attr);
        pthread_attr_setdetachstate(attr, PTHREAD_CREATE_JOINABLE);

        for (t = 0; t < NUM_THREADS; t++) {
                printf("Main: creating thread %ld \n", t);
                rc = pthread_create(&thread[t], attr, busyWork, (void *)t);
                if (rc) {
                        printf("ERROR: return code from pthread_create() is %d \n", rc);
                        exit(-1);
                }
        }

        /* Free attribute and wait for the other threads */
        pthread_attr_destroy(attr);
        for (t = 0; t < NUM_THREADS; t++) {
                rc = pthread_join(thread[t], &status);
                if (rc) {
                        printf("ERROR: return code from pthread_join() is %d \n", rc);
                        exit(-1);
                }
                printf("Main: completed join with thread %ld having a status of %ld\n", t, (long)status);
        }
        printf("Main: program completed. Exiting.\n");
}

int
main_c() {
        pthread_t threads[NUM_THREADS];
        size_t stacksize;
        int rc;
        long t;

        pthread_attr_init(&attr);
        pthread_attr_getstacksize(&attr, &stacksize);
        printf("Default stack size = %li\n", stacksize);
        stacksize = sizeof(double) * N * N + MEGEXTRA;
        printf("Amount of stack needed per thread = %li\n", stacksize);
        pthread_attr_setstacksize(&attr, stacksize);
        printf("Creating threads with stack size = %li bytes\n", stacksize);
        for (t = 0; t < NUM_THREADS; t++) {
                rc = pthread_create(&threads[t], &attr, dowork, (void *)t);
                if (rc) {
                        printf("ERROR: return code from pthread_create() is %d\n", rc);
                        exit(-1);
                }
        }
        printf("Created %ld threads.\n", t);
        return 0;
}

int
main_d() {
        pthread_t threads[NUM_THREADS];
        long i;
        double *a, *b;
        void *status;
        pthread_attr_t attr;

        /* Assign storage and initialize values */
        a = (double*) malloc (NUM_THREADS * VECLEN * sizeof(double));
        b = (double*) malloc (NUM_THREADS * VECLEN * sizeof(double));

        for (i = 0; i < VECLEN*NUM_THREADS; i++) {
                a[i] = 1.0;
                b[i] = a[i];
        }

        dotstr.veclen = VECLEN;
        dotstr.a = a;
        dotstr.b = b;
        dotstr.sum = 0;

        pthread_mutex_init(&mutexsum, NULL);

        /* Create threads to perform the dotproduct  */
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        for(i = 0; i<NUM_THREADS; i++)
        {
                pthread_create(&threads[i], &attr, dotprod, (void *)i);
        }

        pthread_attr_destroy(&attr);

        /* Wait on the other threads */
        for(i = 0; i < NUM_THREADS; i++)
        {
                pthread_join(threads[i], &status);
        }

        /* After joining, print out the results and cleanup */
        printf ("Sum =  %f \n", dotstr.sum);
        free (a);
        free (b);
        pthread_mutex_destroy(&mutexsum);
        return 0;
}

int
main(int argc, char *argv[]) {
        main_d();
}

