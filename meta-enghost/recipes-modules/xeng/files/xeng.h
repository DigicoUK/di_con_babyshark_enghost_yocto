int init_module(void);
void cleanup_module(void);

#define SUCCESS 0
#define DEVICE_NAME "xeng"	/* Dev name as it appears in /proc/devices   */

#define NUM_DEVICES	2	/* number of devices */
//#define FIFO_SIZE	65535	/* read & write fifo size */
