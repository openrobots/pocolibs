/*
 * Copyright (c) 2010 CNRS/LAAS
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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

#ifdef HAVE_SYS_TREE_H
#include <sys/tree.h>
#else
#include "tree.h"
#endif

#include <limits.h>
#include <stdlib.h>
#include "remPosterId.h"

struct ptr_id {
	RB_ENTRY(ptr_id) entry;
	const void *ptr;
	int id;
};

static int remotePosterId = 0;

static int
ptrcmp(struct ptr_id *e1, struct ptr_id *e2)
{

	return(e1->id < e2->id ? -1 : e1->id > e2->id);
}

RB_HEAD(ptrIdTree, ptr_id) head = RB_INITIALIZER(&head);
RB_GENERATE(ptrIdTree, ptr_id, entry, ptrcmp)

int
remposterIdAlloc(const void *ptr)
{
	struct ptr_id *id;

	id = (struct ptr_id *)malloc(sizeof(struct ptr_id));
	if (id == NULL)
		return -1;
	if (remotePosterId == INT_MAX)
		return -1;
	id->ptr = ptr;
	id->id = remotePosterId++;
	RB_INSERT(ptrIdTree, &head, id);
	return id->id;
}

const void *
remposterIdLookup(int id)
{
	struct ptr_id ptrid, *result;

	ptrid.id = id;
	result = RB_FIND(ptrIdTree, &head, &ptrid);
	if (result == NULL)
		return NULL;
	return result->ptr;
}

void
remposterIdRemove(int id)
{
	struct ptr_id ptrid, *result;

	ptrid.id = id;
	result = RB_FIND(ptrIdTree, &head, &ptrid);
	if (result != NULL)
		RB_REMOVE(ptrIdTree, &head, result);
}
