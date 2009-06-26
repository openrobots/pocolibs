/* 
 * Copyright (c) 2009 LAAS/CNRS
 * All rights reserved.
 *
 * Redistribution and use  in source  and binary  forms,  with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of  source  code must retain the  above copyright
 *      notice and this list of conditions.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice and  this list of  conditions in the  documentation and/or
 *      other materials provided with the distribution.
 *
 *					Anthony Mallet on Tue Jun  9 2009
 */

#include <stdio.h>
#include <time.h>

#include "portLib.h"
#include "h2semLib.h"
#include "taskLib.h"

#define PING_PONG_STEPS	11

H2SEM_ID  sync;
H2SEM_ID  sem[2];

static int count;

int
pocoregress_ping(int side)
{
  while (1) {
    h2semTake(sem[side], WAIT_FOREVER);

    logMsg("%s", side ? "ping..." : " ...pong\n");
    if (side) ++count;

    if (count >= PING_PONG_STEPS) break;
    h2semGive(sem[1-side]);
  }

  h2semGive(sync);
  return 0;
}

int
pocoregress_unlock()
{
  struct timespec wreq = { tv_sec: 2, tv_nsec: 0 };
  struct timespec rreq;

  while (nanosleep(&wreq, &rreq))
    wreq = rreq;

  logMsg("unlocking deadlock\n");
  count = 1;
  h2semGive(sem[0]);
  return 0;
}

int
pocoregress_incr()
{
  h2semTake(sem[0], WAIT_FOREVER);
  count++;
  h2semGive(sync);
  return 0;
}


int
pocoregress_init()
{
  sync = h2semAlloc(H2SEM_SYNC);

  sem[0] = h2semAlloc(H2SEM_EXCL);
  sem[1] = h2semAlloc(H2SEM_EXCL);
  if (sync == ERROR || sem[0] == ERROR || sem[1] == ERROR) {
    logMsg("Error: could not create h2sem\n");
    return 2;
  }

  /* ----------------------------------------------------------------------- */

  logMsg("* testing recursive behaviour (should deadlock for two seconds)\n");

  count = 0;
  h2semGive(sem[0]);
  h2semGive(sem[0]); /* 2nd give must be a noop */

  h2semTake(sem[0], WAIT_FOREVER);
  taskSpawn("unlock", 200, VX_FP_TASK, 20000, pocoregress_unlock);
  /* must block forever, the task will unblock in 2 seconds */
  h2semTake(sem[0], WAIT_FOREVER);
  if (count == 0) {
    logMsg("two successive h2semGive() is not noop: wrong\n", count);
    return 2;
  }


  /* ----------------------------------------------------------------------- */

  logMsg("* testing wake up of blocked tasks\n");

  count = 0;
  h2semFlush(sem[0]);
  h2semFlush(sync);
  taskSpawn("unlock", 200, VX_FP_TASK, 20000, pocoregress_incr);
  taskSpawn("unlock", 200, VX_FP_TASK, 20000, pocoregress_incr);

  if (count != 0) {
    logMsg("h2semTake() did not block: wrong\n");
    return 2;
  }
  h2semGive(sem[0]);
  h2semTake(sync, 200);
  if (count != 1) {
    logMsg("h2semGive() did not wake up task: wrong\n");
    return 2;
  }

  h2semGive(sem[0]);
  h2semTake(sync, 200);
  if (count != 2) {
    logMsg("h2semGive() did not wake up task: wrong\n");
    return 2;
  }


  /* ----------------------------------------------------------------------- */

  logMsg("* playing ping pong for %d rounds\n", PING_PONG_STEPS);
  count = 0;
  h2semFlush(sem[0]);
  h2semFlush(sem[1]);
  h2semGive(sem[1]);
  taskSpawn("ping", 200, VX_FP_TASK, 20000, pocoregress_ping, 0);
  taskSpawn("pong", 200, VX_FP_TASK, 20000, pocoregress_ping, 1);

  h2semTake(sync, WAIT_FOREVER);
  h2semFlush(sync);
  if (count != PING_PONG_STEPS) {
    logMsg("Did %d ping pongs: wrong\n", count);
    return 2;
  }


  /* ----------------------------------------------------------------------- */

  h2semDelete(sem[0]);
  h2semDelete(sem[1]);
  h2semDelete(sync);

  return 0;
}
