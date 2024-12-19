#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <gpiod.h>
#include <linux/spi/spidev.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

 #define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _b : _a; })

#define N_SHARCS 3
#define SHARC_RESET_CHIP "/dev/gpiochip1"
static unsigned int const sharc_reset_line_ids[N_SHARCS] = {0, 1, 2};
static char const* sharc_spi_devs[N_SHARCS] =
{
    "/dev/spidev2.1",
    "/dev/spidev2.2",
    "/dev/spidev2.3",
};

#define MODE 0 // SPI_CPHA | SPI_CPOL; SPI_LSB_FIRST;
#define BITS 0
#define SPI_SPEED 30000000
#define DELAY 200

#define INITIAL_FIRMWARE_LOAD_SIZE 384

static struct gpiod_line_request *
request_output_line(const char *chip_path, unsigned int offset,
                    enum gpiod_line_value initial_value, const char *consumer);
static bool reset_sharc(int sharc_index);
static int init_spi(char const* device_filename);
static bool load_file(int spi_device_fd, FILE *fp, int size);
static bool transfer(int spi_device_fd, uint8_t *tx, uint8_t *rx, unsigned long bufSize);
static unsigned char reverseBitOrder(unsigned char b);

int main(int argc, char** argv)
{
    if(argc != 3)
    {
        printf("USAGE: %s [sharc_firmware] [sharc_index]\n", argv[0]);
        return 1;
    }

    char* p;
    errno = 0;
    long sharc_index = strtol(argv[2], &p, 10);
    if (*p != '\0' || errno)
    {
        fprintf(stderr, "Invalid sharc index %s\n", argv[2]);
        return 1;
    }

    if ((sharc_index < 0) || (sharc_index >= N_SHARCS))
    {
        fprintf(stderr, "Invalid sharc index %lu\n", sharc_index);
        return 1;
    }

    FILE *firmware = fopen(argv[1], "rb");
    if(!firmware) {
        fprintf(stderr, "Failed to open firmware file %s\n", argv[1]);
        return 1;
    }

    int spi_device_fd = init_spi(sharc_spi_devs[sharc_index]);
    if (spi_device_fd < 0)
    {
        fprintf(stderr, "Failed to init spi\n");
        return 1;
    }

    if (!reset_sharc(sharc_index))
    {
        fprintf(stderr, "Failed to reset sharc\n");
        return 1;
    }

    printf("Writing initial firmware blob...");
    if (!load_file(spi_device_fd, firmware, INITIAL_FIRMWARE_LOAD_SIZE))
    {
        fprintf(stderr, "Failed to write initial firmware blob\n");
        return 1;
    }
    printf("done\n");


    printf("napping\n");
    usleep(10000);

    printf("Writing rest of firmware...");
    if (!load_file(spi_device_fd, firmware, 1000000000))
    {
        fprintf(stderr, "Failed to write rest of firmware blob\n");
        return 1;
    }
    printf("done\n");

    close(spi_device_fd);
    fclose(firmware);
    return 0;
}

// boilerplate to get access to GPIO line
static struct gpiod_line_request *
request_output_line(const char *chip_path, unsigned int offset,
                    enum gpiod_line_value initial_value, const char *consumer) {
#define gpio_req_err(msg) fprintf(stderr, "request_output_line: " msg "\n");
    struct gpiod_line_request* request = NULL;
	struct gpiod_chip* chip = gpiod_chip_open(chip_path);
	if (!chip)
    {
        gpio_req_err("failed to open chip");
		return NULL;
    }

	struct gpiod_line_settings* settings = gpiod_line_settings_new();
	if (!settings)
    {
        gpio_req_err("failed to create line settings");
		goto close_chip;
    }
	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, initial_value);
	struct gpiod_line_config* line_cfg = gpiod_line_config_new();
	if (!line_cfg)
    {
        gpio_req_err("failed to create line config");
		goto free_settings;
    }
	int ret = gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
	if (ret)
    {
        gpio_req_err("failed to add settings to line config");
		goto free_line_config;
    }
    struct gpiod_request_config* req_cfg = gpiod_request_config_new();
    if (!req_cfg)
    {
        gpio_req_err("failed to create request config");
        goto free_line_config;
    }
    gpiod_request_config_set_consumer(req_cfg, consumer);
	request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
	gpiod_request_config_free(req_cfg);

