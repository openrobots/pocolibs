#include <rpc/rpc.h>
#include "posters.h"


bool_t
xdr_POSTER_STATUS(xdrs, objp)
	XDR *xdrs;
	POSTER_STATUS *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_POSTER_FIND_RESULT(xdrs, objp)
	XDR *xdrs;
	POSTER_FIND_RESULT *objp;
{
	if (!xdr_int(xdrs, &objp->status)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->length)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->endianness)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_POSTER_CREATE_PAR(xdrs, objp)
	XDR *xdrs;
	POSTER_CREATE_PAR *objp;
{
	if (!xdr_string(xdrs, &objp->name, 256)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->length)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->endianness)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_POSTER_CREATE_RESULT(xdrs, objp)
	XDR *xdrs;
	POSTER_CREATE_RESULT *objp;
{
	if (!xdr_int(xdrs, &objp->status)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->id)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_POSTER_WRITE_PAR(xdrs, objp)
	XDR *xdrs;
	POSTER_WRITE_PAR *objp;
{
	if (!xdr_int(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->offset)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->length)) {
		return (FALSE);
	}
	if (!xdr_array(xdrs, (char **)&objp->data.data_val, (u_int *)&objp->data.data_len, ~0, sizeof(char), xdr_char)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_POSTER_READ_PAR(xdrs, objp)
	XDR *xdrs;
	POSTER_READ_PAR *objp;
{
	if (!xdr_int(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->offset)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->length)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_POSTER_READ_RESULT(xdrs, objp)
	XDR *xdrs;
	POSTER_READ_RESULT *objp;
{
	if (!xdr_int(xdrs, &objp->status)) {
		return (FALSE);
	}
	if (!xdr_array(xdrs, (char **)&objp->data.data_val, (u_int *)&objp->data.data_len, ~0, sizeof(char), xdr_char)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_POSTER_IOCTL_PAR(xdrs, objp)
	XDR *xdrs;
	POSTER_IOCTL_PAR *objp;
{
	if (!xdr_int(xdrs, &objp->id)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->cmd)) {
		return (FALSE);
	}
	return (TRUE);
}




bool_t
xdr_POSTER_IOCTL_RESULT(xdrs, objp)
	XDR *xdrs;
	POSTER_IOCTL_RESULT *objp;
{
	if (!xdr_int(xdrs, &objp->status)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->ntick)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->msec)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->sec)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->minute)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->hour)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->day)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->date)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->month)) {
		return (FALSE);
	}
	if (!xdr_u_short(xdrs, &objp->year)) {
		return (FALSE);
	}
	return (TRUE);
}


