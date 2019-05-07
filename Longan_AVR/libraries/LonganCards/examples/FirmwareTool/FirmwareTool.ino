// Atmega hex file uploader (from SD card)
// Author: Nick Gammon
// Date: 22nd May 2012
// Version: 1.16     // NB update 'Version' variable below!

// Version 1.1: Some code cleanups as suggested on the Arduino forum.
// Version 1.2: Cleared temporary flash area to 0xFF before doing each page
// Version 1.3: Added ability to read from flash and write to disk, also to erase flash
// Version 1.4: Slowed down bit-bang SPI to make it more reliable on slower processors
// Version 1.5: Fixed bug where file "YES" might be saved instead of the correct _name
//              Also corrected flash size for Atmega1284P.
// Version 1.6: Echo user input
// Version 1.7: Moved signatures into PROGMEM. Added ability to change fuses/lock byte.
// Version 1.8: Made dates in file list line up. Omit date/time if default (unknown) date used.
//              Added "L" command (list directory)
// Version 1.9: Ensure in programming mode before access flash (eg. if reset removed to test)
//              Added reading of clock calibration byte (note: this cannot be changed)
// Version 1.10: Added signatures for ATtiny2313A, ATtiny4313, ATtiny13
// Version 1.11: Added signature for Atmega8
// Version 1.11: Added signature for Atmega32U4
// Version 1.12: Added option to allow target to run when not being programmed
// Version 1.13: Changed so you can set fuses without an SD card active.
// Version 1.14: Changed SPI writing to have pause before and after setting SCK low
// Version 1.15: Remembers last file _name uploaded in EEPROM
// Version 1.16: Allowed for running on the Leonardo, Micro, etc.


#include <SdFat.h>
#include <avr/eeprom.h>


// 1: SERIAL INPUT ANYTHING,
// 0: BUTTON INPUT
#define KEY_SERIAL_INPUT  0

const bool allowTargetToRun = true;         // if true, programming lines are freed when not programming



const char Version [] = "1.16";

const char name_168[] = "168.hex";         // bootloader _name
const char name_328[] = "328.hex";
const char name_game[] = "game.hex";
//const char name[] = "1.hex";


// 0 -  nothing
// 1 -  328
// 2 -  168


#define PROCESSOR328    1
#define PROCESSOR168    2
#define PROCESSOR32U4   3
char processorInsert = 0;

unsigned char delay_time_spi = 1;

bool isVerifyOk = 0;

// programming commands to send via SPI to the chip
enum {
    progamEnable = 0xAC,

    // writes are preceded by progamEnable
    chipErase = 0x80,
    writeLockByte = 0xE0,
    writeLowFuseByte = 0xA0,
    writeHighFuseByte = 0xA8,
    writeExtendedFuseByte = 0xA4,

    pollReady = 0xF0,

    programAcknowledge = 0x53,

    readSignatureByte = 0x30,
    readCalibrationByte = 0x38,

    readLowFuseByte = 0x50,       readLowFuseByteArg2 = 0x00,
    readExtendedFuseByte = 0x50,  readExtendedFuseByteArg2 = 0x08,
    readHighFuseByte = 0x58,      readHighFuseByteArg2 = 0x08,
    readLockByte = 0x58,          readLockByteArg2 = 0x00,

    readProgramMemory = 0x20,
    writeProgramMemory = 0x4C,
    loadExtendedAddressByte = 0x4D,
    loadProgramMemory = 0x40,

};  // end of enum
// e h l   fb d8 de
const byte fuseCommands [4] = { writeLowFuseByte, writeHighFuseByte, writeExtendedFuseByte, writeLockByte };

void writeFuse()
{
    //unsigned char valFuse[3] = {0xff, 0xde, 0x05};      // arduino uno
    //unsigned char valFuse[3] = {0xe2, 0xda, 0xf8};      // 168 - 8M,internal, lilypad arduino
    //unsigned char valFuse[3] = {0xE2, 0xda, 0x05};

    //unsigned char valFuse[3];
    
    unsigned char valFuse[3][3] = {
        
        0xff, 0xde, 0x05,           // 328, 8M, internal clock
        0xe2, 0xda, 0xf8,           // 168, 8M, internal clock
        0xff, 0xd8, 0xcb,           // 32u4, leonardo
    };

    for(int i = 0; i<3; i++)
    {
        startProgramming();
        writeFuse (valFuse[processorInsert-1][i], fuseCommands [i]);
    }

    //Serial.println("write fuse ok");
}

