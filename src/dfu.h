
#include <stdint.h>
#include <kinetis.h>
#include <stdio.h>
#include <WProgram.h>

#if defined(__MK20DX128__)
#define FLASH_SIZE              0x20000
#define FLASH_SECTOR_SIZE       0x400
#define FLASH_ID                "fw_teensy3.0"
#define RESERVE_FLASH (2 * FLASH_SECTOR_SIZE)
#elif defined(__MK20DX256__)
#define FLASH_SIZE              0x40000
#define FLASH_SECTOR_SIZE       0x800
#define FLASH_ID                "fw_teensy3.1"
#define RESERVE_FLASH (1 * FLASH_SECTOR_SIZE)
#else
#error NOT SUPPORTED
#endif

#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL);


/*void upgrade_firmware(void);
static int flash_hex_line(const char *line);
int parse_hex_line(const char *theline, uint8_t bytes[], unsigned int *addr, unsigned int *num, unsigned int *code);
static int flash_block(uint32_t address, uint32_t *bytes, int count);*/

// apparently better - thanks to Frank Boesing
#define RAMFUNC  __attribute__ ((section(".fastrun"), noinline, noclone, optimize("Os") ))



// check that the uploaded firmware contains a string that indicates that it will run on this MCU
static int check_compatible(uint32_t min, uint32_t max){
  uint32_t i;

  // look for FLASH_ID in the new firmware
  for (i = min; i < max - strlen(FLASH_ID); ++i) {
    if (strncmp((char *)i, FLASH_ID, strlen(FLASH_ID)) == 0)
      return 1;
  }
  return 0;
}

// **********************************************************
/* Intel Hex records:
Start code, one character, an ASCII colon ':'.
Byte count, two hex digits, indicating the number of bytes (hex digit pairs) in the data field.
Address, four hex digits
Record type (see record types below), two hex digits, 00 to 05, defining the meaning of the data field.
Data, a sequence of n bytes of data, represented by 2n hex digits.
Checksum, two hex digits, a computed value that can be used to verify the record has no errors.

Example:
:109D3000711F0000AD38000005390000F546000035
:049D400001480000D6
:00000001FF

*/
/* Intel HEX read/write functions, Paul Stoffregen, paul@ece.orst.edu */
/* This code is in the public domain.  Please retain my name and */
/* email address in distributed copies, and let me know about any bugs */
/* I, Paul Stoffregen, give no warranty, expressed or implied for */
/* this software and/or documentation provided, including, without */
/* limitation, warranty of merchantability and fitness for a */
/* particular purpose. */
// type modifications by Jon Zeeff
/* parses a line of intel hex code, stores the data in bytes[] */
/* and the beginning address in addr, and returns a 1 if the */
/* line was valid, or a 0 if an error occured.  The variable */
/* num gets the number of bytes that were stored into bytes[] */

// **********************************************************
int parse_hex_line (const char *theline, uint8_t bytes[], uint32_t * addr, unsigned int *num, unsigned int *code){
  unsigned sum, len, cksum;
  const char *ptr;
  int temp;

  *num = 0;
  if (theline[0] != ':')
    return 0;
  if (strlen (theline) < 11)
    return 0;
  ptr = theline + 1;
  if (!sscanf (ptr, "%02x", &len))
    return 0;
  ptr += 2;
  if (strlen (theline) < (11 + (len * 2)))
    return 0;
  if (!sscanf (ptr, "%04x", (unsigned int *)addr))
    return 0;
  ptr += 4;
  /* Serial.printf("Line: length=%d Addr=%d\n", len, *addr); */
  if (!sscanf (ptr, "%02x", code))
    return 0;
  ptr += 2;
  sum = (len & 255) + ((*addr >> 8) & 255) + (*addr & 255) + (*code & 255);
  while (*num != len)
  {
    if (!sscanf (ptr, "%02x", &temp))
      return 0;
    bytes[*num] = temp;
    ptr += 2;
    sum += bytes[*num] & 255;
    (*num)++;
    if (*num >= 256)
      return 0;
  }
  if (!sscanf (ptr, "%02x", &cksum))
    return 0;

  if (((sum & 255) + (cksum & 255)) & 255)
    return 0;			/* checksum error */
  return 1;
}

