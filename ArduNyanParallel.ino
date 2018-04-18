// PLEASE DO NOT SUE ME MR. NYAN CAT MAN.

#include <SPI.h>
#include "kitty.h"

#define WIDTH 128 /**< The width of the display in pixels */
#define HEIGHT 64 /**< The height of the display in pixels */
#define BLACK 0  /**< Color value for an unlit pixel for draw functions. */
#define WHITE 1  /**< Color value for a lit pixel for draw functions. */

// Ticker stuff leftover from the start. Still used, but really shouldn't be.
#define frame(i) ((((i))/8)%12)

uint8_t sBuffer[1024];

const uint8_t bitMask[8] = {1, 2, 4, 8, 16, 32, 64, 128};

// Sparkle starting positions.
int16_t sparklesX[] = { 4, 33, 43,  63, 31, 2,
                        64+4, 64+33, 64+43,  64+63, 64+31, 64+2};
char sparklesY[] = {1,  10, 20, 41, 51, 61, 1,  10, 20, 41, 51, 61};
// Starting frames for each sparkle.
char sparklesF[] = {1, 3, 1, 5, 5, 2, 1, 3, 1, 5, 5, 2};

// start the cat centered in the screen
char catX = 47;
char catY = 20;

void setup()
{
  DDRC |=   B00000111; // SER, SRCLK, RCLK = OUTPUT
  PORTC &= ~B00000111; // SER, SRCLK, RCLK = LOW
  DDRB |=   B00101010; // SPI SCK (WR), SPI MOSI (OE), DC = OUTPUT
  PORTB &= ~B00101010; // SPI SCK (WR), SPI MOSI (OE), DC = LOW

  bootLCD();

  PORTB |=  B00001000; // OE HIGH (all SN74AHC595 outputs pulled-down to zero)
  PORTB |=  B00000010; // DC HIGH

  //Clear Screen
  for (uint32_t p = 0; p < 153600; p++) // 320x480 = 153,600
  {
    PORTB &= ~B00100000; // WR LOW
    PORTB |=  B00100000; // WR HIGH
  }

  PORTB &= ~B00001000; // OE LOW

  writeCommand(0x2A); // setAddrWindow 128x64
  writeData(0x00); // 128
  writeData(0x80);
  writeData(0x00); // 191
  writeData(0xBF);
  writeCommand(0x2B);
  writeData(0x00); // 176
  writeData(0xB0);
  writeData(0x01); // 303
  writeData(0x2F);
  writeCommand(0x2C);

  SPI.begin();
  SPCR |=  B00100000; // DORD
  SPCR &= ~B00000011; // SPR1 and SPR0

  PORTB |=  B00000010; // DC HIGH
}

void GoKittyGo ()
{
  static uint16_t i = 0;

  // draw sparkles
  for(char j=0;j<12;j++)
  {
    sparklesX[j] -= 1;/*-8*frame(i)));*/
    if (sparklesX[j] < -8)
      sparklesX[j] += 128;
    drawBitmap(sparklesX[j],sparklesY[j]-3,sparkle + 8*((
      (frame(i)/2) + sparklesF[j]
    )%6),8,8,WHITE);
  }

  drawBitmap(catX,catY,nyan_png_data + (34*3)*(frame(i)/2),34,24,WHITE);
  // draw cat
  drawBitmap(catX,catY,nyan2_png_data + (34*3)*(frame(i)/2),34,24,BLACK);

  // push to screen
  display();

  // increase ticker
  i++;
  if (i == 192*10)
    i = 0;
}

void loop()
{
  setLCDColour(0xF8, 0x00); // Red

  for (uint8_t q = 0; q < 255; q++)
  {
    GoKittyGo();
  }

  setLCDColour(0X00, 0x1F); // Blue

  for (uint8_t q = 0; q < 255; q++)
  {
    GoKittyGo();
  }
}

void setLCDColour(uint8_t colourHI, uint8_t colourLO)
{
  PORTC &= ~B00000001; // RCLK LOW

  for (uint8_t b = 0; b < 8; b++)
  {
    PORTC &= ~B00000010; // SRCLK LOW

    if (colourHI & bitMask[b]) PORTC |=  B00000100; // SER
    else                       PORTC &= ~B00000100;

    PORTC |=  B00000010; // SRCLK HIGH
  }

  for (uint8_t b = 0; b < 8; b++)
  {
    PORTC &= ~B00000010; // SRCLK LOW

    if (colourLO & bitMask[b]) PORTC |=  B00000100; // SER
    else                       PORTC &= ~B00000100;

    PORTC |=  B00000010; // SRCLK HIGH
  }

  PORTC |=  B00000001; // RCLK HIGH
}

void paintScreen(uint8_t image[])
{
  uint16_t i = 0;

  for (uint8_t column = 0; column < WIDTH; column++)
  {
    for (uint8_t page = 0; page < 8; page++)
    {
      SPI.transfer(image[i]);

      // clear the byte in the image buffer
      image[i] = BLACK;

      i = i + WIDTH; // increment to next byte down vertically
    }
    i = i - 1023; // return to beginning of the next column
  }
}

void display()
{
  paintScreen(sBuffer);
}

