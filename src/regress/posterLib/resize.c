/*
 * Copyright (c) 2012 LAAS/CNRS
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
 *					Anthony Mallet on Mon Mar  5 2012
 */
#include "pocolibs-config.h"

#include <assert.h>
#include <err.h>
#include <stdio.h>

#include "portLib.h"
#include "posterLib.h"

static void
pocoregress_test(POSTER_ID p)
{
  unsigned char buffer[256];
  unsigned char *a;
  int i, n;

  for(i = 0; i<256; i++) buffer[i] = i;
  n = posterWrite(p, 0, buffer, 256);
  logMsg("wrote %d\n", n);

  n = posterRead(p, 0, buffer, 256);
  logMsg("read %d\n", n);
  for(i = 0; i<n; i++) assert(buffer[i] == i);

  posterTake(p, POSTER_WRITE);
  a = posterAddr(p);
  for(i = 0; i<n; i++) a[i] = 255-i;
  posterGive(p);

  posterTake(p, POSTER_READ);
  a = posterAddr(p);
  for(i = 0; i<n; i++) assert(a[i] == 255-i);
  posterGive(p);
  logMsg("addr %d ok\n", n);

  n = posterRead(p, 0, buffer, 256);
  logMsg("read %d\n", n);
  for(i = 0; i<n; i++) assert(buffer[i] == 255-i);

  for(i = 0; i<256; i++) buffer[i] = i;
  n = posterWrite(p, 0, buffer, 256);
  logMsg("wrote %d\n", n);

  posterTake(p, POSTER_READ);
  a = posterAddr(p);
  for(i = 0; i<n; i++) assert(a[i] == i);
  posterGive(p);
  logMsg("addr %d ok\n", n);
}

int
pocoregress_init()
{
  STATUS s;
  POSTER_ID p;
  size_t l;
  char *e;

  e = getenv("POSTER_HOST");
  logMsg("POSTER_HOST: %s\n", e?e:"(none)");
  e = getenv("POSTER_PATH");
  logMsg("POSTER_PATH: %s\n", e?e:"(none)");

  s = posterCreate("pipo", 64, &p);
  if (s != OK) { h2perror("create"); errx(2, "create"); }

  logMsg("writing 256 bytes on poster of 64 bytes\n");

  pocoregress_test(p);

  logMsg("writing 256 bytes on poster of 256 bytes\n");

  l = 256;
  s = posterIoctl(p, FIO_RESIZE, &l);
  if (s != OK) { h2perror("ioctl"); errx(2, "ioctl"); }

  pocoregress_test(p);

  logMsg("writing 256 bytes on poster of 128 bytes\n");

  l = 128;
  s = posterIoctl(p, FIO_RESIZE, &l);
  if (s != OK) { h2perror("ioctl"); errx(2, "ioctl"); }

  pocoregress_test(p);

  logMsg("testing resize to 1<<31 bytes\n");

  l = 1<<31;
  s = posterIoctl(p, FIO_RESIZE, &l);
  if (s == OK) { errx(2, "memory exhaustion not detected"); }

  pocoregress_test(p);

  s = posterDelete(p);
  if (s != OK) { h2perror("delete"); errx(2, "delete"); }

  logMsg("memory fragmentation");
  smMemShow(1);

  return 0;
}
