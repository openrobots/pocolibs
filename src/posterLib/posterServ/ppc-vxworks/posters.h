
enum POSTER_STATUS {
	POSTER_OK = 0,
	POSTER_ERROR = 1,
};
typedef enum POSTER_STATUS POSTER_STATUS;
bool_t xdr_POSTER_STATUS();


struct POSTER_FIND_RESULT {
	int status;
	int id;
	int length;
	int endianness;
};
typedef struct POSTER_FIND_RESULT POSTER_FIND_RESULT;
bool_t xdr_POSTER_FIND_RESULT();


struct POSTER_CREATE_PAR {
	char *name;
	int length;
	int endianness;
};
typedef struct POSTER_CREATE_PAR POSTER_CREATE_PAR;
bool_t xdr_POSTER_CREATE_PAR();


struct POSTER_CREATE_RESULT {
	int status;
	int id;
};
typedef struct POSTER_CREATE_RESULT POSTER_CREATE_RESULT;
bool_t xdr_POSTER_CREATE_RESULT();


struct POSTER_WRITE_PAR {
	int id;
	int offset;
	int length;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct POSTER_WRITE_PAR POSTER_WRITE_PAR;
bool_t xdr_POSTER_WRITE_PAR();


struct POSTER_READ_PAR {
	int id;
	int offset;
	int length;
};
typedef struct POSTER_READ_PAR POSTER_READ_PAR;
bool_t xdr_POSTER_READ_PAR();


struct POSTER_READ_RESULT {
	int status;
	struct {
		u_int data_len;
		char *data_val;
	} data;
};
typedef struct POSTER_READ_RESULT POSTER_READ_RESULT;
bool_t xdr_POSTER_READ_RESULT();


struct POSTER_IOCTL_PAR {
	int id;
	int cmd;
};
typedef struct POSTER_IOCTL_PAR POSTER_IOCTL_PAR;
bool_t xdr_POSTER_IOCTL_PAR();


struct POSTER_IOCTL_RESULT {
	int status;
	u_long ntick;
	u_short msec;
	u_short sec;
	u_short minute;
	u_short hour;
	u_short day;
	u_short date;
	u_short month;
	u_short year;
};
typedef struct POSTER_IOCTL_RESULT POSTER_IOCTL_RESULT;
bool_t xdr_POSTER_IOCTL_RESULT();


#define POSTER_SERV ((u_long)600000001)
#define POSTER_VERSION ((u_long)1)
#define poster_find ((u_long)1)
extern POSTER_FIND_RESULT *poster_find_1();
#define poster_create ((u_long)2)
extern POSTER_CREATE_RESULT *poster_create_1();
#define poster_write ((u_long)3)
extern int *poster_write_1();
#define poster_read ((u_long)4)
extern POSTER_READ_RESULT *poster_read_1();
#define poster_delete ((u_long)5)
extern int *poster_delete_1();
#define poster_ioctl ((u_long)6)
extern POSTER_IOCTL_RESULT *poster_ioctl_1();

