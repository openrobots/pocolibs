/*
 * Copyright (c) 2024 CNRS/LAAS
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "pocolibs-config.h"

#include <stdio.h>

#include "mboxLib.h"

int
pocoregress_init()
{
  MBOX_ID id, id2;
  int sz;

  if (mboxInit("recycle") == ERROR) {
    logMsg("Error: could not initialize mbox\n");
    return 2;
  }

  /* creation */
  if (mboxCreate("recycle", 1, &id) != OK) {
    logMsg("Error: could not create mbox\n");
    return 2;
  }
  if (mboxIoctl(id, FIO_NBYTES, (void *)&sz) != OK) {
    logMsg("Error: could get create mbox size\n");
    return 2;
  }

  /* destruction */
  if (mboxDelete(id) != OK) {
    logMsg("Error: could not delete mbox\n");
    return 2;
  }
  if (mboxIoctl(id, FIO_NBYTES, (void *)&sz) == OK) {
    logMsg("Error: could get deleted mbox size\n");
    return 2;
  }

  /* recreation */
  if (mboxCreate("recycle", 1, &id2) != OK) {
    logMsg("Error: could not recreate mbox\n");
    return 2;
  }
  if (mboxIoctl(id, FIO_NBYTES, (void *)&sz) == OK) {
    logMsg("Error: could get recreated mbox size\n");
    return 2;
  }

  return 0;
}
