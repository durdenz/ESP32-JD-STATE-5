// Simple State Machine Based Soda Dispenser
// GD4 and GD5 with special guest GD3
#include <TimeLib.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <FS.h>
#include <SD.h>
#include <AnimatedGIF.h>
#include <JPEGDecoder.h>

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;

String gifFiles[] = {"/VendLoad.gif",
                    "/StandBy_NiCola_optimize.gif",
                    "/Logo_Trans_Pos.gif",
                    "/Header_Nicola_FULL.gif"};
int gifIndex = 0;

bool AnimateGIF = false;

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

// SD Card Reader pins
#define SD_CS 5 // Chip Select

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define FONT_SIZE 2

// Define Button 1
#define BTN1_X 30
#define BTN1_Y 150
#define BTN1_WIDTH 180
#define BTN1_HEIGHT 35

// Define State Machine States
#define STATE_ACTIVE 1
#define STATE_STANDBY 2
#define STATE_VENDING 3

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

// State Machine Current State
int currentState;

// State Machine New State
int newState = 0;

// Indicate State Change is in Progess
bool stateChange = false;

// Standby Timer Variables
#define STANDBY_DURATION 2  // Inactivity Timeout in Minutes
time_t standbyStart = now();

// Stepper Motor Control 
const int DIR = 23;   // GPIO Pin 23 for Direction
const int STEP = 22;  // GPIO Pin 22 for Step
const int steps_per_rev = 200;

// Function to see if Button 1 has been pressed
// Inputs are X and Y from touchscreen press
// Returns true if X and Y are within Button 1 area
// Returns false otherwise
//
bool btn1_pressed(int x,int y) {
  // Check to see if the coordinates of the touch fall inside the range of the button
  if (((x>=BTN1_X) && (x<=(BTN1_X+BTN1_WIDTH))) && ((y>=BTN1_Y)&&(y<=(BTN1_Y+BTN1_HEIGHT)))) {
    Serial.println("Button 1 Pressed");
    return(true);
  } else {
    Serial.println("Button 1 Not Pressed");
    return(false);
  }
}

// Initialize SD Card Reader
bool SD_Init(int cs) {
  if (!SD.begin(cs, tft.getSPIinstance())) {
    Serial.println("Card Mount Failed");
    return(false);
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return(false);
  }

    Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  Serial.println("SD Card initialization Complete");
  return(true);
}

// Create Pointer for hardware timer instance
hw_timer_t *Timer0 = NULL;

// Create mutex for irq handler
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Interrupt handler for Timer0
void IRAM_ATTR onTimer0()
{
  // Lock mutex during ISR
  portENTER_CRITICAL_ISR(&timerMux);

  // Serial.println("Timer0 IRQ Handler Entered");

  // Checks if Touchscreen was touched, then checks if button pressed and changes state
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    // Touchscreen event has occurred
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    // Touch event resets inactivity timer 
    standbyStart = now();

    if (currentState == STATE_ACTIVE) {
      // Check and see if touch was pressing Button 1
      // If so, change state to vending
      if (btn1_pressed(x, y)) {
        changeState(STATE_VENDING);
      } 
    } else if (currentState == STATE_STANDBY) {
      changeState(STATE_ACTIVE);
    } else if (currentState == STATE_VENDING) {
      // Nothing to do for now
    }
  } else {    // No Touchscreen Event
    // Check and see if inactivity time has been exceeded and if so, go into standby state
    if ((currentState == STATE_ACTIVE) && ((minute(now()) - minute(standbyStart)) >= STANDBY_DURATION)) {
      changeState(STATE_STANDBY);
    }
  }

  // Unlock mutex on ISR exit
  portEXIT_CRITICAL_ISR(&timerMux);
}

// changeState - Set Semaphores to signal loop() on state changes
void changeState(int ns) {
  stateChange = true;   // Signal state change is in progress
  newState = ns;        // Set new state
  AnimateGIF = false;   // Signal all animations to stop
}

