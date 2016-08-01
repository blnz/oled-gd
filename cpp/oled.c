#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <sys/time.h>

#include "oled.h"
#include "logo.h"
#include "fontmap.h"

#define DEBUG

#ifdef DEBUG
#define dbg_printf printf
#else
void dbg_printf(void* a, ...) {};
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void write_byte(int fd, uint8_t tx);
void write_display(int fd, uint8_t *tx);
void fill_display(int fd, uint8_t val);

oled singleton;
int oled_initted = 0;


oledPtr oled_create()
{
  singleton.fd = oled_fd();
  return &singleton;
}

void oled_destroy(oledPtr oled)
{
  // nuttin' yet
}


/**
 *
 */ 
double get_sec_since_epoch()
{
  double sec;
  
  struct timeval tv;
  
  gettimeofday(&tv, NULL);
  sec = tv.tv_sec;
  sec += tv.tv_usec / 1000000.0;
  
  return sec;
}


static void pabort(const char *s)
{
  perror(s);
  //  abort();
}

// hard coded, board specific device & GPIOs

static const char *device = "/dev/spidev32766.2";

#define RST_GPIO	"0"
#define A0_GPIO		"1"

static const char *io_export = "/sys/class/gpio/export";
static const char *rst_val = "/sys/class/gpio/gpio0/value";
static const char *rst_dir = "/sys/class/gpio/gpio0/direction";
static const char *a0_val = "/sys/class/gpio/gpio1/value";
static const char *a0_dir = "/sys/class/gpio/gpio1/direction";

static uint8_t mode = 0;
static uint8_t bits = 0;
static uint32_t speed = 0;
static uint16_t delay = 0;

static int cached_fd = -1;


static void set_rst(uint8_t val)
{
  int fd = open(rst_val, O_RDWR);
  
  int wrv; 

  if (fd < 0) {
    pabort("can't open rst file");
  }
  if (val) {
    wrv = write(fd, "1", 1);
  } else {
    wrv = write(fd, "0", 1);
  }
  close(fd);
}

static void set_a0(uint8_t val)
{
  int fd = open(a0_val, O_RDWR);
  int wrv;

  if (fd < 0) {
    pabort("can't open a0 file");
  }
  if (val) {
    wrv = write(fd, "1", 1);
  } else {
    wrv = write(fd, "0", 1);
  }
  close(fd);
}

static void set_page(int fd, int page)
{
  set_a0(0);
  write_byte(fd, 0xb0 + page);
  write_byte(fd, 0x02);
  write_byte(fd, 0x10);
  set_a0(1);
} 

static void request_gpio()
{
  int export_fd;
  int rst_dir_fd;
  int a0_dir_fd;
  
  int wrv = 0;

  /* Export RST Gpio */
  export_fd = open(io_export, O_WRONLY);
  if (export_fd < 0)
    pabort("can't open export file");
  wrv = write(export_fd, RST_GPIO, 1);
  close(export_fd);
  
  /* Export A0 Gpio */
  export_fd = open(io_export, O_WRONLY);
  if (export_fd < 0)
    pabort("can't open export file");
  wrv = write(export_fd, A0_GPIO, 1);
  close(export_fd);
  
  /* Set RST to output */
  rst_dir_fd = open(rst_dir, O_RDWR);
  if (rst_dir_fd < 0)
    pabort("can't open reset dir file");
  wrv = write(rst_dir_fd, "out", 3);
  close(rst_dir_fd);
  
  /* Set A0 to output */
  a0_dir_fd = open(a0_dir, O_RDWR);
  if (a0_dir_fd < 0)
    pabort("can't open a0 dir file");
  wrv = write(a0_dir_fd, "out", 3);
  close(a0_dir_fd);
}