const byte MSPIM_SCK    = 8;    // port D bit 4
const byte MSPIM_SS     = A0;   // port D bit 5
const byte BB_MISO      = 7;    // port D bit 6
const byte BB_MOSI      = 9;    // port D bit 7


const byte PIN_PRESS    = A1;
const byte PIN_GREEN    = A2;
const byte PIN_RED      = A3;

// led and button
const byte PIN_LED_RED      = PIN_RED;
const byte PIN_LED_GREEN    = PIN_GREEN;
const byte PIN_LED_TEST     = PIN_GREEN;
const byte PIN_BTN_START    = PIN_PRESS;


void testPinInit()
{
    pinMode(PIN_PRESS, INPUT_PULLUP);
    //digitalWrite(PIN_PRESS, HIGH);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_RED, OUTPUT);
}


void blinkblink()
{   
    static bool ledst = 0;
    static unsigned long timer_s = millis();
    if(millis()-timer_s < 50)return;
    timer_s = millis();
    
    ledst = 1-ledst;
    
    if(ledst)
    {
        PORTC |= 0b00000100;
        PORTC &= 0b11110111;
    }
    else
    {
        PORTC |= 0b00001000;
        PORTC &= 0b11111011;
    }
    //digitalWrite(PIN_GREEN, ledst);
    //digitalWrite(PIN_RED, 1-ledst);
    
}


// for fast port access (Atmega328)
#define BB_MISO_PORT PIND
#define BB_MOSI_PORT PORTB
#define BB_SCK_PORT PORTB

const byte BB_SCK_BIT   = 0;
const byte BB_MISO_BIT  = 7;
const byte BB_MOSI_BIT  = 1;

// control speed of programming
const byte BB_DELAY_MICROSECONDS = 1;       // 4

// target board reset goes to here
const byte RESET = MSPIM_SS;

// SD chip select pin
const uint8_t chipSelect = 4;

const unsigned long NO_PAGE = 0xFFFFFFFF;
const int MAX_FILENAME = 13;
const int LAST_FILENAME_LOCATION_IN_EEPROM = 0;

//  to take
enum {
    checkFile,
    verifyFlash,
    writeToFlash,
};

// file system object
SdFat sd;

// copy of fuses/lock bytes found for this processor
byte fuses [5];

// meaning of bytes in above array
enum {
    lowFuse,
    highFuse,
    extFuse,
    lockByte,
    calibrationByte
};

// structure to hold signature and other relevant data about each chip
typedef struct {
    byte sig [3];
    char * desc;
    unsigned long flashSize;
    unsigned int baseBootSize;
    unsigned long pageSize;     // bytes
    byte fuseWithBootloaderSize;  // ie. one of: lowFuse, highFuse, extFuse
} signatureType;

const unsigned long kb = 1024;
const byte NO_FUSE = 0xFF;