// Setup Timer0
void Setup_Timer0() {
  Timer0 = timerBegin(1000000); // Timer Frequency in HZ, 1000000 = 1 MHZ
  timerAttachInterrupt(Timer0, &onTimer0);
    timerAlarm(Timer0, 100000,true, 0); // Set Timer to call handler every 100000 ticks = 100 milliseconds
}

// Function to dispense Soda
//
bool dispenseSoda() {
  // Put code here to drive motor and dispense soda
  digitalWrite(DIR, HIGH); // Set Direction to Clockwise
  Serial.println("Rotating Clockwise");

  for (int i=0; i<(steps_per_rev/2); i++) {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(2000);
      digitalWrite(STEP, LOW);
      delayMicroseconds(2000);
  }
  delay(1000);
  for (int i=0; i<(steps_per_rev/2); i++) {
      digitalWrite(STEP, HIGH);
      delayMicroseconds(2000);
      digitalWrite(STEP, LOW);
      delayMicroseconds(2000);
  }
  return(true);
}

// Perform Actions for Vending State
void StateVending() {
  Serial.println("Entering State = Vending");
  
  // Set current state to Vending and mark stateChange as false
  currentState = STATE_VENDING;
  stateChange = false;

  // Stop Animations
  AnimateGIF = false;

  // Clear TFT screen 
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Prepare to Print to screen
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  int textY = 80;
 
  // Print to screen
  // String tempText = "Vending State";

  // tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);

  gifIndex = 0; // gif for Dispense State
  PlayGIF(gifFiles[gifIndex], 1, 5); // Play GIF off of SD card for 5 seconds

  // Call Function to dispense Soda
  //
  dispenseSoda();
  
  // Vending is complete - Set State to Active 
  changeState(STATE_ACTIVE);
}

// Perform Actions for Active State
void StateActive() {
  Serial.println("Entering State = Active");
  
  // Set current state to Active and mark stateChange as false
  currentState = STATE_ACTIVE;
  stateChange = false;

  // Stop Animations
  AnimateGIF = false;

  // Clear TFT screen 
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Prepare to Print to screen
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  int textY = 80;
 
  // Print to screen
  // String tempText = "Active State";
  // tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);
  // tft.drawCentreString("Welcome to Jordes Sodaland", centerX, 30, FONT_SIZE);
  // tft.drawCentreString("Press the button for a Soda!", centerX, centerY, FONT_SIZE);

  // Draw Button
  tft.drawRect( BTN1_X, BTN1_Y, BTN1_WIDTH, BTN1_HEIGHT, TFT_BLACK);

  // PlayGIF
  gifIndex = 3; // gif for Active State
  PlayGIF(gifFiles[gifIndex], 0, 0); // Play GIF off of SD card continuous loop

  // Start Time for Inactive Timer
  standbyStart = now();
}

// Perform Actions for StandBy State
void StateStandBy() {
  Serial.println("Entering State = StandBy");

  // Set current state to StandBy
  currentState = STATE_STANDBY;
  stateChange = false;

  // Stop Animations
  AnimateGIF = false;

  // Clear TFT screen 
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // Prepare to Print to screen
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  int textY = 80;
 
  // Print to screen
  // String tempText = "StandBy State";
  // tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);

  // PlayGIF
  gifIndex = 1; // gif for StandBy State
  PlayGIF(gifFiles[gifIndex], 0, 0); // Play GIF off of SD card for 1 iteration or no longer than 5 seconds

  // drawSdJpeg("/lena20k.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card
}

