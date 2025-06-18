/*
 * ssd1306.c
 */

#include <msp430.h>
#include <stdint.h>
#include <stdlib.h>


#include <lib/i2c/i2c.h>
#include <lib/oled/ssd1306.h>
#include <lib/oled/font_5x7.h>
#include <lib/oled/font12x16.h>



unsigned char *dataBuffer;

/* ====================================================================
 * Horizontal Centering Number Array
 * ==================================================================== */
const unsigned char HcenterUL[] = {                                           // Horizontally center number with separators on screen
                               0,                                       // 0 digits, not used but included to size array correctly
                               61,                                      // 1 digit
                               58,                                      // 2 digits
                               55,                                      // 3 digits
                               49,                                      // 4 digits and 1 separator
                               46,                                      // 5 digits and 1 separator
                               43,                                      // 6 digits and 1 separator
                               37,                                      // 7 digits and 2 separators
                               34,                                      // 8 digits and 2 separators
                               31,                                      // 9 digits and 2 separators
                               25                                       // 10 digits and 3 separators
};

// Image Buffer prefilled with all black pixels
const unsigned char blackFillBlockBuffer[] = {
    SSD1306_SETSTARTLINE,
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
};

// Image Buffer prefilled with all white pixels
const unsigned char whiteFillBlockBuffer[] = {
    SSD1306_SETSTARTLINE,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE
};

void ssd1306_init(void) {
    // SSD1306 init sequence
    ssd1306_command(SSD1306_DISPLAYOFF);                                // 0xAE
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);                        // 0xD5
    ssd1306_command(0x80);                                              // the suggested ratio 0x80

    ssd1306_command(SSD1306_SETMULTIPLEX);                              // 0xA8
    ssd1306_command(SSD1306_LCDHEIGHT - 1);

    ssd1306_command(SSD1306_SETDISPLAYOFFSET);                          // 0xD3
    ssd1306_command(0x0);                                               // no offset
    ssd1306_command(SSD1306_SETSTARTLINE | 0x0);                        // line #0
    ssd1306_command(SSD1306_CHARGEPUMP);                                // 0x8D
    ssd1306_command(0x14);                                              // generate high voltage from 3.3v line internally
    ssd1306_command(SSD1306_MEMORYMODE);                                // 0x20
    ssd1306_command(0x00);                                              // 0x0 act like ks0108
    ssd1306_command(SSD1306_SEGREMAP | 0x1);
    ssd1306_command(SSD1306_COMSCANDEC);

    ssd1306_command(SSD1306_SETCOMPINS);                                // 0xDA
    ssd1306_command(0x12);
    ssd1306_command(SSD1306_SETCONTRAST);                               // 0x81
    ssd1306_command(0xCF);

    ssd1306_command(SSD1306_SETPRECHARGE);                              // 0xd9
    ssd1306_command(0xF1);
    ssd1306_command(SSD1306_SETVCOMDETECT);                             // 0xDB
    ssd1306_command(0x40);
    ssd1306_command(SSD1306_DISPLAYALLON_RESUME);                       // 0xA4
    ssd1306_command(SSD1306_NORMALDISPLAY);                             // 0xA6

    ssd1306_command(SSD1306_DEACTIVATE_SCROLL);

    ssd1306_command(SSD1306_DISPLAYON);                                 //--turn on oled panel
} // end ssd1306_init

void ssd1306_command(unsigned char command) {
    buffer[0] = 0x80;
    buffer[1] = command;

    i2c_write(SSD1306_I2C_ADDRESS, buffer, 2);
} // end ssd1306_command


void ssd1306_clearDisplay(void)
{
    ssd1306_setPosition(0, 0);

    uint8_t i;
    for (i = 32; i > 0; i--)    // The screen is made up of 32 blocks of such 8x32 pixels
    {
        i2c_write(SSD1306_I2C_ADDRESS, (unsigned char*)blackFillBlockBuffer, SSD1306_MAX_DATA_BUFFER_SIZE);
    }
} // end ssd1306_clearDisplay