// *****************************************************************************
// WARNING:  you can destroy your MCU with flash erase or write!
// This code may or may not protect you from that.
// Modifications by Jon Zeeff
// Original by Niels A. Moseley, 2015.
// This code is released into the public domain.
// https://namoseley.wordpress.com/2015/02/04/freescale-kinetis-mk20dx-series-flash-erasing/
static int leave_interrupts_disabled = 0;

// *********************************
// actual flash operation occurs here - must run from ram
// flash a 4 byte word
RAMFUNC static int flash_word (uint32_t address, uint32_t word_value){
  if (address >= FLASH_SIZE || (address & 0B11) != 0) // basic checks
    return 1;

  // correct value in FTFL_FSEC no matter what
  if (address == 0x40C) {
    word_value = 0xFFFFFFFE;
  }

  // check if already done - not an error
  if (*(volatile uint32_t *) address == word_value)
    return 0;

  // check if not erased
  if (*(volatile uint32_t *) address != 0xFFFFFFFF)  // TODO this fails
    return 4;

  __disable_irq ();

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)  // wait for ready
  {
  };

  // clear error flags
  FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0;

  // program long word!
  FTFL_FCCOB0 = 0x06;   // PGM
  FTFL_FCCOB1 = address >> 16;
  FTFL_FCCOB2 = address >> 8;
  FTFL_FCCOB3 = address;
  FTFL_FCCOB4 = word_value >> 24;
  FTFL_FCCOB5 = word_value >> 16;
  FTFL_FCCOB6 = word_value >> 8;
  FTFL_FCCOB7 = word_value;

  FTFL_FSTAT = FTFL_FSTAT_CCIF;  // execute!

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)  // wait for ready
  {
  };

  FMC_PFB0CR |= 0xF << 20;  // flush cache

  if (!leave_interrupts_disabled)
    __enable_irq ();

  // check if done OK
  if (*(volatile uint32_t *) address != word_value)
    return 8;

  return FTFL_FSTAT & (FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0);
}

int erase_count = 0;

// *************************************
RAMFUNC static int flash_erase_sector (uint32_t address, int unsafe){
  if (address > FLASH_SIZE || (address & (FLASH_SECTOR_SIZE - 1)) != 0) // basic checks
    return 1;

  if (address == (0x40C & ~(FLASH_SECTOR_SIZE - 1)) && unsafe != 54321)    // 0x40C is dangerous, don't erase it without override
    return 2;

  __disable_irq ();

  // wait for flash to be ready!
  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)
  {
  };

  // clear error flags
  FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0;

  // erase sector
  FTFL_FCCOB0 = 0x09;
  FTFL_FCCOB1 = address >> 16;
  FTFL_FCCOB2 = address >> 8;
  FTFL_FCCOB3 = address;

  FTFL_FSTAT = FTFL_FSTAT_CCIF;  // execute!

  while ((FTFL_FSTAT & FTFL_FSTAT_CCIF) != FTFL_FSTAT_CCIF)  // wait for ready
  {
  };

  FMC_PFB0CR = 0xF << 20;  // flush cache

  if (!leave_interrupts_disabled)
    __enable_irq ();

  return FTFL_FSTAT & (FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL | FTFL_FSTAT_MGSTAT0);
}

