///////////////////////////////////////////////////////////////////////////////
// inspired by github "TheRealYoussef"
//    https://github.com/TheRealYoussef/The-Dining-Philosophers-Problem/
///////////////////////////////////////////////////////////////////////////////

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include <wait.h>

///////////////////////////////////////////////////////////////////////////////

#define NUMBER_OF_PHILOSOPHERS 5
#define NUMBER_OF_ROUNDS 5

///////////////////////////////////////////////////////////////////////////////

void no_zombie(int signalNumber)
{
   pid_t pid;
   int ret;

   // https://linux.die.net/man/3/waitpid
   // pid_t waitpid(pid_t pid, int *stat_loc, int options);
   while ((pid = waitpid(-1, &ret, WNOHANG)) > 0)
      printf("Child with pid=%d stopped (signal number: %d)\n",
             pid,
             signalNumber);
   return;
}

///////////////////////////////////////////////////////////////////////////////

void *philosopher_main(void *);
void think(int);
void pickUp(int);
void eat(int);
void putDown(int);

///////////////////////////////////////////////////////////////////////////////

pthread_mutex_t chopsticks[NUMBER_OF_PHILOSOPHERS];
pthread_t philosophers[NUMBER_OF_PHILOSOPHERS];
int philosopherNumbers[NUMBER_OF_PHILOSOPHERS];
pthread_attr_t attributes[NUMBER_OF_PHILOSOPHERS];

///////////////////////////////////////////////////////////////////////////////

int main()
{
   int i;

   ////////////////////////////////////////////////////////////////////////////
   // https://linux.die.net/man/3/srand
   // sets seed
   srand(time(NULL));

   // https://linux.die.net/man/2/signal
   // reaction to termination required, else zombie process
   // see also https://linux.die.net/man/3/sigaction
   signal(SIGCHLD, no_zombie);

   // https://linux.die.net/man/3/fork
   int pid = fork();
   if (pid == 0)
   {
      // Upon successful completion, fork() shall return 0 to the child process
      // and shall return the process ID of the child process to the parent
      // process.

      time_t current_timestamp;
      struct tm *timeinfo;
      char current_timestamp_string[100];
      // I am a child process and print stat-lines here;
      for (i = 0; i < 5; i++)
      {
         time(&current_timestamp);
         timeinfo = localtime(&current_timestamp);
         strftime(current_timestamp_string, 100, "%H:%M:%S", timeinfo);

         printf("-------------- %s\n", current_timestamp_string);
         usleep(1000); // micro-seconds
      };
      return EXIT_SUCCESS;
   }
   else
   {
      printf("process created for 100micro-sec separation (pid: %d)\n", pid);
   }

   // https://linux.die.net/man/3/uuid_generate_random
   //
   // The UUID is 16 bytes (128 bits) long, which gives approximately 3.4x10^38
   // unique values (there are approximately 10^80 elementary particles in the
   // universe according to Carl Sagan's Cosmos). The new UUID can reasonably
   // be considered unique among all UUIDs created on the local system, and
   // among UUIDs created on other systems in the past and in the future.
   uuid_t binuuid;
   uuid_generate_random(binuuid);

   char testSessionId[37]; // 36 + \0
   // https://linux.die.net/man/3/uuid_unparse
   uuid_unparse_lower(binuuid, testSessionId);
   printf("Start Test-SessionId: %s\n", testSessionId);

   // init
   for (i = 0; i < NUMBER_OF_PHILOSOPHERS; i++)
   {
      philosopherNumbers[i] = i;

      // https://linux.die.net/man/3/pthread_mutex_init
      // int pthread_mutex_init(pthread_mutex_t *restrict mutex,
      //                        const pthread_mutexattr_t *restrict attr);
      // NULL = default attributes
      pthread_mutex_init(&chopsticks[i], NULL);

      // https://linux.die.net/man/3/pthread_attr_init
      // initializes the thread attributes object pointed to by
      // attr with default attribute values.
      pthread_attr_init(&attributes[i]);
   }

   for (i = 0; i < NUMBER_OF_PHILOSOPHERS; i++)
   {
      // https://linux.die.net/man/3/pthread_create
      // returns thread id (same as pthread_self)
      pthread_create(&philosophers[i],
                     &attributes[i],
                     philosopher_main,
                     (void *)(&philosopherNumbers[i]));
   }

   for (i = 0; i < NUMBER_OF_PHILOSOPHERS; i++)
   {
      // https://linux.die.net/man/3/pthread_join
      // The pthread_join() function waits for the thread specified
      // by thread to terminate
      //
      // Either pthread_join(3) or pthread_detach() should be called for each
      // thread that an application creates, so that system resources for the
      //thread can be released. (But note that the resources of all threads
      // are freed when the process terminates.)
      //
      // https://man7.org/linux/man-pages/man3/pthread_join.3.html
      // Failure to join with a thread that is joinable (i.e., one that is
      // not detached), produces a "zombie thread".
      // int pthread_join(pthread_t thread, void **retval);
      if (pthread_join(philosophers[i], NULL) != 0)
      {
         printf("ERR: join not successful");
      }
      else
      {
         printf("philosopher %d done in main\n", i);
      }
   }

   for (i = 0; i < NUMBER_OF_PHILOSOPHERS; i++)
   {
      // https://linux.die.net/man/3/pthread_mutex_destroy
      // int pthread_mutex_destroy(pthread_mutex_t *mutex);
      pthread_mutex_destroy(&chopsticks[i]);
   }

   printf("Stop  Test-SessionId: %s\n", testSessionId);

   return 0;
}

