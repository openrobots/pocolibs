/* $LAAS$ */
/*
 * Copyright (c) 1991,2003 CNRS/LAAS
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
#ifndef _CSLIBPROTO_H
#define _CSLIBPROTO_H
#ifdef __STDC__

extern STATUS csClientEnd ( CLIENT_ID clientId );
extern STATUS csClientInit ( char *servMboxName, int maxRqstSize, int maxIntermedReplySize, int maxFinalReplySize, CLIENT_ID *pClientId );
extern int csClientReplyRcv ( CLIENT_ID clientId, int rqstId, int block, char *intermedReplyDataAdrs, int intermedReplyDataSize, FUNCPTR intermedReplyDecodFunc, char *finalReplyDataAdrs, int finalReplyDataSize, FUNCPTR finalReplyDecodFunc );
extern int csClientRqstIdFree ( CLIENT_ID clientId, int rqstId );
extern STATUS csClientRqstSend ( CLIENT_ID clientId, int rqstType, char *rqstDataAdrs, int rqstDataSize, FUNCPTR codFunc, BOOL intermedFlag, int intermedReplyTout, int finalReplyTout, int *pRqstId );
extern STATUS csMboxEnd ( void );
extern STATUS csMboxInit ( char *mboxBaseName, int rcvMboxSize, int replyMboxSize );
extern int csMboxStatus ( int mask );
extern int csMboxWait ( int timeout, int mboxMask );
extern STATUS csServEnd ( SERV_ID servId );
extern STATUS csServFuncInstall ( SERV_ID servId, int rqstType, FUNCPTR rqstFunc );
extern STATUS csServInit ( int maxRqstDataSize, int maxReplyDataSize, SERV_ID *pServId );
extern STATUS csServInitN ( int maxRqstDataSize, int maxReplyDataSize, int nbRqstFunc, SERV_ID *pServId );
extern STATUS csServReplySend ( SERV_ID servId, int rqstId, int replyType, int replyBilan, char *replyDataAdrs, int replyDataSize, FUNCPTR codFunc );
extern STATUS csServRqstExec ( SERV_ID servId );
extern STATUS csServRqstIdFree ( SERV_ID servId, int rqstId );
extern STATUS csServRqstParamsGet ( SERV_ID servId, int rqstId, char *rqstDataAdrs, int rqstDataSize, FUNCPTR decodFunc );

#else /* __STDC__ */

extern STATUS csClientEnd (/* CLIENT_ID clientId */);
extern STATUS csClientInit (/* char *servMboxName, int maxRqstSize, int maxIntermedReplySize, int maxFinalReplySize, CLIENT_ID *pClientId */);
extern int csClientReplyRcv (/* CLIENT_ID clientId, int rqstId, int block, char *intermedReplyDataAdrs, int intermedReplyDataSize, FUNCPTR intermedReplyDecodFunc, char *finalReplyDataAdrs, int finalReplyDataSize, FUNCPTR finalReplyDecodFunc */);
extern int csClientRqstIdFree (/* CLIENT_ID clientId, int rqstId */);
extern STATUS csClientRqstSend (/* CLIENT_ID clientId, int rqstType, char *rqstDataAdrs, int rqstDataSize, FUNCPTR codFunc, BOOL intermedFlag, int intermedReplyTout, int finalReplyTout, int *pRqstId */);
extern STATUS csMboxEnd (/* void */);
extern STATUS csMboxInit (/* char *mboxBaseName, int rcvMboxSize, int replyMboxSize */);
extern int csMboxStatus (/* int mask */);
extern int csMboxWait (/* int timeout, int mboxMask */);
extern STATUS csServEnd (/* SERV_ID servId */);
extern STATUS csServFuncInstall (/* SERV_ID servId, int rqstType, FUNCPTR rqstFunc */);
extern STATUS csServInit (/* int maxRqstDataSize, int maxReplyDataSize, SERV_ID *pServId */);
extern STATUS csServReplySend (/* SERV_ID servId, int rqstId, int replyType, int replyBilan, char *replyDataAdrs, int replyDataSize, FUNCPTR codFunc */);
extern STATUS csServRqstExec (/* SERV_ID servId */);
extern STATUS csServRqstIdFree (/* SERV_ID servId, int rqstId */);
extern STATUS csServRqstParamsGet (/* SERV_ID servId, int rqstId, char *rqstDataAdrs, int rqstDataSize, FUNCPTR decodFunc */);

#endif /* __STDC__ */
#endif /* _CSLIBPROTO_H */