// ***********************************************
// move upper half down to lower half
// DANGER: if this is interrupted, the teensy could be permanently destroyed
RAMFUNC static void flash_move (uint32_t min_address, uint32_t max_address){
  leave_interrupts_disabled = 1;  

  min_address &= ~(FLASH_SECTOR_SIZE - 1);      // round down

  uint32_t address;
  int error = 0;

  // below here is critical

  // copy upper to lower, always erasing as we go up
  for (address = min_address; address <= max_address; address += 4) {

    if ((address & (FLASH_SECTOR_SIZE - 1)) == 0) {   // new sector?
      error |= flash_erase_sector(address, 54321);

      if (address == (0x40C & ~(FLASH_SECTOR_SIZE - 1)))  // critical sector
        error |= flash_word(0x40C, 0xFFFFFFFE);                  // fix it immediately
    }

    error |= flash_word(address, *(uint32_t *)(address + FLASH_SIZE / 2));

  } // for

  // hint - can use LED here for debugging.  Or disable erase and load the same program as is running.

  if (error) {
    //digitalWrite(USER_LED, HIGH);    // set the LED on and halt
    for (;;) {}
  }

  // restart to run new program
#define CPU_RESTART_ADDR ((uint32_t *)0xE000ED0C)
#define CPU_RESTART_VAL 0x5FA0004
  *CPU_RESTART_ADDR = CPU_RESTART_VAL;

}

// **************************
// take a word aligned array of words and write it to upper memory flash
static int flash_block (uint32_t address, uint32_t * bytes, int count){
  int ret;

  if ((address % 4) != 0 || (count % 4 != 0))  // sanity checks
  {
    Serial.print ("flash_block align error\n");
    return -1;
  }

  while (count > 0)
  {
    if ((ret = flash_word(address, *bytes)) != 0)
    {
      Serial.printf ("flash_block write error %d\n", ret);
      return -2;
    }
    address += 4;
    ++bytes;
    count -= 4;
  }				// while

  return 0;
}

// ***********************************
// Given an Intel hex format string, write it to upper flash memory (normal location + 128K)
// When finished, move it to lower flash
//
// Note:  hex records must be 32 bit word aligned!
// TODO: use a CRC value instead of line count
static int flash_hex_line (const char *line){
  // hex records info
  unsigned int byte_count;
  static uint32_t address;
  unsigned int code;
  uint8_t data[128];   // assume no hex line will have more than this.  Alignment?

  static uint32_t base_address = 0;
  static int line_count = 0;
  static int error = 0;
  static int done = 0;
  static uint32_t max_address = 0;
  static uint32_t min_address = ~0;

  if (line[0] != ':')		// a non hex line is ignored
    return 0;

  if (error) {      // a single error and nothing more happens
    return -1;
  }

  // check for final flash execute command
  int lines;
  if (sscanf(line, ":flash %d", &lines) == 1) {
    if (lines == line_count && done) {
      Serial.printf("flash %x:%x begins...\n", min_address, max_address);
      delay(100);
      flash_move (min_address, max_address);
      // should never get here
    } else {
      Serial.printf ("bad line count %d vs %d or not done\n", lines, line_count);
      return -7;
    }
  } // if

  ++line_count;

  // must be a hex data line
  if (!parse_hex_line (line, data, &address, &byte_count, &code))
  {
    Serial.printf ("bad hex line %s\n", line);
    error = 1;
    return -1;
  }

  // address sanity check
  if (base_address + address + byte_count > FLASH_SIZE / 2 - RESERVE_FLASH) {
    Serial.print("address too large\n");
    error = 2;
    return -4;
  }

  // process line
  switch (code)
  {
    case 0:             // normal data
      break;
    case 1:             // EOF
      Serial.printf ("done, %d hex lines, address range %x:%x, waiting for :flash %d\n", line_count, min_address, max_address, line_count);
      if (check_compatible(min_address , max_address + FLASH_SIZE / 2))
        done = 1;
      else
        Serial.printf ("new firmware not compatible\n");
      return 0;
    case 2:
      base_address = ((data[0] << 8) | data[1]) << 4;   // extended segment address
      return 0;
    case 4:
      base_address = ((data[0] << 8) | data[1]) << 16;  // extended linear address
      return 0;
    default:
      Serial.printf ("err code = %d, line = %s\n", code, line);
      error = 3;
      return -3;
  }				// switch

  // write hex line to upper flash - note, cast assumes little endian and alignment
  if (flash_block (base_address + address + (FLASH_SIZE / 2), (uint32_t *)data, byte_count))  // offset to upper 128K
  {
    Serial.printf ("can't flash %d bytes to %x\n", byte_count, address);
    error = 4;
    return -2;
  }

  // track size of modifications
  if (base_address + address + byte_count > max_address)
    max_address = base_address + address + byte_count;
  if (base_address + address < min_address)
    min_address = base_address + address;

  return 0;
}				// flash_hex_line()

