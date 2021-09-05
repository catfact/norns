cmake_minimum_required(VERSION 3.16)
project(matron)

set(SRC src/device/device.c
        src/device/device_hid.c
        src/device/device_list.c
        src/device/device_midi.c
        src/device/device_monitor.c
        src/device/device_monome.c
        src/device/device_crow.c
        src/osc.c
        src/hardware/battery.c
        src/hardware/gpio.c
        src/hardware/i2c.c
        src/hardware/platform.c
        src/hardware/screen.c
        src/hardware/stat.c
        src/args.c
        src/events.c
        src/hello.c
        src/input.c
        src/lua_eval.c
        src/main.c
        src/metro.c
        src/oracle.c
        src/weaver.c
        src/shmem.c
        src/snd_file.c
        src/system_cmd.c
        src/clock.c
        src/clocks/clock_internal.c
        src/clocks/clock_midi.c
        src/clocks/clock_crow.c
        src/clocks/clock_scheduler.c
        src/clocks/clock_link.c
        )

add_executable(matron ${SRC})

include_directories(./src
        ./src/device
        ./src/hardware
        ./lua
        ../common
        ../third-party/link_c
        # FIXME :/
        /usr/include/libevdev-1.0
        /usr/include/cairo
        /usr/include/freetype2
        /usr/include/lua5.3
        /usr/include/avahi-compat-libdns_sd
        )


find_library(CAIRO_LIB cairo)
#find_library(CAIRO_FT_LIB cairo-ft)
find_library(LUA_LIB lua5.3)
#find_library(AVAHI_LIB avahi-compat-libdns_sd)
#target_link_libraries(matron PRIVATE ${AVAHI_LIB})

target_link_libraries(matron PRIVATE pthread)
target_link_libraries(matron PRIVATE m)
target_link_libraries(matron PRIVATE asound)
target_link_libraries(matron PRIVATE udev)
target_link_libraries(matron PRIVATE evdev)
target_link_libraries(matron PRIVATE freetype)

#target_link_libraries(matron PRIVATE cairo)
#target_link_libraries(matron PRIVATE cairo-ft)
target_link_libraries(matron PRIVATE ${CAIRO_LIB})
#target_link_libraries(matron PRIVATE ${CAIRO_FT_LIB})
target_link_libraries(matron PRIVATE ${LUA_LIB})
target_link_libraries(matron PRIVATE lo)
target_link_libraries(matron PRIVATE monome)
target_link_libraries(matron PRIVATE sndfile)
#target_link_libraries(matron PRIVATE avahi-compat-libdns_sd)
target_link_libraries(matron PRIVATE dns_sd)
target_link_libraries(matron PRIVATE jack)
target_link_libraries(matron PRIVATE link)


#
#matron_sources += ['src/clocks/clock_link.c']
#matron_includes += ['../third-party/link-c']
#matron_libs += ['stdc++']
#matron_use += ['LIBLINK_C']