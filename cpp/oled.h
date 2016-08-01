#ifdef __cplusplus
extern "C" {
#endif

#ifndef OLED_H
#define OLED_H 1

#define OLED_MAJOR_VERSION 2
#define OLED_MINOR_VERSION 1
#define OLED_RELEASE_VERSION 0
#define OLED_EXTRA_VERSION "dev"
#define OLED_VERSION_STRING "2.1.1-dev"


#define LCD_WIDTH	128
#define LCD_HEIGHT	64
#define LCD_PAGES	LCD_HEIGHT/8


#define OLED_WIDTH	128
#define OLED_HEIGHT	64
#define OLED_PAGES	OLED_HEIGHT/8

#define DISPLAY_LEN (OLED_WIDTH*OLED_HEIGHT)/8


#ifdef __cplusplus
  extern "C"
  {
#endif

    /* stdio is needed for file I/O. */
#include <stdio.h>
#include <stdarg.h>


    typedef struct oledStruct {
      /* Palette-based image pixels */
      int fd;

      uint8_t display_bitmap[DISPLAY_LEN];
      uint8_t oled_bitmap[DISPLAY_LEN];

      int sx;
      int sy;
      /* These are valid in palette images only. See also
         'alpha', which appears later in the structure to
         preserve binary backwards compatibility */
    }
      oled;

    typedef oled *oledPtr;

    oledPtr oled_create();
    void oled_destroy(oledPtr oled);

    int oled_fd();
    int oled_demo();

    void fill_display(int fd, uint8_t val);
    void oledFillDisplay(oledPtr op, uint8_t val);
    void render_display(int fd);
    void oledRenderDisplay(oledPtr op);
    void clear_display();
    void oledClearDisplay(oledPtr op);

    void invert_display (int xoff, int yoff, int width, int height);
    void oledInvertDisplay (oledPtr op, int xoff, int yoff, int width, int height);
    int blitString (const char* theStr, int xoff, int yoff);
    int oledBlitString (oledPtr op, const char* theStr, int xoff, int yoff);

#ifdef __cplusplus
  }
#endif

#endif				/* OLED_H */

#ifdef __cplusplus
}
#endif