// ****************************
// check if sector is all 0xFF
static int flash_sector_erased(uint32_t address){
  uint32_t *ptr;

  for (ptr = (uint32_t *)address; ptr < (uint32_t *)(address + FLASH_SECTOR_SIZE); ++ptr) {
    if (*ptr != 0xFFFFFFFF)
      return 0;
  }
  return 1;
} // flash_sector_erased()

// ***************************
// erase the entire upper half
// Note: highest sectors of flash are used for other things - don't erase them
void flash_erase_upper(){
  uint32_t address;
  int ret;

  // erase each block
  for (address = FLASH_SIZE / 2; address < (FLASH_SIZE - RESERVE_FLASH); address += FLASH_SECTOR_SIZE) {
    if (!flash_sector_erased(address)) {
      Serial.printf("\n\nerase sector %x", address);
      Serial.println("");
      if ((ret = flash_erase_sector(address, 0)) != 0)
        Serial.printf("flash erase error %d\n", ret);
    }
  } // for
} // flash_erase_upper()

// *******************************
// Version 1.2
// code to allow firmware update over a Serial port
// for teensy 3.x.  It is intended that this code always be included in your application.
// Jon Zeeff
// This code is in the public domain.  Please retain my name and
// in distributed copies, and let me know about any bugs
// I, Jon Zeeff, give no warranty, expressed or implied for
// this software and/or documentation provided, including, without
// limitation, warranty of merchantability and fitness for a
// particular purpose.
// load new firmware over the Serial port and flash it
// format is an intel hex file followed by ":flash xx" where xx is the number of lines sent
// Max size is 1/2 of flash
// hint:  on Linux, exit the serial console and do "dd if=blink.hex of=/dev/ttyACM0", then restart the Serial console and
// enter the ":flash xxx" command.
void upgrade_firmware(void){
  Serial.printf("%s flash size = %dK in %dK sectors\n\n", FLASH_ID, FLASH_SIZE / 1024, FLASH_SECTOR_SIZE / 1024);

  flash_erase_upper ();   // erase upper half of flash

  if ((uint32_t)flash_word < FLASH_SIZE || (uint32_t)flash_erase_sector < FLASH_SIZE || (uint32_t)flash_move < FLASH_SIZE) {
    Serial.printf("routines not in ram\n");
    return;
  }

  // what is currently used?
  int32_t addr = FLASH_SIZE / 2;
  while (addr > 0 && *((uint32_t *)addr) == 0xFFFFFFFF)
    addr -= 4;
  //Serial.printf("current firmware 0:%x\n", addr + 4);

  if (addr > FLASH_SIZE / 2 - RESERVE_FLASH) {
    Serial.println("firmware is too large\n");
    return;
  }
  
  Serial.println("received firmware update request");

  //Serial.println("WARNING: this can ruin your device");
  Serial.println("waiting for intel hex lines");

  char line[200];
  int c;
  int count = 0;

  // read in hex lines

  int cnt=0;
  for (;;)  {

    while (!Serial.available()) {}
    c = Serial.read();

    if (c == '\n' || c == '\r') {
      line[count] = 0;          // terminate string
      flash_hex_line(line);
      count = 0;
    } else
      line[count++] = c;        // add to string
  } // for ever
}
