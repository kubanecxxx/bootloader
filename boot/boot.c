#include "ch.h"
#include "hal.h"
#include "boot.h"
#include <string.h>
#include "flash_abstract.h"

#include "esp8266.h"
#include "crc16.h"

static char buffer[512];
static void boot(const char * raw, uint8_t len, uint8_t tcp_id);

static uint16_t speed = 200;
static int16_t stren = 0xff;

extern void watchdog_reset(void);

#ifdef STM32F4xx_MCUCONF
//arbitrary code is different in each application

static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palSetPad(GPIOD, GPIOD_LED3);       /* Orange.  */
    chThdSleepMilliseconds(speed * 1 / 8);
    palClearPad(GPIOD, GPIOD_LED3);     /* Orange.  */
    chThdSleepMilliseconds(speed * 7 / 4);
  }
}
#endif

static const esp_config_t esp =
{
    &SD2, watchdog_reset, "OpenWrt_Kuba2", "jakypakheslo",120
};

void bootTask(void)
{
    static uint8_t ok;
    static uint8_t first = 1;
    //first run
    if (first)
    {        
        chThdCreateStatic(waThread1,sizeof(waThread1), HIGHPRIO, Thread1,NULL);
        esp_set_sd(&esp);
        first = 0;
    }


    if (!ok)
        speed= 200; //connecting
    ok = esp_keep_connected_loop( 8080, 0);

    uint16_t len;
    uint8_t id;
    if (ok)
    {
        speed = 1000; //ready
        //everything is set properly
        len = esp_decode_ipd(buffer, &id);

        if (len)
        {
            boot(buffer, len,id);
            esp_basic_commands(buffer,len,id);
        }
    }
    else
    {
        speed = 50;     //disconnected
    }
}



void boot(const char * raw, uint8_t len, uint8_t id)
{
    //command sets
    //uint8_t 01; erase program size will be uint32_t
    //uint8_t 02; uint16_t segment number uint16_t segment size bytes; uint16_t crc; raw data
    //uint8_t 03; last segment - start user program
    //uint8_t 'i'; info where should user program start ; no args

    //responses
    //uint8_t 81: ok send next
    //uint8_t 82: segment has errors


    uint8_t i;
    const char * p;
    static uint8_t flash[1030];
    static uint16_t  end;
    static uint32_t offset;
    char buf[10];

    if (raw[0] == '1')
    {
        if (strlen(raw) < 3)
            return;
        uint16_t size;
        size = atoiw(raw+1);

        flash_error_t e;
        e = flash_erase(USER_PROGRAM_START_ADDRESS,size);
        if (e == FLASH_ERROR)
        {
            //fail
            chSysHalt("flash_error");
        }

        end = 0;
        offset = 0;
        esp_write_tcp_char(0x81,id);
    }
    else if (raw[0] == '2')
    {
        uint16_t size, crc, part,parts;
        flash_error_t e;
        p = raw;
        p++;
        part = atoiw(p);
        p+=4;
        size = atoiw(p);
        p+=4;
        crc = atoiw(p);
        p+=4;
        parts = atoiw(p);
        p+=4;

        for(i = 0 ; i < size ; i++)
        {
            flash[i+end] = atoi(p);
            p+=2;
        }

        uint16_t last_end = end;
        uint8_t * pt = &flash[end];
        flash[i + end] = crc >> 8;
        flash[i + end + 1] = crc & 0xff;

        crc = crc16_ccitt(pt,size + 2);

        end += i;

        //buffer full or last segment
        if ((end == 1024 || part + 1 == parts) && !crc)
        {
            //write to flash
            e = flash_write(USER_PROGRAM_START_ADDRESS + offset,flash,end);
            if (e == FLASH_ERROR)
            {
                //fail
                chSysHalt("flash_error");
            }
            offset += end;
            end = 0;
        }

        if (!crc)
        {
            esp_write_tcp_char(0x81,id);
        }
        else
        {
            end = last_end;
            esp_write_tcp_char(0x82,id);
        }

    }
    else if (raw[0] == '3' )
    {
        if(boot_is_user_program_ready())
        {
            esp_write_tcp_char('3',id);
            bootJumpToUser();
        }
        else
        {
            esp_write_tcp_char('4',id);
        }

        asm("nop");
    }
    else if (raw[0] == 'i')
    {
        buf[0] = '0';
        buf[1] = 'x';
        itoa(buf+2,USER_PROGRAM_START_ADDRESS,4,16);
        esp_write_tcp(buf,strlen(buf),id);
    }

}

#define BOOT_GET_MEMORY_BEFORE_ENTRY(start, offset) \
    (*(uint32_t*)((entry_point(start)-1) - offset))

#define B_MAGIC_WORD1(start) BOOT_GET_MEMORY_BEFORE_ENTRY(start,8)
#define B_MAGIC_WORD2(start) BOOT_GET_MEMORY_BEFORE_ENTRY(start,4)

#define B_MAGIC_WORD1_BOOT() B_MAGIC_WORD1(BOOTLOADER_START_ADDRESS)
#define B_MAGIC_WORD2_BOOT() B_MAGIC_WORD2(BOOTLOADER_START_ADDRESS)

#define B_MAGIC_WORD1_USER() B_MAGIC_WORD1(USER_PROGRAM_START_ADDRESS)
#define B_MAGIC_WORD2_USER() B_MAGIC_WORD2(USER_PROGRAM_START_ADDRESS)

uint8_t boot_is_user_program_ready(void)
{
    //get vector table
    //validate vector table - reset handler must be few bytes after table
    //get reset handler
    //get magic words before reset handler

    //if reset handler points somewhere on start then its ok
    uint32_t rh = *(uint32_t*)(USER_PROGRAM_START_ADDRESS +4);
    if (rh  > USER_PROGRAM_START_ADDRESS + 1024 || rh < USER_PROGRAM_START_ADDRESS)
    {
        return 0;
    }


    uint32_t b1,b2,u1,u2;

    b1 = B_MAGIC_WORD1_BOOT();
    b2 = B_MAGIC_WORD2_BOOT();
    u1 = B_MAGIC_WORD1_USER();
    u2 = B_MAGIC_WORD2_USER();

    return (b1 == u1 && b2 == u2);
}

