#ifndef BOOT_H
#define BOOT_H

//instructions on start
#define MAGIC_NUMBER 0x54879653

extern uint32_t __ram0_end__;
#define RAM_END ((uint32_t)(&__ram0_end__))

#define BOOT_RESET_OFFSET   9
#define BOOT_CLEAR_RESET() (*((uint32_t *)RAM_END - BOOT_RESET_OFFSET) = 0)
#define BOOT_INC_RESET() ((*((uint32_t *)RAM_END - BOOT_RESET_OFFSET) += 1))
#define BOOT_GET_RESET() (*((uint32_t *)RAM_END - BOOT_RESET_OFFSET))
#define IS_MAGIC_NUMBER() (*((uint32_t *)RAM_END - 5) == MAGIC_NUMBER)
#define WRITE_TO_RAM_END(num_u32) (*((uint32_t *)RAM_END - 5) = num_u32)

//#pragma message ("Ram end RAM_END")

//find reset handler in right vector table and jump there
#define entry_point(start) \
    (*(uint32_t*)(start + 4 ))

#define bootJumpToUser() \
    asm("cpsid i"); \
    asm("cpsid f"); \
    WRITE_TO_RAM_END(0); \
    ((void (*) (void)) (entry_point(USER_PROGRAM_START_ADDRESS)) )();

#define bootJumpToBootloader() \
    asm("cpsid i"); \
    asm("cpsid f"); \
    WRITE_TO_RAM_END(MAGIC_NUMBER); \
    ((void (*) (void)) (entry_point(BOOTLOADER_START_ADDRESS)) )();

//uart and pins must be already configured
void bootTask(void);

uint8_t boot_is_user_program_ready(void);

#endif // BOOT_H
