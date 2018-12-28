/***************************************************
  Christmas Ornament 3D Printer
  A 3D Printed Christmas Ornament 3D Printer that 3D prints Chistmas Onraments
 ****************************************************/

#include <Adafruit_GFX.h>  // Core graphics library
//#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <SD.h>
#include <SPI.h>
#include <Stepper.h>

#define TFT_CS 11
#define TFT_RST 10  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 9

#define SD_CS 4  // Chip select line for SD card

#define TFT_MOSI 12  // Data out
#define TFT_SCLK 13  // Clock out
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

String folder = "SIMPLE";
int fileNum = 1;
//char* CurrFile = "";

#define INITIALIZING 0
#define IN_PROGRESS 1
#define COMPLETE 2

int printing = INITIALIZING;

#define LED1 A0
#define LED2 A1
#define BUTTON 6
#define SWITCH 1
#define SWGND 21
#define CURETIME 60000  //MILLISECONDS

const int stepsPerRevolution = 20;  // change this to fit the number of steps per revolution
// for your motor

float perStep = 0.13;  //(mm)

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, A3, A2, A4, A5);
#define STBY 0

void setup(void) {
  Serial.begin(115200);

  //  while (!Serial) {
  //    delay(10);  // wait for serial console
  //  }
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, LOW);
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, LOW);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED2, LOW);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(SWITCH, INPUT_PULLUP);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  tft.init(240, 240);  // initialize a ST7789 chip, 240x240 pixels

  tft.fillScreen(ST77XX_BLACK);

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    return;
  }
  Serial.println("OK!");

  File root = SD.open("/");
  //printDirectory(root, 0);
  root.close();
  pinMode(SWGND, OUTPUT);
  digitalWrite(SWGND, LOW);
  int buttState = digitalRead(BUTTON);
  while (buttState == HIGH) {
    buttState = digitalRead(BUTTON);
  }
  while (buttState == LOW) {
    buttState = digitalRead(BUTTON);
  }
  printing = IN_PROGRESS;
  digitalWrite(LED1, HIGH);
  delay(100);
  digitalWrite(LED1, LOW);
  myStepper.setSpeed(200);
  digitalWrite(STBY, HIGH);
  myStepper.step(-20);
  delay(10);
  myStepper.step(5);
  digitalWrite(STBY, LOW);
}

void loop() {
  while (printing == IN_PROGRESS) {
    fileNum++;
    displayFileName(fileNum, folder);
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    delay(CURETIME);
    if (fileNum == 1) {
      delay(CURETIME);
    }
    digitalWrite(LED2, LOW);
    digitalWrite(LED1, LOW);
    //tft.fillScreen(ST77XX_BLACK);
    digitalWrite(STBY, HIGH);
    myStepper.step(100);
    delay(500);
    myStepper.step(-120);
    delay(200);
    myStepper.step(5);
    delay(100);
    digitalWrite(STBY, LOW);
  }
  while (printing == COMPLETE) {
    digitalWrite(STBY, HIGH);
    myStepper.step(100);
    digitalWrite(STBY, LOW);
    while (1) {
    }
  }
}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.

#define BUFFPIXEL 60

void bmpDraw(char *filename, uint8_t x, uint16_t y) {
  File bmpFile;
  int bmpWidth, bmpHeight;             // W+H in pixels
  uint8_t bmpDepth;                    // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;             // Start of image data in file
  uint32_t rowSize;                    // Not always = bmpWidth; may have padding
  uint8_t sdbuffer[3 * BUFFPIXEL];     // pixel buffer (R+G+B per pixel)
  uint8_t buffidx = sizeof(sdbuffer);  // Current position in sdbuffer
  boolean goodBmp = false;             // Set to true on valid header parse
  boolean flip = true;                 // BMP is stored bottom-to-top
  int w, h, row, col;
  uint8_t r, g, b;
  uint32_t pos = 0, startTime = millis();

  if ((x >= tft.width()) || (y >= tft.height()))
    return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    printing = COMPLETE;
    Serial.println("Print Finished");
    return;
  }

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) {  // BMP signature
    Serial.print(F("File size: "));
    Serial.println(read32(bmpFile));
    (void)read32(bmpFile);             // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile);  // Start of image data
    Serial.print(F("Image Offset: "));
    Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: "));
    Serial.println(read32(bmpFile));
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) {    // # planes -- must be '1'
      bmpDepth = read16(bmpFile);  // bits per pixel
      Serial.print(F("Bit Depth: "));
      Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) {  // 0 = uncompressed

        goodBmp = true;  // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width())
          w = tft.width() - x;
        if ((y + h - 1) >= tft.height())
          h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.startWrite();
        tft.setAddrWindow(x, y, w, h);

        for (row = 0; row < h; row++) {  // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if (flip)  // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else  // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) {  // Need seek?
            tft.endWrite();
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer);  // Force buffer reload
          }

          for (col = 0; col < w; col++) {  // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) {  // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0;  // Set index to beginning
              tft.startWrite();
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r, g, b));
          }  // end pixel
        }    // end scanline
        tft.endWrite();
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      }  // end goodBmp
    }
  }

  bmpFile.close();
  if (!goodBmp)
    Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();  // LSB
  ((uint8_t *)&result)[1] = f.read();  // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();  // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();  // MSB
  return result;
}

void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void displayFileName(int filenum, String folder) {
  String sendfile = "";
  String num = String(filenum);
  sendfile += folder;
  sendfile += "/";
  sendfile += num;
  sendfile += ".bmp";
  char charBuf[20];
  sendfile.toCharArray(charBuf, 20);
  bmpDraw(charBuf, 0, 0);
}