free_line_config:
	gpiod_line_config_free(line_cfg);
free_settings:
	gpiod_line_settings_free(settings);
close_chip:
	gpiod_chip_close(chip);

	return request;
}

static bool reset_sharc(int sharc_index)
{
    unsigned int reset_line_offset = sharc_reset_line_ids[sharc_index];
    struct gpiod_line_request* request = request_output_line(SHARC_RESET_CHIP, reset_line_offset, GPIOD_LINE_VALUE_ACTIVE, "sharc-reset");
    if (!request)
    {
        fprintf(stderr, "reset_sharc: failed to request line: %s\n", strerror(errno));
        return false;
    }

    gpiod_line_request_set_value(request, reset_line_offset, GPIOD_LINE_VALUE_INACTIVE);
    usleep(1000);
    gpiod_line_request_set_value(request, reset_line_offset, GPIOD_LINE_VALUE_ACTIVE);
    usleep(1000);

    gpiod_line_request_release(request);
    return true;
}

static int init_spi(char const* device_filename)
{
#define spi_err(msg) fprintf(stderr, "init_spi: " msg "\n");
	int ret = 0;

	int fd = open(device_filename, O_RDWR);
	if (fd < 0)
    {
        spi_err("couldn't open device file");
        return fd;
    }
        
    static unsigned char mode = MODE;
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0)
    {
		spi_err("can't set spi mode");
        return ret;
    }

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret < 0)
    {
		spi_err("can't get spi mode");
        return ret;
    }

    static unsigned char bits = BITS;
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0)
    {
		spi_err("can't set spi bits per word");
        return ret;
    }

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret < 0)
    {
		spi_err("can't get spi bits per word");
        return ret;
    }


    static unsigned int speed = SPI_SPEED;
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret < 0)
    {
		spi_err("can't set spi max speed");
        return ret;
    }

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret < 0)
    {
		spi_err("can't get spi max speed");
        return ret;
    }
    return fd;
}

#define BUF_SIZE_WORDS 1024

static bool load_file(int spi_device_fd, FILE *fp, int size)
{
	unsigned char buf[BUF_SIZE_WORDS * 4];
	uint8_t tx[BUF_SIZE_WORDS * 4];
	uint8_t rx[BUF_SIZE_WORDS * 4];
	int readed = 0;
	// int sent = 0;

	while(size > 0 && (readed = fread(buf, 4, min(BUF_SIZE_WORDS, size), fp)) > 0)
	{
        for(int i = 0; i < readed * 4; i++)
            tx[i] = reverseBitOrder(buf[i]);
        if(!transfer(spi_device_fd, tx, rx, readed * 4))
        {
            return false;
        }
		size -= readed;
	}
    return true;
}

static bool transfer(int spi_device_fd, uint8_t *tx, uint8_t *rx, unsigned long bufSize)
{
	int ret;
	struct spi_ioc_transfer transfer_ctrl = {
		.tx_buf = (unsigned long) tx,
		.rx_buf = (unsigned long) rx,
		.len = bufSize,
		.delay_usecs = DELAY,
		.speed_hz = SPI_SPEED,
		.bits_per_word = BITS,
	};

	ret = ioctl(spi_device_fd, SPI_IOC_MESSAGE(1), &transfer_ctrl);
	if (ret < 1)
    {
        fprintf(stderr, "failed to execute spi transfer\n");
		return false;
    }
    return true;
}

static unsigned char reverseBitOrder(unsigned char b)
{
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}
