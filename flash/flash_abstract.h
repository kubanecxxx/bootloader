#ifndef FLASH_ABSTRACT_H
#define FLASH_ABSTRACT_H

typedef enum
{FLASH_OK, FLASH_ERROR}
flash_error_t;

flash_error_t flash_erase(uint32_t start, uint32_t size);
flash_error_t flash_write(uint32_t start, void * data , uint32_t data_size);

#endif // FLASH_ABSTRACT_H