void setup() {
  Serial.begin(9600);

  // Setup the GPIO pins for the Stepper Motor
    pinMode(STEP, OUTPUT);
    pinMode(DIR, OUTPUT);

  // Start the SPI for the touchscreen and init the touchscreen
  //
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  // Set the Touchscreen rotation in portrait mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 3: touchscreen.setRotation(3);
  //
  touchscreen.setRotation(4);

  // Start the tft display
  //
  tft.init();

  // Set the TFT display rotation in portait mode
  //
  tft.setRotation(2);

  // Clear the screen before writing to it
  //
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Initialize the gif decoder
  gif.begin(BIG_ENDIAN_PIXELS);

  // Initialize SD Card Reader
  if (SD_Init(SD_CS) == false) {
    Serial.println("SD Card Initialization Failed");
  }

  // Initialize Timer0 for IRQ driven events
  Setup_Timer0();
  
  // Set initial State to Active State
  changeState(STATE_ACTIVE);
}

void loop() {
  if (stateChange) {
    switch (newState) {
    case STATE_ACTIVE:
      StateActive();
      break;
    case STATE_STANDBY:
      StateStandBy();
      break;
    case STATE_VENDING:
      StateVending();
      break;
    }
  }
}

// ======================================================
// BEGIN - GIF Support Functions
//
bool PlayGIF(String gifFile, int iterations, int seconds) {
  time_t startTime = now();
  int iCount = 1;

  Serial.printf("PlayGIF: Entered - Iteration = %d\n", iCount);

  AnimateGIF = true;
  while (AnimateGIF == true) {
    if (gif.open(gifFile.c_str(), fileOpen, fileClose, fileRead, fileSeek, GIFDraw)) {
    #if defined(SERIALDEBUG)
        Serial.print("SUCCESS gif.open: ");
        Serial.println(gifFiles[gifIndex].c_str());
        Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    #endif
      while (gif.playFrame(true, NULL)) {
        yield();
        if (AnimateGIF == false) { // **** GD **** Test to see if we can break out
          gif.close();
          tft.fillScreen(TFT_BLACK);
          return(true);
        }
      }
      gif.close();
    } else {
      Serial.print("FAIL gif.open: ");
      Serial.println(gifFile.c_str());
      return (false);
    }
    if ((seconds > 0) && ((second(now()) - second(startTime)) >= seconds)) {
      Serial.println("PlayGIF: Time Exceeded");
      AnimateGIF = false;
      return(true);
    }
    if ((iterations > 0) && (iCount++ > iterations)) {
      Serial.printf("PlayGIF: Iterations Exceeded, iCount = %d\n", iCount);
      AnimateGIF = false;
      return(0);
    }
    if (AnimateGIF == false) {
      return(true);
    }
    Serial.printf("PlayGIF: Iteration iCount = %d\n", iCount);
  }
}

void* fileOpen(const char* filename, int32_t* pFileSize) {
  File* f = new File();
  *f = SD.open(filename, FILE_READ);
  if (*f) {
    *pFileSize = f->size();
    
#if defined(SERIALDEBUG)
    Serial.print("SUCCESS - fileopen: ");
    Serial.println(filename);
#endif

    return (void*)f;
  } else {
    Serial.print("FAIL - fileopen: ");
    Serial.println(filename);
    delete f;
    return NULL;
  }
}

void fileClose(void* pHandle) {
#if defined(SERIALDEBUG)
  Serial.println("ENTER - fileClose");
#endif

  File* f = static_cast<File*>(pHandle);
  if (f != NULL) {
    f->close();
    delete f;
  }
}

int32_t fileRead(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen) {
#if defined(SERIALDEBUG)
  Serial.println("ENTER - fileRead");
#endif

  File* f = static_cast<File*>(pFile->fHandle);
  if (f == NULL) {
    Serial.println("f == NULL");
    return 0;
  }
#if defined(SERIALDEBUG)
  Serial.print("File* f= ");
  Serial.print(reinterpret_cast<uintptr_t> (f));
  Serial.print("\nCalling f->read");
  Serial.print("\npBuf: ");
  Serial.print(reinterpret_cast<uintptr_t> (pBuf));
  Serial.print("\niLen: ");
  Serial.print(iLen, HEX);
#endif

  int32_t iBytesRead = f->read(pBuf, iLen);
  pFile->iPos = f->position();
  
#if defined(SERIALDEBUG)
  Serial.print("iBytesRead = ");
  Serial.println(iBytesRead);
  Serial.print("iPos = ");
  Serial.println(pFile->iPos);
#endif

  return iBytesRead;
}