// see Atmega datasheets
const signatureType PROGMEM signatures [] =
{
    //     signature        description   flash size   bootloader  flash  fuse
    //                                                     size    page    to
    //                                                             size   change

    // Attiny84 family
    { { 0x1E, 0x91, 0x0B }, "ATtiny24",   2 * kb,           0,   32,   NO_FUSE },
    { { 0x1E, 0x92, 0x07 }, "ATtiny44",   4 * kb,           0,   64,   NO_FUSE },
    { { 0x1E, 0x93, 0x0C }, "ATtiny84",   8 * kb,           0,   64,   NO_FUSE },

    // Attiny85 family
    { { 0x1E, 0x91, 0x08 }, "ATtiny25",   2 * kb,           0,   32,   NO_FUSE },
    { { 0x1E, 0x92, 0x06 }, "ATtiny45",   4 * kb,           0,   64,   NO_FUSE },
    { { 0x1E, 0x93, 0x0B }, "ATtiny85",   8 * kb,           0,   64,   NO_FUSE },

    // Atmega328 family
    { { 0x1E, 0x92, 0x0A }, "ATmega48PA",   4 * kb,         0,    64,  NO_FUSE },
    { { 0x1E, 0x93, 0x0F }, "ATmega88PA",   8 * kb,       256,   128,  extFuse },
    { { 0x1E, 0x94, 0x0B }, "ATmega168PA", 16 * kb,       256,   128,  extFuse },
    { { 0x1E, 0x95, 0x0F }, "ATmega328P",  32 * kb,       512,   128,  highFuse },

    // Atmega644 family
    { { 0x1E, 0x94, 0x0A }, "ATmega164P",   16 * kb,      256,   128,  highFuse },
    { { 0x1E, 0x95, 0x08 }, "ATmega324P",   32 * kb,      512,   128,  highFuse },
    { { 0x1E, 0x96, 0x0A }, "ATmega644P",   64 * kb,   1 * kb,   256,  highFuse },

    // Atmega2560 family
    { { 0x1E, 0x96, 0x08 }, "ATmega640",    64 * kb,   1 * kb,   256,  highFuse },
    { { 0x1E, 0x97, 0x03 }, "ATmega1280",  128 * kb,   1 * kb,   256,  highFuse },
    { { 0x1E, 0x97, 0x04 }, "ATmega1281",  128 * kb,   1 * kb,   256,  highFuse },
    { { 0x1E, 0x98, 0x01 }, "ATmega2560",  256 * kb,   1 * kb,   256,  highFuse },

    { { 0x1E, 0x98, 0x02 }, "ATmega2561",  256 * kb,   1 * kb,   256,  highFuse },

    // Atmega32U2 family
    { { 0x1E, 0x93, 0x89 }, "ATmega8U2",    8 * kb,       512,   128,  highFuse  },
    { { 0x1E, 0x94, 0x89 }, "ATmega16U2",  16 * kb,       512,   128,  highFuse  },
    { { 0x1E, 0x95, 0x8A }, "ATmega32U2",  32 * kb,       512,   128,  highFuse  },

    // Atmega32U4 family
    { { 0x1E, 0x94, 0x88 }, "ATmega16U4",  16 * kb,       512,   128,  highFuse },
    { { 0x1E, 0x95, 0x87 }, "ATmega32U4",  32 * kb,       512,   128,  highFuse },

    // ATmega1284P family
    { { 0x1E, 0x97, 0x05 }, "ATmega1284P", 128 * kb,   1 * kb,   256,  highFuse  },

    // ATtiny4313 family
    { { 0x1E, 0x91, 0x0A }, "ATtiny2313A",   2 * kb,        0,    32,  NO_FUSE  },
    { { 0x1E, 0x92, 0x0D }, "ATtiny4313",    4 * kb,        0,    64,  NO_FUSE  },

    // ATtiny13 family
    { { 0x1E, 0x90, 0x07 }, "ATtiny13A",     1 * kb,        0,    32,  NO_FUSE },

    // Atmega8A family
    { { 0x1E, 0x93, 0x07 }, "ATmega8A",      8 * kb,      256,    64,  highFuse },

};  // end of signatures


// number of items in an array
#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))



// which program instruction writes which fuse


// types of record in .hex file
enum {
    hexDataRecord,  // 00
    hexEndOfFile,   // 01
    hexExtendedSegmentAddressRecord, // 02
    hexStartSegmentAddressRecord  // 03
};

// Bit Banged SPI transfer
byte BB_SPITransfer (byte c)
{
    byte bit;

    for (bit = 0; bit < 8; bit++)
    {
        // write MOSI on falling edge of previous clock
        if (c & 0x80)
        BB_MOSI_PORT |= _BV (BB_MOSI_BIT);
        else
        BB_MOSI_PORT &= ~_BV (BB_MOSI_BIT);
        c <<= 1;

        // read MISO
        c |= (BB_MISO_PORT & _BV (BB_MISO_BIT)) != 0;

        // clock high
        BB_SCK_PORT |= _BV (BB_SCK_BIT);

        // delay between rise and fall of clock
        delayMicroseconds (delay_time_spi);

        // clock low
        BB_SCK_PORT &= ~_BV (BB_SCK_BIT);

        // delay between rise and fall of clock
         delayMicroseconds (delay_time_spi);
    }

    return c;
}  // end of BB_SPITransfer


// if signature found in above table, this is its index
int foundSig = -1;
byte lastAddressMSB = 0;
// copy of current signature entry for matching processor
signatureType currentSignature;

// execute one programming instruction ... b1 is command, b2, b3, b4 are arguments
//  processor may return a result on the 4th transfer, this is returned.
byte program (const byte b1, const byte b2 = 0, const byte b3 = 0, const byte b4 = 0)
{
    noInterrupts ();
    BB_SPITransfer (b1);
    BB_SPITransfer (b2);
    BB_SPITransfer (b3);
    byte b = BB_SPITransfer (b4);
    interrupts ();
    return b;
} // end of program