// clear a rectangular area of w X h blocks, where the starting location given by
// columnX - which runs from 0 to 96 ( because the block is 32 pixel-columns wide )
// page - runs from 0 to 7 as Y start points of the block because the block height is 8px
// w - is the number of ( 32 pixel columns wide and 8px down ) block in x direction
// h - is the number of ( 32 pixel columns wide and 8px down ) block in y direction
void ssd1306_clearPageBlockArea(unsigned char columnX, unsigned char page, unsigned char w, unsigned char h)
{
    unsigned char iw, ih;
    for (ih = 0; ih < h; ih++)
        for (iw = 0; iw < w; iw++)
        {
            ssd1306_clearPageBlock(columnX + iw*(SSD1306_MAX_DATA_BUFFER_COLUMNS), page + ih);
        }
}


// clear by blocks of fixed size ( 32 pixel columns wide and 8px down ) with starting location given by
// columnX - which runs from 0 to 96 ( because the block is 32 pixel-columns wide )
// page - runs from 0 to 7 as Y start points of the block because the block height is 8px
void ssd1306_clearPageBlock(unsigned char columnX, unsigned char page)
{
    if (columnX > 96) columnX = 96; // limit columnX to it's upper bound
    if (page > 7) page = 7;         // limit page to it's upper bound

    ssd1306_setPosition(columnX, page);

    i2c_write(SSD1306_I2C_ADDRESS, (unsigned char*)blackFillBlockBuffer, SSD1306_MAX_DATA_BUFFER_SIZE);
}




// A proper precise clear rect function has not been achieved yet. this is merely a copy
// of the clear page block function above. precise implementation is yet to come.
void ssd1306_clearRect(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    ssd1306_setPosition(x, y);
    i2c_write(SSD1306_I2C_ADDRESS, (unsigned char*)whiteFillBlockBuffer, SSD1306_MAX_DATA_BUFFER_SIZE);
}


void ssd1306_setPosition(unsigned char column, unsigned char page) {
    if (column > 128) {
        column = 0;                                                     // constrain column to upper limit
    }

    if (page > 8) {
        page = 0;                                                       // constrain page to upper limit
    }

    ssd1306_command(SSD1306_COLUMNADDR);
    ssd1306_command(column);                                            // Column start address (0 = reset)
    ssd1306_command(SSD1306_LCDWIDTH-1);                                // Column end address (127 = reset)

    ssd1306_command(SSD1306_PAGEADDR);
    ssd1306_command(page);                                              // Page start address (0 = reset)
    ssd1306_command(7);                                                 // Page end address
} // end ssd1306_setPosition



void ssd1306_printText(uint8_t x, uint8_t y, char *ptString) {
    ssd1306_setPosition(x, y);

    while (*ptString != '\0') {
        buffer[0] = 0x40;

        if ((x + 5) >= 127) {                                           // char will run off screen
            x = 0;                                                      // set column to 0
            y++;                                                        // jump to next page
            ssd1306_setPosition(x, y);                                  // send position change to oled
        }

        uint8_t i;
        for(i = 0; i< 5; i++) {
            buffer[i+1] = font_5x7[*ptString - ' '][i];
        }

        buffer[6] = 0x0;

        i2c_write(SSD1306_I2C_ADDRESS, buffer, 7);
        ptString++;
        x+=6;
    }
} // end ssd1306_printText

void ssd1306_printTextBlock(uint8_t x, uint8_t y, char *ptString) {
    char word[12];
    uint8_t i;
    uint8_t endX = x;
    while (*ptString != '\0'){
        i = 0;
        while ((*ptString != ' ') && (*ptString != '\0')) {
            word[i] = *ptString;
            ptString++;
            i++;
            endX += 6;
        }

        word[i++] = '\0';

        if (endX >= 127) {
            x = 0;
            y++;
            ssd1306_printText(x, y, word);
            endX = (i * 6);
            x = endX;
        } else {
            ssd1306_printText(x, y, word);
            endX += 6;
            x = endX;
        }
        ptString++;
    }

}

