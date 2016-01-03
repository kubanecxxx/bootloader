/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"
#include "boot.h"

/*
 * Application entry point.
 */

const SerialConfig cfg =
{
115200,0,0,0
};

void setup_watchdog(void)
{
    IWDG->KR = 0x5555;

    //watchdog timeout is about 2 seconds
    IWDG->PR = 2;
    IWDG->RLR = 0xfff;
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;


    IWDG->KR = 0xCCCC;
}

void watchdog_reset(void)
{
    IWDG->KR = 0xAAAA;
}


int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */

    // and now we can finally start with bootloader itself bitches!!!

    //decide if start user program or bootloader
    // if user memory is empty start bootloader
    //but what if i want to start the bootloader?
    //if there is some magic numbers in the end of ram

    //(re)start watchdog here
    //user program will have to reload it periodically
    //if reset occured because of watchdog few times in row jump into bootloader
    setup_watchdog();

    //if user program watchdog failed
    if (RCC->CSR & RCC_CSR_WDGRSTF)
    {
        BOOT_INC_RESET();
    }
    else
    {
        BOOT_CLEAR_RESET();
    }

    RCC->CSR |= RCC_CSR_RMVF;
    uint32_t reset = BOOT_GET_RESET();


    //magic words in bootloader and user program must be the same
    //if watchdog reset occured 10 times in short time skip user program
    if (boot_is_user_program_ready() && !IS_MAGIC_NUMBER() && reset < 10)
    {
        bootJumpToUser();
    }


    BOOT_CLEAR_RESET();

    halInit();
    chSysInit();
    SerialDriver * uart;

#ifdef STM32F4xx_MCUCONF
    /*
     * Activates the serial driver 2 using the driver default configuration.
     * PA2(TX) and PA3(RX) are routed to USART2.
     */
    uart = &SD2;
    sdStart(uart, &cfg);
    palSetPadMode(GPIOA, 2, PAL_MODE_ALTERNATE(7));
    palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(7));
#endif



  while (true)
  {
        bootTask();
        watchdog_reset();

  }
}