// read a byte from flash memory
byte readFlash (unsigned long addr)
{
    byte high = (addr & 1) ? 0x08 : 0;  // set if high byte wanted
    addr >>= 1;  // turn into word address

    // set the extended (most significant) address byte if necessary
    byte MSB = (addr >> 16) & 0xFF;
    if (MSB != lastAddressMSB)
    {
        program (loadExtendedAddressByte, 0, MSB);
        lastAddressMSB = MSB;
    }  // end if different MSB

    return program (readProgramMemory | high, highByte (addr), lowByte (addr));
} // end of readFlash

// write a byte to the flash memory buffer (ready for committing)
byte writeFlash (unsigned long addr, const byte data)
{
    byte high = (addr & 1) ? 0x08 : 0;  // set if high byte wanted
    addr >>= 1;  // turn into word address
    program (loadProgramMemory | high, 0, lowByte (addr), data);
} // end of writeFlash

// show a byte in hex with leading zero and optional newline
void showHex (const byte b, const boolean newline = false, const boolean show0x = true)
{
    if (show0x)
    Serial.print (F("0x"));
    char buf [4];
    sprintf (buf, "%02X ", b);
    Serial.print (buf);
    if (newline)
    Serial.println ();
}  // end of showHex

// convert two hex characters into a byte
//    returns true if error, false if OK
boolean hexConv (const char * (& pStr), byte & b)
{

    if (!isxdigit (pStr [0]) || !isxdigit (pStr [1]))
    {
        Serial.print (F("Invalid hex digits: "));
        Serial.print (pStr [0]);
        Serial.println (pStr [1]);
        return true;
    } // end not hex

    b = *pStr++ - '0';
    if (b > 9)
    b -= 7;

    // high-order nybble
    b <<= 4;

    byte b1 = *pStr++ - '0';
    if (b1 > 9)
    b1 -= 7;

    b |= b1;

    return false;  // OK
}  // end of hexConv

// poll the target device until it is ready to be programmed
void pollUntilReady ()
{
    while ((program (pollReady) & 1) == 1)
    {}  // wait till ready
}  // end of pollUntilReady

unsigned long pagesize;
unsigned long pagemask;
unsigned long oldPage;


unsigned long countLed = 0;


// show one progress symbol, wrap at 64 characters
unsigned int progressBarCount;

int state_led = 0;      // 0 -> red on  1-> green on
unsigned long timer_1 = 0;

void showProgress ()
{
    blinkblink();
}  // end of showProgress

// clear entire temporary page to 0xFF in case we don't write to all of it
void clearPage ()
{
    unsigned int len = currentSignature.pageSize;
    for (int i = 0; i < len; i++)
    writeFlash (i, 0xFF);
}  // end of clearPage

// commit page to flash memory
void commitPage (unsigned long addr)
{
    addr >>= 1;  // turn into word address

    // set the extended (most significant) address byte if necessary
    byte MSB = (addr >> 16) & 0xFF;
    if (MSB != lastAddressMSB)
    {
        program (loadExtendedAddressByte, 0, MSB);
        lastAddressMSB = MSB;
    }  // end if different MSB

    showProgress ();

    program (writeProgramMemory, highByte (addr), lowByte (addr));
    pollUntilReady ();

    clearPage();  // clear ready for next page full
}  // end of commitPage

// write data to temporary buffer, ready for committing
void writeData (const unsigned long addr, const byte * pData, const int length)
{
    // write each byte
    for (int i = 0; i < length; i++)
    {
        unsigned long thisPage = (addr + i) & pagemask;
        // page changed? commit old one
        if (thisPage != oldPage && oldPage != NO_PAGE)
        commitPage (oldPage);
        // now this is the current page
        oldPage = thisPage;
        // put byte into work buffer
        writeFlash (addr + i, pData [i]);
    }  // end of for

}  // end of writeData

// count errors
unsigned int errors;

void verifyData (const unsigned long addr, const byte * pData, const int length)
{
    // check each byte
    for (int i = 0; i < length; i++)
    {
        unsigned long thisPage = (addr + i) & pagemask;
        // page changed? show progress
        if (thisPage != oldPage && oldPage != NO_PAGE)
        showProgress ();
        // now this is the current page
        oldPage = thisPage;

        byte found = readFlash (addr + i);
        byte expected = pData [i];
        if (found != expected)
        {
            if (errors <= 100)
            {
                Serial.print (F("Verification error at address "));
                Serial.print (addr + i, HEX);
                Serial.print (F(". Got: "));
                showHex (found);
                Serial.print (F(" Expected: "));
                showHex (expected, true);
            }  // end of haven't shown 100 errors yet
            errors++;
        }  // end if error
    }  // end of for

}  // end of verifyData

