// PLEASE DO NOT SUE ME MR. NYAN CAT MAN.

#include "kitty.h"
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#define WIDTH 128 /**< The width of the display in pixels */
#define HEIGHT 64 /**< The height of the display in pixels */
#define BLACK 0  /**< Color value for an unlit pixel for draw functions. */
#define WHITE 1  /**< Color value for a lit pixel for draw functions. */

// Ticker stuff leftover from the start. Still used, but really shouldn't be.
#define frame(i) ((((i))/8)%12)

#define TFT_CS     1
#define TFT_RST   -1  // you can also connect this to the Arduino reset
                      // in which case, set this #define pin to -1!
#define TFT_DC     4

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

uint8_t sBuffer[1024];

uint8_t coloursArrayIndex;
uint16_t coloursArray[6] =
{
  0xF800, // Red
  0X07E0, // Green
  0XFFE0, // Red + Green = Yellow
  0X001F, // Blue
  0XF81F, // Red + Blue = Pink
  0X07FF  // Green + Blue = Cyan
};

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
  tft.initR(INITR_144GREENTAB); // initialize a ST7735S chip, black tab

  tft.fillScreen(ST7735_BLACK);

  tft.setAddrWindow(0, 0, WIDTH-1, HEIGHT-1);
}

void GoKittyGo ()
{
  static uint16_t i = 0;

  // clear the screen
  clear();

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
  display(coloursArray[coloursArrayIndex]);

  // increase ticker
  i++;
  if (i == 192*10)
    i = 0;
}

void loop()
{
  for (coloursArrayIndex = 0; coloursArrayIndex < 6; coloursArrayIndex++)
  {
    GoKittyGo();
  }
}

void paintScreen(uint8_t image[], uint16_t colour)
{
  static uint16_t tempColour;

  for (uint8_t t = 0; t < 8; t++)
  {
    for (uint8_t r = 0; r < 8; r++)
    {
      uint16_t a = t * 128;

      uint8_t bitMask = B00000001 << r;

      for (uint8_t i = 0; i < 128; i++)
      {  
        if (image[a] & bitMask) tempColour = colour;
        else                    tempColour = BLACK;

        a++;

        tft.pushColor(tempColour);
      }
    }
  }
}

void display(uint16_t colour)
{
  paintScreen(sBuffer, colour);
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

void fillScreen(uint8_t color)
{
  // C version:
  //
  // if (color != BLACK)
  // {
  //   color = 0xFF; // all pixels on
  // }
  // for (int16_t i = 0; i < WIDTH * HEIGTH / 8; i++)
  // {
  //    sBuffer[i] = color;
  // }

  // This asm version is hard coded for 1024 bytes. It doesn't use the defined
  // WIDTH and HEIGHT values. It will have to be modified for a different
  // screen buffer size.
  // It also assumes color value for BLACK is 0.

  // local variable for screen buffer pointer,
  // which can be declared a read-write operand
  uint8_t* bPtr = sBuffer;

  asm volatile
  (
    // if value is zero, skip assigning to 0xff
    "cpse %[color], __zero_reg__\n"
    "ldi %[color], 0xFF\n"
    // counter = 0
    "clr __tmp_reg__\n"
    "loopto:\n"
    // (4x) push zero into screen buffer,
    // then increment buffer position
    "st Z+, %[color]\n"
    "st Z+, %[color]\n"
    "st Z+, %[color]\n"
    "st Z+, %[color]\n"
    // increase counter
    "inc __tmp_reg__\n"
    // repeat for 256 loops
    // (until counter rolls over back to 0)
    "brne loopto\n"
    : [color] "+d" (color),
      "+z" (bPtr)
    :
    :
  );
}

void clear()
{
  fillScreen(BLACK);
}

