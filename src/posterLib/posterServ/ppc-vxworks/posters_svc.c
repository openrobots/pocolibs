#include <stdio.h>
#include <rpc/rpc.h>
#include "posters.h"

void
poster_serv_1(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	union {
		char *poster_find_1_arg;
		POSTER_CREATE_PAR poster_create_1_arg;
		POSTER_WRITE_PAR poster_write_1_arg;
		POSTER_READ_PAR poster_read_1_arg;
		int poster_delete_1_arg;
		POSTER_IOCTL_PAR poster_ioctl_1_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void)svc_sendreply(transp, xdr_void, (char *)NULL);
		return;

	case poster_find:
		xdr_argument = xdr_wrapstring;
		xdr_result = xdr_POSTER_FIND_RESULT;
		local = (char *(*)()) poster_find_1;
		break;

	case poster_create:
		xdr_argument = xdr_POSTER_CREATE_PAR;
		xdr_result = xdr_POSTER_CREATE_RESULT;
		local = (char *(*)()) poster_create_1;
		break;

	case poster_write:
		xdr_argument = xdr_POSTER_WRITE_PAR;
		xdr_result = xdr_int;
		local = (char *(*)()) poster_write_1;
		break;

	case poster_read:
		xdr_argument = xdr_POSTER_READ_PAR;
		xdr_result = xdr_POSTER_READ_RESULT;
		local = (char *(*)()) poster_read_1;
		break;

	case poster_delete:
		xdr_argument = xdr_int;
		xdr_result = xdr_int;
		local = (char *(*)()) poster_delete_1;
		break;

	case poster_ioctl:
		xdr_argument = xdr_POSTER_IOCTL_PAR;
		xdr_result = xdr_POSTER_IOCTL_RESULT;
		local = (char *(*)()) poster_ioctl_1;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	bzero((char *)&argument, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, &argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, &argument)) {
		(void)fprintf(stderr, "unable to free arguments\n");
		exit(1);
	}
}