static void initialize_oled(int fd)
{
  
  // struct timeval tstart;
  // gettimeofday(&tstart, NULL);
  
  if (!oled_initted) {
    oled_initted = 1;
    set_rst(0);
    usleep(5);
    set_rst(1);
    usleep(5);
    set_a0(0);
    
    write_byte(fd, 0xAE);	// Display Off
    write_byte(fd, 0xD5);	// Clock Divide Ratio
    write_byte(fd, 0x80);	//    Divide = 0, Fosc = 16
    write_byte(fd, 0xA8);	// Mux Ratio
    write_byte(fd, 0x3F);	//    63
    write_byte(fd, 0xD3);	// Display offset (Vertical shift)
    write_byte(fd, 0x00);	//    0
    write_byte(fd, 0x40);	// Display Line Start 0
    write_byte(fd, 0xAD);	// DC-DC Control Mode Set
    write_byte(fd, 0x8B);	// DC-DC On/Off Mode
    write_byte(fd, 0x32);	// Set Pump Voltage
    write_byte(fd, 0xA1);	// Segment Remap (Col 127 is mapped to Seg 0)
    write_byte(fd, 0xC8);	// COM Scan Direction (Remapped mode)
    write_byte(fd, 0xDA);	// COM Pin configuration
    write_byte(fd, 0x12);	//    Alternative conf, left/right remap disabled
    write_byte(fd, 0x81);	// Contrast
    write_byte(fd, 0x50);	//    0x50 (From Datasheet)
    write_byte(fd, 0xD9);	// Precharge Period
    write_byte(fd, 0x1F);	//    Phase 1 - 15DCLK, Phase 2 - 1 DCLK
    write_byte(fd, 0xDB);	//  VCOMH Deselect Level
    write_byte(fd, 0x40);	//    4 (undefined in datasheet)
    write_byte(fd, 0xA4);	// Output follows RAM
    write_byte(fd, 0xA6);	// Normal Display
    write_byte(fd, 0xAF);	// Display on
  }

}

void write_byte(int fd, uint8_t tx)
{
  int ret;
  
  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long) &tx,
    .rx_buf = (unsigned long) NULL,
    .len = 1,
    .delay_usecs = delay,
  };
  
  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1)
    pabort("can't send spi message");
  
}

/*
 * tx represents a bitmap, in the Visionox OLED's arrangement, where each byte represents
 * a vertical column of pixels, 8 high
 */
void write_display(int fd, uint8_t *tx)
{
  
  struct timeval tstart;
  gettimeofday(&tstart, NULL);
  
  int i,j;
  for (j = 0; j < OLED_PAGES; j++) {
    set_page(fd, j);
    for (i = 0; i < OLED_WIDTH; i++) {
      write_byte(fd, tx[i+j * OLED_WIDTH]);
    }
  }
  
  struct timeval tend;
  gettimeofday(&tend, NULL);
  
}


void fill_display(int fd, uint8_t val)
{
  int i, j;
  for (j = 0; j < OLED_PAGES; j++) {
    set_page(fd, j);
    for (i = 0; i < OLED_WIDTH; i++) {
      write_byte(fd,val);
    }
  }
}
#define blank(fd) fill_display(fd,0x00)
#define white(fd) fill_display(fd,0xFF)

int oled_fd()
{
  
  struct timeval tstart;
  gettimeofday(&tstart, NULL);
  
  int fd;
  if (cached_fd > 0) {
    fd = cached_fd;
  } else {
    fd = open(device, O_RDWR);
    if (fd < 0)
      pabort("can't open device");
    
    cached_fd = fd;
    
    int ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
      pabort("can't set spi mode");
    
    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1)
      pabort("can't get spi mode");
    
    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
      pabort("can't get bits per word");
    
    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
      pabort("can't get max speed hz");
    
    request_gpio();
    initialize_oled(fd);
  }
  
  return fd;
}

// display a bitmap
int oled_demo()
{
  int fd;

  fd = oled_fd();
  write_display(fd,(uint8_t *)logo);
  
  return 0;
}


uint8_t display_bitmap[DISPLAY_LEN];
uint8_t oled_bitmap[DISPLAY_LEN];

void clear_display() {
  
  int dl = DISPLAY_LEN;
  int i;
  for (i = 0; i < dl; ++i) {
    display_bitmap[i] = 0;
  }
}


void oledClearDisplay(oledPtr op) {
  
  int dl = DISPLAY_LEN;
  uint8_t* dbm = op->display_bitmap; 
  int i;
  for (i = 0; i < dl; ++i) {
    dbm[i] = 0;
  }
}

