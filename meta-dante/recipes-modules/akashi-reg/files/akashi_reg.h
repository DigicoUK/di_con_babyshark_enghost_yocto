/*
 * File : akashi_reg.h
 * Created : April 2022
 * Authors : Martin Siu
 * Synopsis: Akashi Register Interface device driver head file.
 *
 * Copyright 2022 Audinate Pty Ltd and/or its licensors
 *
 */

#define AREG(x) (*(volatile uint32_t *)(x))

#define AUD_SYD_BASEADDR (virtual_base_addr)
#define AUD_SYD_HIGHADDR (virtual_base_addr + 0x7FFFF))
#define DANTE_REG_0_BASE_ADDR   	(AUD_SYD_BASEADDR + 0x0)
#define AUD_SYD_FPGA_VERSION		(*(volatile u32 *)(AUD_SYD_BASEADDR + 0x4))
#define AUD_SYD_FPGA_VERSION_MAJOR	((AUD_SYD_FPGA_VERSION & 0xff000000) >> 24)
#define AUD_SYD_FPGA_VERSION_MINOR	((AUD_SYD_FPGA_VERSION & 0x00ff0000) >> 16)
#define AUD_SYD_FPGA_VERSION_PATCH	((AUD_SYD_FPGA_VERSION & 0x0000ff00) >> 8)
#define AUD_SYD_FPGA_VERSION_RC		((AUD_SYD_FPGA_VERSION & 0xff))


#define KA_INFO_QUEUE_SIZE			65
#define RX_AUD_DATA_HDR_SIZE		48 // Bytes

#define AUD_SYD_SCHED_RX_HEADER_AUDIO_INT_ENABLE (1 << 1)
#define AUD_SYD_SCHED_RX_HEADER_AUDIO_FIFO_CLEAR (1 << 0)

#define AUD_SYD_SCHED_FIFO_OCC  (*(volatile u32 *)(AUD_SYD_BASEADDR + AUD_SYD_SCHED_BASEADDR + 0xc)) // 15:0 Tx FIFO Free (32 bit words)
#define AUD_SYD_SCHED_FIFO_AUDIO_NOT_EMPTY_MASK 0x20000

#define AUD_SYD_APE_BASEADDR	0x10000
#define AUD_SYD_SCHED_BASEADDR 	0x24000


#define AUD_METERING_INT 		(*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x3010))

//DAIF

//APE Control
#define AUD_SYD_APE_CONTROL				(*(volatile uint32_t *)(AUD_SYD_BASEADDR + AUD_SYD_APE_BASEADDR + 0x1400))
#define AUD_SYD_APE_INT_ENABLE			(0x1 << 16)


// Audio FIFO
#define AUD_SYD_SCHED_AUDIO_FIFO_CONTROL	(*(volatile uint32_t *)(AUD_SYD_BASEADDR + AUD_SYD_SCHED_BASEADDR + 0x40))
#define AUD_SYD_SCHED_RX_AUDIO_FIFO		(*(volatile uint32_t *)(AUD_SYD_BASEADDR + AUD_SYD_SCHED_BASEADDR + 0xc00))

#define AUD_SYD_DAIF_RX_ERROR_THRES             (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C004))
#define AUD_SYD_DAIF_RX_CHAN_ENABLE_0           (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C008))
#define AUD_SYD_DAIF_RX_CHAN_ENABLE_1           (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C00c))
#define AUD_SYD_DAIF_RX_THRES_ENABLE_0          (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C010))
#define AUD_SYD_DAIF_RX_THRES_ENABLE_1          (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C014))
#define AUD_SYD_DAIF_RX_COUNTER_ENABLE_0        (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C018))
#define AUD_SYD_DAIF_RX_COUNTER_ENABLE_1        (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C01c))
#define AUD_SYD_DAIF_RX_ERROR_0                 (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C020))
#define AUD_SYD_DAIF_RX_ERROR_1                 (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C024))
#define AUD_SYD_DAIF_RX_ERROR_MASK_0            (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C028))
#define AUD_SYD_DAIF_RX_ERROR_MASK_1            (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C02c))
#define AUD_SYD_DAIF_RX_ERROR_COUNTER_BASE      (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x1C400))

// Audio Activity
#define AUD_SYD_SCHED_RX_BUNDLE_ACTIVITY_IRQ_MASK	(*(volatile uint32_t *)(AUD_SYD_BASEADDR + AUD_SYD_SCHED_BASEADDR + (0x00*4)))
#define AUD_SYD_SCHED_RX_KA_IRQ_MASK			(*(volatile uint32_t *)(AUD_SYD_BASEADDR + AUD_SYD_SCHED_BASEADDR + (0x00*4)))
#define AUD_SYD_SCHED_RX_BUNDLE_ACTIVITY_IRQ_ENABLE 	(0x3 <<  8)
#define AUD_SYD_SCHED_RX_KA_MISS_IRQ_ENABLE     	(0x3 << 10)

/* For Reading Device ID from FPGA */
#define AUD_SYD_ICAP_DEVICE_ID_READ_CMD_RSP      (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x208))
#define AUD_SYD_INFO_DEVICE_IDCODE               (*(volatile uint32_t *)(AUD_SYD_BASEADDR + 0x68))

#ifdef CONFIG_PLATFORM_AUDINATE_CAP6SYD

//--------------------------------------------------------------------------------
// Sydney Registers - RXERR
#define AUD_SYD_RXERR_BASEADDR        0x2800
#define AUD_SYD_RXERR_CONTROL         (*(volatile uint32_t *)(AUD_SYD_BASEADDR + AUD_SYD_RXERR_BASEADDR + 0x04))
// RXERR Main Control Register Masks
#define AUD_SYD_RXERR_CONTROL_INT_EN  0x00000004

#endif /* CONFIG_PLATFORM_AUDINATE_CAP6SYD */

#ifdef CONFIG_AKASHI_REG_SPI_HP
// SPI Slave Interface register start address
#define AKASHI_SPI_REG_SLAVE_ADDR       (spi_slave_virtual_base_addr)
// SPI Slave Interface interrupt resources
#define AKASHI_SPI_INT_MASK_REG         *(volatile uint32_t*)(AKASHI_SPI_REG_SLAVE_ADDR+0x800)	// use bit 0 for enabling.
#define AKASHI_SPI_INT_STATUS_REG       *(volatile uint32_t*)(AKASHI_SPI_REG_SLAVE_ADDR+0x804)	// write for clearing.
#endif /* CONFIG_AKASHI_REG_SPI_HP */