int32_t fileSeek(GIFFILE* pFile, int32_t iPosition) {
#if defined(SERIALDEBUG)
  Serial.println("ENTER - fileSeek");
#endif

  File* f = static_cast<File*>(pFile->fHandle);
  if (f == NULL) return 0;
  f->seek(iPosition);
  pFile->iPos = f->position();

#if defined(SERIALDEBUG)
  Serial.print("iPos = ");
  Serial.println(pFile->iPos);
#endif

  return pFile->iPos;
}

//
// END -- GIF Support Functions
// ======================================================

//
// Start of Code for GIF Renderer = GIFDraw()
//
// GIFDraw is called by AnimatedGIF library to draw frame to screen

#define DISPLAY_WIDTH  tft.width()
#define DISPLAY_HEIGHT tft.height()
#define BUFFER_SIZE 256            // Optimum is >= GIF width or integral division of width

#ifdef USE_DMA
  uint16_t usTemp[2][BUFFER_SIZE]; // Global to support DMA use
#else
  uint16_t usTemp[1][BUFFER_SIZE];    // Global to support DMA use
#endif
bool     dmaBuf = 0;
  
// Draw a line of image directly on the LCD
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;

//  Serial.print("GIFDraw() Entered");


  // Display bounds check and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > DISPLAY_WIDTH)
    iWidth = DISPLAY_WIDTH - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1)
    return;

  tft.startWrite();

  // Old image disposal
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = &usTemp[0][0];
      while (c != ucTransparent && s < pEnd && iCount < BUFFER_SIZE )
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        // DMA would degrtade performance here due to short line segments
        tft.setAddrWindow(pDraw->iX + x, y, iCount, 1);
        tft.pushPixels(usTemp, iCount);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          x++;
        else
          s--;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;

    // Unroll the first pass to boost DMA performance
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    if (iWidth <= BUFFER_SIZE)
      for (iCount = 0; iCount < iWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
    else
      for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA // 71.6 fps (ST7796 84.5 fps)
    tft.dmaWait();
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
    dmaBuf = !dmaBuf;
#else // 57.0 fps
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixels(&usTemp[0][0], iCount);
#endif

    iWidth -= iCount;
    // Loop if pixel buffer smaller than width
    while (iWidth > 0)
    {
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA
      tft.dmaWait();
      tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
      dmaBuf = !dmaBuf;
#else
      tft.pushPixels(&usTemp[0][0], iCount);
#endif
      iWidth -= iCount;
    }
  }
  tft.endWrite();
} /* GIFDraw() */

// ======================================================
// BEGIN - JPEG Support Functions
//

//####################################################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
void drawSdJpeg(const char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // Use one of the following methods to initialise the decoder:
  bool decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  //bool decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)

  if (decoded) {
    // print information about the image to the serial port
    jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

  //jpegInfo(); // Print information from the JPEG file (could comment this line out)

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  
  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

    // Calculate coordinates of top left corner of current MCU
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tft.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  tft.setSwapBytes(swapBytes);

  showTime(millis() - drawTime); // These lines are for sketch testing only
}

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
// JpegDec.decodeFile(...) or JpegDec.decodeArray(...) must be called before this info is available!
void jpegInfo() {

  // Print information extracted from the JPEG file
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}

//####################################################################################################
// Show the execution time (optional)
//####################################################################################################
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16-bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime) {
  //tft.setCursor(0, 0);
  //tft.setTextFont(1);
  //tft.setTextSize(2);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.print(F(" JPEG drawn in "));
  //tft.print(msTime);
  //tft.println(F(" ms "));
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}

// END -- JPEG Support Functions
// ======================================================



