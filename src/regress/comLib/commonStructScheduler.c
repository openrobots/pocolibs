#include <portLib.h>

#include <sys/select.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <commonStructLib.h>
#include <h2initGlob.h>
#include <taskLib.h>

int spawned = 0;
char common[512] = "";
void *cs;
pthread_barrier_t barrier;


void *
t(void *arg)
{
  commonStructTake(cs);
  spawned++;
  while(1) {
    commonStructGive(cs);
    commonStructTake(cs);

    if (spawned < 2) continue;

    strcat(common, arg);
    if (strlen(common) >= sizeof(common)-2) break;
  }
  spawned++;
  commonStructGive(cs);
  pthread_barrier_wait(&barrier);

  return NULL;
}

int
pocoregress_init()
{
  commonStructCreate(1, &cs);
  if (!cs) exit(2);

  pthread_barrier_init(&barrier, NULL, 3);

  taskSpawn2("t1", 255, 0, 1024, t, "1");
  taskSpawn2("t2", 255, 0, 1024, t, "2");

  pthread_barrier_wait(&barrier);

  if (strstr(common, "11") || strstr(common, "22")) {
    printf("scheduling = %s\n", common);
    return 2;
  }

  printf("scheduling is fair\n");
  return 0;
}