boolean gotEndOfFile;
unsigned long extendedAddress;

unsigned long lowestAddress;
unsigned long highestAddress;
unsigned long bytesWritten;
unsigned int lineCount;

boolean processLine (const char * pLine, const byte action)
{
    if (*pLine++ != ':')
    {
        //Serial.println (F("Line does not start with ':' character."));
        return true;  // error
    }

    const int maxHexData = 40;
    byte hexBuffer [maxHexData];
    int bytesInLine = 0;

    if (action == checkFile)
    if (lineCount++ % 40 == 0)
    showProgress ();

    // convert entire line from ASCII into binary
    while (isxdigit (*pLine))
    {
        // can't fit?
        if (bytesInLine >= maxHexData)
        {
            //Serial.println (F("Line too long to process."));
            return true;
        } // end if too long

        if (hexConv (pLine, hexBuffer [bytesInLine++]))
        return true;
    }  // end of while

    if (bytesInLine < 5)
    {
        //Serial.println (F("Line too short."));
        return true;
    }

    // sumcheck it

    byte sumCheck = 0;
    for (int i = 0; i < (bytesInLine - 1); i++)
    sumCheck += hexBuffer [i];

    // 2's complement
    sumCheck = ~sumCheck + 1;

    // check sumcheck
    if (sumCheck != hexBuffer [bytesInLine - 1])
    {
        Serial.print (F("Sumcheck error. Expected: "));
        showHex (sumCheck);
        Serial.print (F(", got: "));
        showHex (hexBuffer [bytesInLine - 1], true);
        return true;
    }

    // length of data (eg. how much to write to memory)
    byte len = hexBuffer [0];

    // the data length should be the number of bytes, less
    //   length / address (2) / transaction type / sumcheck
    if (len != (bytesInLine - 5))
    {
        Serial.print (F("Line not expected length. Expected "));
        Serial.print (len, DEC);
        Serial.print (F(" bytes, got "));
        Serial.print (bytesInLine - 5, DEC);
        Serial.println (F(" bytes."));
        return true;
    }

    // two bytes of address
    unsigned long addrH = hexBuffer [1];
    unsigned long addrL = hexBuffer [2];

    unsigned long addr = addrL | (addrH << 8);

    byte recType = hexBuffer [3];

    switch (recType)
    {
        // stuff to be written to memory
        case hexDataRecord:
        lowestAddress = min (lowestAddress, addr + extendedAddress);
        highestAddress = max (lowestAddress, addr + extendedAddress + len - 1);
        bytesWritten += len;

        switch (action)
        {
            case checkFile:  // nothing much to do, we do the checks anyway
            break;

            case verifyFlash:
            verifyData (addr + extendedAddress, &hexBuffer [4], len);
            break;

            case writeToFlash:
            writeData (addr + extendedAddress, &hexBuffer [4], len);
            break;
        } // end of switch on action
        break;

        // end of data
        case hexEndOfFile:
        gotEndOfFile = true;
        break;

        // we are setting the high-order byte of the address
        case hexExtendedSegmentAddressRecord:
        extendedAddress = ((unsigned long) hexBuffer [4]) << 12;
        break;

        // ignore this, who cares?
        case hexStartSegmentAddressRecord:
        break;

        default:
        Serial.print (F("Cannot handle record type: "));
        Serial.println (recType, DEC);
        return true;
    }  // end of switch on recType

    return false;
} // end of processLine

