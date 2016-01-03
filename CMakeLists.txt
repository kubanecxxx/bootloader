cmake_minimum_required(VERSION 3.4)

project(bootloader)

set (CHIBIOS_DIR "/home/kubanec/workspaces/chibios_3stable")

#todo
#cmakefiles for my modules


set (USER_SOURCES 
main.c
boot/boot.c

#select according current MCU
flash/stm32f4xx_flash.c

)

set (EXTRA_HEADERS
halconf.h
chconf.h
mcuconf.h

boot/boot.h

flash/flash_abstract.h
)

set (OPTIMIZE_RTOS 1)

set (OPTIMIZED_SOURCES

)

include_directories(
)

set (INCLUDE_DIRS
    boot
    flash

)

#set (LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32F100xB.ld")

set (GLOBAL_FLAGS
"-O0  -ggdb "
)

set (OPTIMIZED_FLAGS
"-Os"
)


#link binary start address after bootloader part
set (USE_BOOTLOADER 1)
set (BOOTLOADER_ITSELF 1)

#family or chip
#set (CHIP "STM32F100C8")
set (CHIP "STM32F407VG")
# chibiOS stable v3.0.x branch commit ed8937f718ace39051768bb45c66a195b48270b3

if (NOT EXISTS ${CHIBIOS_DIR})
	set (CHIBIOS_DIR "/home/kuba/chibi/ChibiOS")
endif()
set (BOARD "ST_STM32F4_DISCOVERY")

set (FLASH_SIZE "1M")
set (RAM_SIZE "128k")

include (${CHIBIOS_DIR}/CMakeLists.txt)

#add_subdirectory(${PROJECT_DIRECTORY}/piris/framework ${CMAKE_CURRENT_BINARY_DIR}/piris)
add_subdirectory("../library" "${CMAKE_CURRENT_BINARY_DIR}/library")
target_link_libraries(${PROJECT_NAME} library)

message("")
