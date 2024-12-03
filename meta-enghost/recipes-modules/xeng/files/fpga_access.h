/*
 * NOTE.... Comment out when running on the real HW
 *
 */
// #define DEBUG    1

#define NUM_REGS	8
#define REGS_BYTE_SIZE	4

/*
 * Register offsets from the base_address
 * supplied when fpga_access_open is called
 */

typedef enum {
	FPGA_REG_INTERRUPT_PENDING_REG = 0,
	FPGA_REG_INTERRUPT_ENABLE_REG  = 1,
	FPGA_REG_INTERRUPT_CNTL_REG    = 2,
	FPGA_REG_READ_WRITE_TEST_REG   = 3,
	FPGA_REG_STREAMER_FIFO_DATA    = 4,
	FPGA_REG_AUDIO_FIFO_DATA       = 5,
	FPGA_REG_EXTENDED_ADDR_REG     = 6,
	FPGA_REG_EXTENDED_DATA_REG     = 7
} FPGA_REG;

/*
 * Register bit fields
 */

/* bit defines for the ICR */

#define GIE			0x00000001	// global interrupt enable
#define GTI			0x00000002	// generate test interrupt
#define RXSFTHR		0x00000700	// rx streamer fifo not empty threshold
#define TXSFTHR		0x00007000	// tx streamer fifo not full threshold
#define RXAFTHR		0x00070000	// rx audio fifo not empty threshold
#define TXAFTHR		0x00700000	// tx audio fifo not full threshold
#define SFTHR1		0x00000000	// 1 or more
#define SFTHR2		0x00000001	// 2 or more
#define SFTHR4		0x00000002	// 4 or more
#define SFTHR8		0x00000003	// 8 or more
#define SFTHR16		0x00000004	// 16 or more
#define SFTHR32		0x00000005	// 32 or more
#define SFTHR64		0x00000006	// 64 or more
#define SFTHR128	0x00000007	// 128 or more
#define AFTHR1		0x00000000	// 1 or more
#define AFTHR2		0x00000001	// 16 or more
#define AFTHR4		0x00000002	// 32 or more
#define AFTHR8		0x00000003	// 64 or more
#define AFTHR16		0x00000004	// 128 or more
#define AFTHR32		0x00000005	// 256 or more
#define AFTHR64		0x00000006	// 512 or more
#define AFTHR128	0x00000007	// 1024 or more

/* bit defines for the IPR, IER */
#define RX_STREAMER_FIFO_DEPTH_GREATER_THAN_THRES_INTERRUPT			0x00000001
#define TX_STREAMER_FIFO_AVAIL_SPACE_GREATER_THAN_THRES_INTERRUPT	0x00000002
#define RX_AUDIO_FIFO_DEPTH_GREATER_THAN_THRES_INTERRUPT			0x00000004
#define TX_AUDIO_FIFO_AVAIL_SPACE_GREATER_THAN_THRES_INTERRUPT		0x00000008
#define TEST_INTERRUPT												0x00000080

extern uint32_t *fpga_base_p;

extern void		fpga_read_rxafifo(uint32_t *buf, unsigned long count);
extern void		fpga_write_txafifo(uint32_t *buf, int count);
extern void		fpga_read_rxsfifo(uint32_t *buf, unsigned long count);
extern void 	fpga_write_txsfifo(uint32_t *buf, int count);
extern uint32_t fpga_read_icr(void);
extern void 	fpga_write_icr(uint32_t val);
extern uint32_t fpga_read_ipr(void);
extern uint32_t fpga_read_ier(void);
extern void 	fpga_write_ier(uint32_t val);
extern uint32_t fpga_read_test_reg(void);
extern void 	fpga_write_test_reg(uint32_t val);
extern uint32_t fpga_read_rx_data_available(void);
extern uint32_t fpga_read_tx_space_available(void);

extern void 	fpga_write_extended_data_reg(uint32_t val);
extern uint32_t fpga_read_extended_data_reg(void);
extern void 	fpga_write_extended_addr_reg(uint32_t val);
extern uint32_t fpga_read_extended_addr_reg(void);

extern uint32_t fpga_read_extended_reg(uint32_t addr);
extern void 	fpga_write_extended_reg(uint32_t addr, uint32_t val);

extern int 		fpga_access_open(unsigned long base_addr, unsigned long len, char *name);
extern int 		fpga_access_close(unsigned long base_addr, unsigned long len);