//------------------------------------------------------------------------------
boolean readHexFile (const char * fName, const byte action)
{
    const int maxLine = 80;
    char buffer[maxLine];
    ifstream sdin (fName);
    int lineNumber = 0;
    gotEndOfFile = false;
    extendedAddress = 0;
    errors = 0;
    lowestAddress = 0xFFFFFFFF;
    highestAddress = 0;
    bytesWritten = 0;
    progressBarCount = 0;

    pagesize = currentSignature.pageSize;
    pagemask = ~(pagesize - 1);
    oldPage = NO_PAGE;

    Serial.print (F("Processing file: "));
    Serial.println (fName);

    // check for open error
    if (!sdin.is_open())
    {
        Serial.println (F("Could not open file."));
        return true;
    }

    switch (action)
    {
        case checkFile:
        Serial.println (F("Checking file ..."));
        break;

        case verifyFlash:
        Serial.println (F("Verifying flash ..."));
        break;

        case writeToFlash:
        Serial.println (F("Erasing chip ..."));
        program (progamEnable, chipErase);   // erase it
        pollUntilReady ();
        clearPage();  // clear temporary page
        Serial.println (F("Writing flash ..."));
        break;
    } // end of switch

    while (sdin.getline (buffer, maxLine))
    {
        lineNumber++;
        int count = sdin.gcount();
        if (sdin.fail())
        {
            Serial.print (F("Line "));
            Serial.println (lineNumber);
            Serial.print (F(" too long."));
            return true;
        }  // end of fail (line too long?)

        // ignore empty lines
        if (count > 1)
        {
            if (processLine (buffer, action))
            {
                Serial.print (F("Error in line "));
                Serial.println (lineNumber);
                return true;  // error
            }
        }
    }    // end of while each line

    if (!gotEndOfFile)
    {
        Serial.println (F("Did not get 'end of file' record."));
        return true;
    }

    switch (action)
    {
        case writeToFlash:
        // commit final page
        if (oldPage != NO_PAGE)
        commitPage (oldPage);
        Serial.println ();   // finish line of dots
        Serial.println (F("Written."));
        break;

        case verifyFlash:
        Serial.println ();   // finish line of dots
        if (errors == 0)
        {
            Serial.println (F("No errors found."));
            isVerifyOk = 1;
        }
        else
        {
            Serial.print (errors, DEC);
            Serial.println (F(" verification error(s)."));
            if (errors > 100)
            Serial.println (F("First 100 shown."));
            isVerifyOk = 0;
        }  // end if
        break;

        case checkFile:
        Serial.println ();   // finish line of dots
        Serial.print (F("Lowest address  = 0x"));
        Serial.println (lowestAddress, HEX);
        Serial.print (F("Highest address = 0x"));
        Serial.println (highestAddress, HEX);
        Serial.print (F("Bytes to write  = "));
        Serial.println (bytesWritten, DEC);
        break;

    }  // end of switch

    return false;
}  // end of readHexFile

bool startProgramming ()
{
    Serial.println (F("Attempting to enter programming mode ..."));

    byte confirm;
    pinMode (RESET, OUTPUT);
    digitalWrite (MSPIM_SCK, LOW);
    pinMode (MSPIM_SCK, OUTPUT);
    pinMode (BB_MOSI, OUTPUT);

    // we are in sync if we get back programAcknowledge on the third byte
    
    unsigned long timer_tmp = millis();
    do
    {
        // regrouping pause
        blinkblink();
        if(millis()-timer_tmp > 100)
        {
            timer_tmp = millis();
            delay_time_spi++;
            if(delay_time_spi > 3)return 0;
            Serial.println(delay_time_spi);
        }
        delay (25);

        // ensure SCK low
        noInterrupts ();
        digitalWrite (MSPIM_SCK, LOW);
        // then pulse reset, see page 309 of datasheet
        digitalWrite (RESET, HIGH);
        delayMicroseconds (10);  // pulse for at least 2 clock cycles
        digitalWrite (RESET, LOW);
        interrupts ();

        delay (25);  // wait at least 20 mS
        noInterrupts ();
        BB_SPITransfer (progamEnable);
        BB_SPITransfer (programAcknowledge);
        confirm = BB_SPITransfer (0);
        BB_SPITransfer (0);
        interrupts ();


    } while (confirm != programAcknowledge);

    Serial.println (F("Entered programming mode OK."));
    return 1;

}  // end of startProgramming

void stopProgramming ()
{
    // turn off pull-ups
    digitalWrite (RESET, LOW);
    digitalWrite (MSPIM_SCK, LOW);
    digitalWrite (BB_MOSI, LOW);
    digitalWrite (BB_MISO, LOW);

    // set everything back to inputs
    pinMode (RESET, INPUT);
    pinMode (MSPIM_SCK, INPUT);
    pinMode (BB_MOSI, INPUT);
    pinMode (BB_MISO, INPUT);

    //Serial.println (F("Programming mode off."));

} // end of startProgramming


// 328: 1E 95 0F
// 168: 1E 94 0B