int blitchar(int charcode, int xoff, int yoff)
{
  
  if ((charcode < 0) || (charcode > FONTMAP_MAX)) {
    charcode = 0;
  }
  
  chardef theChar = fontmap[charcode];
  uint8_t *cPixels = &theChar.pixels[0];
  int rowBytes = theChar.bytes_cnt / 16;
  
  if (xoff <= (OLED_WIDTH - (rowBytes * 8))) {
    int i;
    for (i = 0; i < 16; ++i) {

      int start = ((yoff + i) * (OLED_WIDTH / 8)) + floor(xoff / 8);

      int ds = start;
      int ss = i * rowBytes;
      int j;
      for (j = 0; j < rowBytes; j++) {
        display_bitmap[ds + j] = cPixels[ss + j];
      }
    }
  }
  return rowBytes * 8;
}

int oledBlitchar(oledPtr op, int charcode, int xoff, int yoff)
{
  
  if ((charcode < 0) || (charcode > FONTMAP_MAX)) {
    charcode = 0;
  }
  
  chardef theChar = fontmap[charcode];
  uint8_t *cPixels = &theChar.pixels[0];
  int rowBytes = theChar.bytes_cnt / 16;
  
  if (xoff <= (OLED_WIDTH - (rowBytes * 8))) {
    int i;
    for (i = 0; i < 16; ++i) {

      int start = ((yoff + i) * (OLED_WIDTH / 8)) + floor(xoff / 8);

      int ds = start;
      int ss = i * rowBytes;
      int j;
      for (j = 0; j < rowBytes; j++) {
        op->display_bitmap[ds + j] = cPixels[ss + j];
      }
    }
  }
  return rowBytes * 8;
}

void invert_display (int xoff, int yoff, int width, int height) 
{
  int rowBytes = (width / 8);
  int i;
  for (i = 0; i < height; ++i) {
    int start = ((yoff + i) * (OLED_WIDTH / 8)) + floor(xoff / 8);

    uint8_t* dest = &display_bitmap[start];
    int j;
    for (j = 0; j < rowBytes; j++) {
      dest[j] ^= 0xff;
    }
  }
}

void oledInvertDisplay (oledPtr op, int xoff, int yoff, int width, int height) 
{
  int rowBytes = (width / 8);
  int i;
  for (i = 0; i < height; ++i) {
    int start = ((yoff + i) * (OLED_WIDTH / 8)) + floor(xoff / 8);

    uint8_t* dest = &(op->display_bitmap[start]);
    int j;
    for (j = 0; j < rowBytes; j++) {
      dest[j] ^= 0xff;
    }
  }
}

// shift bits around from a "standard" bitmap to the arrangement of bits & bytes
//  favored by our Visionox OLED
static void rotate_bytes(uint8_t *src, uint8_t *dest) 
{
  int lcd_bytes = DISPLAY_LEN;
  int i;
  for (i = 0; i < lcd_bytes; ++i) {
    dest[i] = 0;
  }
  int row, col;
  for (row = 0; row < OLED_HEIGHT; ++row) {
    for (col = 0; col < OLED_WIDTH; ++col) {
      int srcByte = (row * (OLED_WIDTH / 8)) + (col / 8);
      int srcBit = col % 8;

      int destByte = ((row / 8) * OLED_WIDTH) + col;
      int destBit = row % 8;

      int destBitMask = 1 << destBit;

      int srcBitMask = 128 >> srcBit;  
      if (src[srcByte] & srcBitMask) {
	dest[destByte] |= destBitMask;
      }
      
    }
  }
}


void render_display(int fd)
{
  rotate_bytes(display_bitmap, oled_bitmap);
  write_display(fd, oled_bitmap);
}


void oledRenderDisplay(oledPtr op)
{
  rotate_bytes(op->display_bitmap, op->oled_bitmap);
  write_display(op->fd, op->oled_bitmap);
}


int blitString (const char* theStr, int xoff, int yoff) 
{
    int nextx = xoff;
    while (*theStr > 0) {
        nextx += blitchar(*theStr++, nextx, yoff);
    }
    return nextx;
}


int oledBlitString (oledPtr op, const char* theStr, int xoff, int yoff) 
{
    int nextx = xoff;
    while (*theStr > 0) {
      nextx += oledBlitchar(op, *theStr++, nextx, yoff);
    }
    return nextx;
}