/*
void ssd1306_printUI32( uint8_t x, uint8_t y, uint32_t val, uint8_t Hcenter ) {
    char text[14];

    ultoa(val, text);
    if (Hcenter) {
        ssd1306_printText(HcenterUL[digits(val)], y, text);
    } else {
        ssd1306_printText(x, y, text);
    }
} // end ssd1306_printUI32

uint8_t digits(uint32_t n) {
    if (n < 10) {
        return 1;
    } else if (n < 100) {
        return 2;
    } else if (n < 1000) {
        return 3;
    } else if (n < 10000) {
        return 4;
    } else if (n < 100000) {
        return 5;
    } else if (n < 1000000) {
        return 6;
    } else if (n < 10000000) {
        return 7;
    } else if (n < 100000000) {
        return 8;
    } else if (n < 1000000000) {
        return 9;
    } else {
        return 10;
    }
} // end digits

void ultoa(uint32_t val, char *string) {
    uint8_t i = 0;
    uint8_t j = 0;
                                                                        // use do loop to convert val to string
    do {
        if (j==3) {                                                     // we have reached a separator position
            string[i++] = ',';                                          // add a separator to the number string
            j=0;                                                        // reset separator indexer thingy
        }
            string[i++] = val%10 + '0';                                 // add the ith digit to the number string
            j++;                                                        // increment counter to keep track of separator placement
    } while ((val/=10) > 0);

    string[i++] = '\0';                                                 // add termination to string
    reverse(string);                                                    // string was built in reverse, fix that
} // end ultoa
*/

void reverse(char *s)
{
    uint8_t i, j;
    uint8_t c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
} // end reverse

void drawImage(unsigned char x, unsigned char y, unsigned char sx,
                       unsigned char sy, const unsigned char img[],
                       unsigned char invert) {
  unsigned int j, t;
  unsigned char i, p, p0, p1, n, n1, b;

  if (((x + sx) > SSD1306_LCDWIDTH) || ((y + sy) > SSD1306_LCDHEIGHT) ||
      (sx == 0) || (sy == 0)) return;

  // Total bytes of the image array
  if (sy % 8)
    t = (sy / 8 + 1) * sx;
  else
    t = (sy / 8) * sx;
  p0 = y / 8;                 // first page index
  p1 = (y + sy - 1) / 8;      // last page index
  n = y % 8;                  // offset from begin of page

  n1 = (y + sy) % 8;
  if (n1) n1 = 8 - n1;

  j = 0;                      // bytes counter [0..t], or [0..(t+sx)]
  dataBuffer = malloc(sx + 1);       // allocate memory for the buf
  dataBuffer[0] = SSD1306_SETSTARTLINE; // fist item "send data mode"
  for (p=p0; p<(p1+1); p++) {
      ssd1306_setPosition(x, p);
    for (i=x; i<(x+sx); i++) {
      if (p == p0) {
        b = (img[j] << n) & 0xFF;
      } else if ((p == p1) && (j >= t)) {
        b = (img[j - sx] >> n1) & 0xFF;
      } else {
        b = ((img[j - sx] >> (8 - n)) & 0xFF) | ((img[j] << n) & 0xFF);
      };
      if (invert)
          dataBuffer[i - x + 1] = b;
      else
          dataBuffer[i - x + 1] = ~b;
      j++;
    }
   // sendData(dataBuffer, sx + 1); // send the buf to display
    i2c_write(SSD1306_I2C_ADDRESS, dataBuffer, sx + 1);
  }
  free(dataBuffer);
}


void draw12x16Str(unsigned char x, unsigned char y, const char str[],
                          unsigned char invert) {
  unsigned char i;
  unsigned int c;

  i = 0;
  while (str[i] != '\0') {
    if ((unsigned char)str[i] > 191)
      c = (str[i] - 64) * FONT12X16_WIDTH * 2;// why is this line here
    else
      c = str[i] * FONT12X16_WIDTH * 2;
    drawImage(x, y, 12, 16, (unsigned char *) &font12x16[c], invert);
    i++;
    x += 12;
  };
}

void draw5x7Str(unsigned char x, unsigned char y, const char str[],
                          unsigned char invert) {
  unsigned char i;
  unsigned int c;

  i = 0;
  while (str[i] != '\0') {
    c = str[i] - 32;
    drawImage(x, y, FONT5X7_WIDTH, FONT5X7_HEIGHT+1, (unsigned char *) &font_5x7[c], invert);
    i++;
    x += FONT5X7_WIDTH+2;
  };
}