void drawBitmap
(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h,
 uint8_t color)
{
  // no need to draw at all if we're offscreen
  if (x+w < 0 || x > WIDTH-1 || y+h < 0 || y > HEIGHT-1)
    return;

  int yOffset = abs(y) % 8;
  int sRow = y / 8;
  if (y < 0) {
    sRow--;
    yOffset = 8 - yOffset;
  }
  int rows = h/8;
  if (h%8!=0) rows++;
  for (int a = 0; a < rows; a++) {
    int bRow = sRow + a;
    if (bRow > (HEIGHT/8)-1) break;
    if (bRow > -2) {
      for (int iCol = 0; iCol<w; iCol++) {
        if (iCol + x > (WIDTH-1)) break;
        if (iCol + x >= 0) {
          if (bRow >= 0) {
            if (color == WHITE)
              sBuffer[(bRow*WIDTH) + x + iCol] |= pgm_read_byte(bitmap+(a*w)+iCol) << yOffset;
            else if (color == BLACK)
              sBuffer[(bRow*WIDTH) + x + iCol] &= ~(pgm_read_byte(bitmap+(a*w)+iCol) << yOffset);
            else
              sBuffer[(bRow*WIDTH) + x + iCol] ^= pgm_read_byte(bitmap+(a*w)+iCol) << yOffset;
          }
          if (yOffset && bRow<(HEIGHT/8)-1 && bRow > -2) {
            if (color == WHITE)
              sBuffer[((bRow+1)*WIDTH) + x + iCol] |= pgm_read_byte(bitmap+(a*w)+iCol) >> (8-yOffset);
            else if (color == BLACK)
              sBuffer[((bRow+1)*WIDTH) + x + iCol] &= ~(pgm_read_byte(bitmap+(a*w)+iCol) >> (8-yOffset));
            else
              sBuffer[((bRow+1)*WIDTH) + x + iCol] ^= pgm_read_byte(bitmap+(a*w)+iCol) >> (8-yOffset);
          }
        }
      }
    }
  }
}

//=============================================================
//WRITE COMMAND
void writeCommand(uint8_t c)
{
    PORTB &= ~B00000010; // DC LOW

    PORTC &= ~B00000001; // RCLK LOW

    for (uint8_t b = 0; b < 8; b++)
    {
      PORTC &= ~B00000010; // SRCLK LOW

      if (c & bitMask[b]) PORTC |=  B00000100; // SER
      else                PORTC &= ~B00000100;

      PORTC |=  B00000010; // SRCLK HIGH
    }

    PORTC |=  B00000001; // RCLK HIGH

    PORTB &= ~B00100000; // WR LOW
    PORTB |=  B00100000; // WR HIGH
}

//===============================================================
//WRITE COMMAND DATA
void writeData(uint8_t d)
{
    PORTB |=  B00000010; // DC HIGH

    PORTC &= ~B00000001; // RCLK LOW

    for (uint8_t b = 0; b < 8; b++)
    {
      PORTC &= ~B00000010; // SRCLK LOW

      if (d & bitMask[b]) PORTC |=  B00000100; // SER
      else                PORTC &= ~B00000100;

      PORTC |=  B00000010; // SRCLK HIGH
    }

    PORTC |=  B00000001; // RCLK HIGH

    PORTB &= ~B00100000; // WR LOW
    PORTB |=  B00100000; // WR HIGH
}

// HX8357-C display initialisation
// https://github.com/Bodmer/TFT_HX8357
void bootLCD()
{
    writeCommand(0xB9); // Enable extension command
    writeData(0xFF);
    writeData(0x83);
    writeData(0x57);
    delay(50);

    writeCommand(0xB6); //Set VCOM voltage
    writeData(0x2C);    //0x52 for HSD 3.0"

    writeCommand(0x11); // Sleep off
    delay(200);

    writeCommand(0x35); // Tearing effect on
    writeData(0x00);    // Added parameter

    writeCommand(0x3A); // Interface pixel format
    writeData(0x55);    // 16 bits per pixel

    writeCommand(0xB1); // Power control
    writeData(0x00);
    writeData(0x15);
    writeData(0x0D);
    writeData(0x0D);
    writeData(0x83);
    writeData(0x48);

    writeCommand(0xC0); // Does this do anything?
    writeData(0x24);
    writeData(0x24);
    writeData(0x01);
    writeData(0x3C);
    writeData(0xC8);
    writeData(0x08);

    writeCommand(0xB4); // Display cycle
    writeData(0x02);
    writeData(0x40);
    writeData(0x00);
    writeData(0x2A);
    writeData(0x2A);
    writeData(0x0D);
    writeData(0x4F);

    writeCommand(0xE0); // Gamma curve
    writeData(0x00);
    writeData(0x15);
    writeData(0x1D);
    writeData(0x2A);
    writeData(0x31);
    writeData(0x42);
    writeData(0x4C);
    writeData(0x53);
    writeData(0x45);
    writeData(0x40);
    writeData(0x3B);
    writeData(0x32);
    writeData(0x2E);
    writeData(0x28);

    writeData(0x24);
    writeData(0x03);
    writeData(0x00);
    writeData(0x15);
    writeData(0x1D);
    writeData(0x2A);
    writeData(0x31);
    writeData(0x42);
    writeData(0x4C);
    writeData(0x53);
    writeData(0x45);
    writeData(0x40);
    writeData(0x3B);
    writeData(0x32);

    writeData(0x2E);
    writeData(0x28);
    writeData(0x24);
    writeData(0x03);
    writeData(0x00);
    writeData(0x01);

    writeCommand(0x36); // MADCTL Memory access control
    writeData(0x08);
    delay(20);

    writeCommand(0x21); //Display inversion on
    delay(20);

    writeCommand(0x29); // Display on

    delay(120);

    writeCommand(0x2A); // setAddrWindow
    writeData(0x00);
    writeData(0x00);
    writeData(0x01); // 319
    writeData(0x3F);
    writeCommand(0x2B);
    writeData(0x00);
    writeData(0x00);
    writeData(0x01); // 479
    writeData(0xDF);
    writeCommand(0x2C);
}
