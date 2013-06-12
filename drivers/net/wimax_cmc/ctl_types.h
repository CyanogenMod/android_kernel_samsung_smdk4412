/*
 * ctl_types.h
 *
 * Control types and definitions
 */
#ifndef _WIMAX_CTL_TYPES_H
#define _WIMAX_CTL_TYPES_H

#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x89
#endif

/* Macro definition for defining IOCTL */
#define CTL_CODE( DeviceType, Function, Method, Access )	\
(	\
	((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
/* Define the method codes for how buffers are passed for I/O and FS controls */
#define METHOD_BUFFERED			0
#define METHOD_IN_DIRECT		1
#define METHOD_OUT_DIRECT		2
#define METHOD_NEITHER			3

/*
	Define the access check value for any access

	The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
	ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
	constants *MUST* always be in sync.
*/
#define FILE_ANY_ACCESS			0
#define FILE_READ_ACCESS		(0x0001)
#define FILE_WRITE_ACCESS		(0x0002)

#define CONTROL_ETH_TYPE_WCM		0x0015

#define	CONTROL_IOCTL_WRITE_REQUEST		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x820, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_POWER_CTL		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_MODE_CHANGE		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x838, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x839, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_SLEEP_MODE		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83A, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_WRITE_REV		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83B, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_CHECK_CERT		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83C, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_CHECK_CAL		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83D, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MAX_APPLICATION_LEN		50

#define ETHERNET_ADDRESS_LENGTH		6

/* eth types for control message */
enum {
	ETHERTYPE_HIM	= 0x1500,
	ETHERTYPE_MC	= 0x1501,
	ETHERTYPE_DM	= 0x1502,
	ETHERTYPE_CT	= 0x1503,
	ETHERTYPE_DL	= 0x1504,
	ETHERTYPE_VSP	= 0x1510,
	ETHERTYPE_AUTH	= 0x1521
};

/* eth header structure */
#pragma pack(1)
struct eth_header {
	u_char		dest[ETHERNET_ADDRESS_LENGTH];
	u_char		src[ETHERNET_ADDRESS_LENGTH];
	u_short		type;
};
#pragma pack()

/* process element managed by control type */
struct process_descriptor {
	struct list_head	node;
	wait_queue_head_t	read_wait;
	u_long		id;
	u_short		type;
	u_short		irp;	/* Used for the read thread indication */
};

/* head structure for process element */
struct ctl_app_descriptor {
	struct list_head	process_list;	/* there could be undefined number of applications */
	spinlock_t		lock;
	u_char		ready;
};

struct ctl_info {
	struct ctl_app_descriptor	apps;		/* application device structure */
	struct queue_info		q_received;	/* pending queue */
};

#endif	/* _WIMAX_CTL_TYPES_H */