void *philosopher_main(void *arg)
{
   int philosopherNumber = *((int *)arg);
   for (int round = 0; round < NUMBER_OF_ROUNDS; round++)
   {
      printf("Philosopher %d starts round                                  %d\n", philosopherNumber, round);
      think(philosopherNumber);
      pickUp(philosopherNumber);
      eat(philosopherNumber);
      putDown(philosopherNumber);
   }

   sleep(3); // seconds

   // https://linux.die.net/man/3/pthread_self
   printf("Philosopher %d done on thread: %lu \n", 
      philosopherNumber, 
      pthread_self());

   // https://linux.die.net/man/3/pthread_exit
   // The pthread_exit() function terminates the calling thread and returns a
   // value via retval that (if the thread is joinable) is available to 
   // another thread in the same process that calls pthread_join(3).
   pthread_exit(NULL);

   return NULL;
}

void think(int philosopherNumber)
{
   int sleepTime = (rand() % 3 + 1) * 100;
   printf("Philosopher %d will think for %d micro-seconds\n", 
      philosopherNumber, 
      sleepTime);
   usleep(sleepTime);
}

void pickUp(int philosopherNumber)
{
   int right = (philosopherNumber + 1) % NUMBER_OF_PHILOSOPHERS;
   int left = (philosopherNumber + NUMBER_OF_PHILOSOPHERS) % NUMBER_OF_PHILOSOPHERS;

   int first = philosopherNumber & 1 ? right : left;
   int second = philosopherNumber & 1 ? left : right;

   printf("Philosopher %d is waiting to pick up chopstick %d\n", philosopherNumber, first);
   // https://linux.die.net/man/3/pthread_mutex_lock
   // The mutex object referenced by mutex shall be locked by calling pthread_mutex_lock().
   // If the mutex is already locked, the calling thread shall block until the mutex becomes available.
   // This operation shall return with the mutex object referenced by mutex in the locked state with
   //   the calling thread as its owner.
   pthread_mutex_lock(&chopsticks[first]);
   printf("Philosopher %d picked up chopstick %d\n", philosopherNumber, first);

   printf("Philosopher %d is waiting to pick up chopstick %d\n", philosopherNumber, second);
   pthread_mutex_lock(&chopsticks[second]);
   printf("Philosopher %d picked up chopstick %d\n", philosopherNumber, second);
}

void eat(int philosopherNumber)
{
   int eatTime = (rand() % 3 + 1) * 100;
   printf("Philosopher %d will eat for %d micro-seconds\n", philosopherNumber, eatTime);
   usleep(eatTime);
}

void putDown(int philosopherNumber)
{
   printf("Philosopher %d will will put down her chopsticks\n", philosopherNumber);
   // https://linux.die.net/man/3/pthread_mutex_unlock
   pthread_mutex_unlock(&chopsticks[(philosopherNumber + 1) % NUMBER_OF_PHILOSOPHERS]);
   pthread_mutex_unlock(&chopsticks[(philosopherNumber + NUMBER_OF_PHILOSOPHERS) % NUMBER_OF_PHILOSOPHERS]);
}