void getSignature ()
{
    foundSig = -1;
    lastAddressMSB = 0;

    byte sig [3];
    Serial.print (F("Signature = "));
    for (byte i = 0; i < 3; i++)
    {
        sig [i] = program (readSignatureByte, 0, i);
        showHex (sig [i]);
    }  // end for each signature byte
    Serial.println ();
    
    if(sig[0] == 0x1E && sig[1] == 0x95 && sig[2] == 0x0F)      // 328
    {
        Serial.println("ATMEGA328P INSERT");
        processorInsert = PROCESSOR328;
    }
    else if(sig[0] == 0x1E && sig[1] == 0x94 && sig[2] == 0x0B)     // 168
    {
        Serial.println("ATMEGA168PA INSERT");
        processorInsert = PROCESSOR168;
    }
    else if(sig[0] == 0x1E && sig[1] == 0x95 && sig[2] == 0x87)     // leonardo
    {
        Serial.println("ATMEGA32U4 INSERT");
        processorInsert = PROCESSOR32U4;
    }

    for (int j = 0; j < NUMITEMS (signatures); j++)
    {
        memcpy_P (&currentSignature, &signatures [j], sizeof currentSignature);

        if (memcmp (sig, currentSignature.sig, sizeof sig) == 0)
        {
            foundSig = j;
            Serial.print (F("Processor = "));
            Serial.println (currentSignature.desc);
            Serial.print (F("Flash memory size = "));
            Serial.print (currentSignature.flashSize, DEC);
            Serial.println (F(" bytes."));

            // make sure extended address is zero to match lastAddressMSB variable
            program (loadExtendedAddressByte, 0, 0);
            return;
        }  // end of signature found
    }  // end of for each signature

    Serial.println (F("Unrecogized signature."));
}  // end of getSignature


void getFuseBytes ()
{
    fuses [lowFuse]   = program (readLowFuseByte, readLowFuseByteArg2);
    fuses [highFuse]  = program (readHighFuseByte, readHighFuseByteArg2);
    fuses [extFuse]   = program (readExtendedFuseByte, readExtendedFuseByteArg2);
    fuses [lockByte]  = program (readLockByte, readLockByteArg2);
    fuses [calibrationByte]  = program (readCalibrationByte);

    Serial.print (F("LFuse = "));
    showHex (fuses [lowFuse], true);
    Serial.print (F("HFuse = "));
    showHex (fuses [highFuse], true);
    Serial.print (F("EFuse = "));
    showHex (fuses [extFuse], true);
    Serial.print (F("Lock byte = "));
    showHex (fuses [lockByte], true);
    Serial.print (F("Clock calibration = "));
    showHex (fuses [calibrationByte], true);
}  // end of getFuseBytes


// write specified value to specified fuse/lock byte
void writeFuse (const byte newValue, const byte instruction)
{
    if (newValue == 0)
    return;  // ignore

    program (progamEnable, instruction, 0, newValue);
    pollUntilReady ();
}  // end of writeFuse


void showDirectory ()
{

    SdFile file;
    char dirname[MAX_FILENAME];

    Serial.println ();
    Serial.println (F("HEX files in root directory:"));
    Serial.println ();

    // back to start of directory
    sd.vwd()->rewind ();

    // open next file in root.  The volume working directory, vwd, is root
    while (file.openNext(sd.vwd(), O_READ)) {
        file.getFilename(dirname);
        byte len = strlen (dirname);
        if (len > 4 && strcmp (&dirname [len - 4], ".HEX") == 0)
        {
            Serial.print (dirname);
            for (byte i = strlen (dirname); i < 13; i++)
            Serial.write (' ');  // space out so dates line up
            Serial.print (F(" : "));
            char buf [12];
            sprintf (buf, "%10lu", file.fileSize ());

            Serial.print (buf);
            Serial.print (F(" bytes."));

            dir_t d;
            if (!file.dirEntry(&d))
            Serial.println(F("Failed to find file date/time."));
            else if (d.creationDate != FAT_DEFAULT_DATE)
            {
                Serial.print(F("  Created: "));
                file.printFatDate(&Serial, d.creationDate);
                Serial.print(F(" "));
                file.printFatTime(&Serial, d.creationTime);
                Serial.print(F(".  Modified: "));
                file.printFatDate(&Serial, d.lastWriteDate);
                Serial.print(F(" "));
                file.printFatTime(&Serial, d.lastWriteTime);
            }  // end of got date/time from directory
            Serial.println ();
        }
        file.close();
    }  // end of listing files

}  // end of showDirectory

char lastFileName [MAX_FILENAME] = { 0 };


/*led and button api*/

bool isBtnStart()
{
    return 1-digitalRead(PIN_BTN_START);
/*
    if(Serial.available())
    {
        char c = Serial.read();
        if(c != -1)return 1;
    }
    
    return 0;*/
}

void redOn()
{
    digitalWrite(PIN_LED_RED, HIGH);
}

