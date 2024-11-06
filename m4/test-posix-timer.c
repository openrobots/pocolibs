/* Public domain - Anthony Mallet on Mon Nov  4 2024 */

#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

static volatile int ticks;

/* SIGALRM handler */
void
tick(int arg)
{
  (void)arg; /* unused */

  /* global counter - even if access is not atomic, we don't care here as the
   * exact value is not used, only the fact that the value changes is relevant
   */
  ticks++;
}

/* thread forking thread */
void *
thr(void *arg)
{
  pthread_attr_t attr;
  pthread_t t;
  (void)arg; /* unused */

  /* spwan a new thread in detached state so that we don't grow too much */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&t, &attr, thr, NULL))
    err(2, "pthread_create");

  return NULL;
}

int
main()
{
  int hz = 1000; /* 1kHz timer - the higher, the faster the issue happens */

  struct sigaction act;
  struct itimerspec tv;
  struct timespec pts, ts, rem;
  sigset_t sigset;
  timer_t timer;
  int i, c1, c2;

  /* SIGALRM handler */
  act.sa_handler = tick;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if (sigaction(SIGALRM, &act, NULL) == -1)
    err(2, "sigaction");

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGALRM);
  if (pthread_sigmask(SIG_UNBLOCK, &sigset, NULL) == -1)
    err(2, "pthread_sigmask");


  /* SIGALRM timer at 'hz' frequency */
  if (timer_create(CLOCK_REALTIME, NULL, &timer) == -1)
    err(2, "timer_create");

  tv.it_interval.tv_nsec = 1000000000/hz;
  tv.it_interval.tv_sec = 0;
  tv.it_value = tv.it_interval;


  /* thread forking threads - this is an issue spotted on ubuntu-22.04 and
   * 24.04, as well as other architectures, that affects timer signal
   * delivrery. This seems to affect kernels from 6.4 to 6.11 inclusive. */
  thr(NULL);


  /* start timer */
  if (timer_settime(timer, 0, &tv, NULL) == -1)
    err(2, "timer_settime");

  /* 100 periods delay */
  pts.tv_sec = 0;
  pts.tv_nsec = tv.it_interval.tv_nsec * 100; /* 100ms */
  while(pts.tv_nsec >= 1000000000) {
    pts.tv_nsec -= 1000000000;
    pts.tv_sec++;
  }
  /* for 1s */
  for (i = 0; i < 10; i++) {
    ts = pts;
    c1 = ticks;
    while (nanosleep(&ts, &rem) != 0 && errno == EINTR) ts = rem;
    c2 = ticks;

    if (c1 == c2) {
      /* the counter is stuck, SIGALRM not firing anymore */
      fprintf(stderr, "SIGALRM issue after %d ticks\n", c1);
      return 2;

      /* just resetting the timer at this point makes it work again: */
      /* if (timer_settime(timer, 0, &tv, NULL) == -1) */
      /*   err(2, "timer_settime"); */
      /* but the issue will trigger again after some time */

      /* also note that timer_gettime(timer, &tv) will show both correct
       * tv.it_interval and tv.it_value changing normally */

      /* manually sending SIGALRM also still works: */
      /* raise(SIGALRM); */
    }
  }

  return 0;
}
