/* $LAAS$ */
/*
 * Copyright (c) 1991, 2003 CNRS/LAAS
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

#ifndef _GCOMLIBPROTO_H
#define _GCOMLIBPROTO_H

BOOL gcomLetterRcv (LETTER_ID letter, MBOX_ID *pOrigMboxId, int *pSendId, int timeout);
STATUS gcomEnd (void);
STATUS gcomInit (char *procName, int rcvMboxSize, int replyMboxSize);
STATUS gcomLetterAlloc (int sizeLetter, LETTER_ID *pLetterId);
STATUS gcomLetterDiscard (LETTER_ID letterId);
STATUS gcomLetterReply (MBOX_ID mboxId, int sendId, int replyLetterType, LETTER_ID letterReply);
STATUS gcomLetterWrite (LETTER_ID letterId, int dataType, char *pData, int dataSize, FUNCPTR codingFunc);
STATUS gcomMboxFind (char *procName, MBOX_ID *pMboxId);
STATUS gcomMboxName (MBOX_ID mboxId, char *pName);
STATUS gcomSendIdFree (int sendId);
int gcomLetterList (LETTER_ID *pList, int maxList);
int gcomLetterRead (LETTER_ID letterId, char *pData, int maxDataSize, FUNCPTR decodingFunc);
int gcomLetterSend (MBOX_ID mboxId, LETTER_ID sendLetter, LETTER_ID intermedReplyLetter, LETTER_ID finalReplyLetter, int block, int *pSendId, int finalReplyTout, int intermedReplyTout);
int gcomLetterType (LETTER_ID letterId);
int gcomMboxPause (int timeout, int mask);
int gcomMboxStatus (int mask);
int gcomReplyStatus (int sendId);
int gcomReplyWait (int sendId, int replyLetterType);
int gcomSendIdList (int *pList, int maxList);
void gcomMboxShow (void);
void gcomReplyLetterBySendId (int sendId, LETTER_ID *pIntermedReplyLetter, LETTER_ID *pFinalReplyLetter);
void gcomSelect(char *sendIdTable);

#endif /* _GCOMLIBPROTO_H */