void redOff()
{
    digitalWrite(PIN_LED_RED, LOW);
}

void greenOn()
{
    digitalWrite(PIN_LED_GREEN, HIGH);
}

void greenOff()
{
    digitalWrite(PIN_LED_GREEN, LOW);
}



//------------------------------------------------------------------------------
//      SETUP
//------------------------------------------------------------------------------
void setup ()
{

//    int error = paj7620Init();      // initialize Paj7620 registers
    Serial.begin(115200);
    testPinInit();

    //btntest();
    
    pinMode(10, OUTPUT);
    pinMode(9, OUTPUT);
    digitalWrite(9, HIGH);

    Serial.println("Burning Bootloader for Arduino Leonardo");
    Serial.println (F("Reading SD card ..."));

    if (!sd.begin (chipSelect, SPI_HALF_SPEED))
    {
        sd.initErrorPrint();
        Serial.println("SD card open fail.");
        redOn();
        while(1)
        {
            digitalWrite(PIN_RED, 1-digitalRead(PIN_RED));
            delay(100);
        }
    }
    else
    {
        showDirectory ();
    }

    // find what filename they used last
    eeprom_read_block (&lastFileName, LAST_FILENAME_LOCATION_IN_EEPROM, MAX_FILENAME);
    lastFileName [MAX_FILENAME - 1] = 0;  // ensure terminating null

    for (byte i = 0; i < strlen (lastFileName); i++)
    {
        if (!isprint (lastFileName [i]))
        {
            lastFileName [0] = 0;
            break;
        }
    }
    
    //redOn();

}  // end of setup


// write bootloader
bool writeFlashContents ()
{
    // ensure back in programming mode
    startProgramming ();

    // now commit to flash
    Serial.println("write to flash");
    
    if(processorInsert == PROCESSOR168)
    {
        readHexFile(name_168, writeToFlash);
        Serial.println("write to flash ok\r\nbegin to verify...");
        readHexFile(name_168, verifyFlash);
        Serial.println("verify ok");
    }
    else if(processorInsert == PROCESSOR328)
    {
        readHexFile(name_328, writeToFlash);
        Serial.println("write to flash ok\r\nbegin to verify...");
        readHexFile(name_328, verifyFlash);
        Serial.println("verify ok");
    }
    else if(processorInsert == PROCESSOR32U4)
    {
        readHexFile(name_game, writeToFlash);
        Serial.println("write to flash ok\r\nbegin to verify...");
        readHexFile(name_game, verifyFlash);
        Serial.println("verify ok");
    }
    return 1;
}  // end of writeFlashContents


void loop ()
{
    
    waitBtn();
    redOn();
    unsigned long timer_tmp = millis();
    
START:
    isVerifyOk = 0;
    redOn();
    greenOff();

    Serial.println ();
    Serial.println (F("--------- Starting ---------"));
    Serial.println ();

    if(!startProgramming ())
    {
        redOn();
        greenOff();
        return;
    }

    getSignature ();
    getFuseBytes ();

    // don't have signature? don't proceed
    if (foundSig == -1)
    {
        delay_time_spi++;
        goto START;
        while(1);
    }

    redOff();

    if (allowTargetToRun)
    stopProgramming ();

    if (allowTargetToRun)
    startProgramming ();

    Serial.println("\r\nBegin to Programming...........");

    if(writeFlashContents () && isVerifyOk)
    {
        writeFuse();
        redOff();
        greenOn();
    }
    else
    {
        greenOff();
        redOn();
    }

    digitalWrite(RESET, HIGH);

    Serial.print("THIS PROGRAM TAKE: ");
    Serial.print((float)(millis()-timer_tmp)/1000.0);
    Serial.println(" s");
    //while(1);
    Serial.println("print s to restart...");
}  // end of loop

// wait for button to be pressed

void blinkblinkgreen()
{
    static unsigned long timer_s = millis();
    if(millis()-timer_s < 100)return;
    timer_s = millis();
    
    digitalWrite(PIN_LED_GREEN, 1-digitalRead(PIN_LED_GREEN));
}

void waitBtn()
{
    delay_time_spi = 1;

#if (KEY_SERIAL_INPUT==1) // Serial INPUT

    while(!Serial.available());
    while(Serial.available())Serial.read();
    
#elif (KEY_SERIAL_INPUT == 0) // button input

    while(!isBtnStart())
    {
        blinkblinkgreen();
    }

#endif
}

void waitBtnRelease()
{
    while(!digitalRead(PIN_PRESS));
}
