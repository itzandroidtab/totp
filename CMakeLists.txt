# set the sources
set(SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/button.cpp

    # example for including the startup.cpp
    ${CMAKE_SOURCE_DIR}/targets/${TARGET_CPU}/startup.cpp
)

set(HEADERS)

# add our executable
add_executable(klib_project
    ${SOURCES} ${HEADERS}
)

# enable LTO
target_compile_options(klib PUBLIC "-flto")
target_compile_options(klib PUBLIC "-fuse-linker-plugin")
target_link_options(klib PUBLIC "-flto")
target_link_options(klib PUBLIC "-fuse-linker-plugin")

# set the output filename
set_target_properties(klib_project PROPERTIES OUTPUT_NAME "klib" SUFFIX ".elf")

# set the interrupt implementation
target_compile_definitions(klib PUBLIC "KLIB_IRQ=irq_ram")
# target_compile_definitions(klib PUBLIC "KLIB_IRQ=irq_hooked")
# target_compile_definitions(klib PUBLIC "KLIB_IRQ=irq_flash")

# set the default cout/cin
target_compile_definitions(klib PUBLIC "KLIB_DEFAULT_COUT=rtt")
target_compile_definitions(klib PUBLIC "KLIB_DEFAULT_CIN=rtt")

# override the rtt buffer size (not required and not used when no segger rtt is used)
target_compile_definitions(klib PUBLIC "BUFFER_SIZE_UP=512")
target_compile_definitions(klib PUBLIC "BUFFER_SIZE_DOWN=16")

# set if we should support certain callbacks
target_compile_definitions(klib PUBLIC "SYSTICK_CALLBACK_ENABLED=false")

# link the klib_project to klib and the cpu target
target_link_libraries(klib_project PUBLIC klib)
target_link_libraries(klib_project PUBLIC target_cpu)

# Libraries to link for all targets
target_link_libraries(klib_project PUBLIC m)

set(TARGET_LINKERSCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linkerscript.ld)

# link to the linkerscript of the target cpu
target_link_options(klib_project PUBLIC "-T${TARGET_LINKERSCRIPT}")
set_target_properties(klib_project PROPERTIES LINK_DEPENDS ${TARGET_LINKERSCRIPT})

# add the project directory to the include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}) 

# Custom commands for processing the build binary and show some statistics and debug info 
add_custom_command(TARGET klib_project DEPENDS ${CMAKE_BINARY_DIR}/klib.elf POST_BUILD COMMAND arm-none-eabi-objcopy ARGS -O binary -R .bss -R .stack klib.elf klib.bin)
add_custom_command(TARGET klib_project DEPENDS ${CMAKE_BINARY_DIR}/klib.elf POST_BUILD COMMAND arm-none-eabi-objcopy ARGS -O ihex -R .bss -R .stack klib.elf klib.hex)
add_custom_command(TARGET klib_project DEPENDS ${CMAKE_BINARY_DIR}/klib.elf POST_BUILD COMMAND arm-none-eabi-objdump ARGS -C -S klib.elf > klib.lss)
add_custom_command(TARGET klib_project DEPENDS ${CMAKE_BINARY_DIR}/klib.elf POST_BUILD COMMAND arm-none-eabi-objdump ARGS -C -sj .bss -sj .data -sj .rodata -sj .vectors -S klib.elf > klib.memory)
add_custom_command(TARGET klib_project DEPENDS ${CMAKE_BINARY_DIR}/klib.elf POST_BUILD COMMAND arm-none-eabi-size ARGS -A klib.elf -x)
