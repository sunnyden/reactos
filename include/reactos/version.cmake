macro(today RESULT)
    if(CMAKE_HOST_WIN32)
        execute_process(COMMAND wmic os get LocalDateTime OUTPUT_VARIABLE RAWDATE)
        string(REPLACE "\n" ";" RAWDATE ${RAWDATE})
        LIST(GET RAWDATE 1 RAWDATE)
        string(SUBSTRING ${RAWDATE} 0 8 ${RESULT})
    elseif(CMAKE_HOST_UNIX)
        execute_process(COMMAND "date" "+%Y%m%d" OUTPUT_VARIABLE ${RESULT})
        string(STRIP ${${RESULT}} ${RESULT})
    else()
        message(SEND_ERROR "date not implemented")
        set(${RESULT} 00000000)
    endif()
endmacro()

macro(inttohex INTVALUE OUTPUT_VARIABLE)
    list(APPEND HEXLIST 0 1 2 3 4 5 6 7 8 9 a b c d e f)
    list(GET HEXLIST ${INTVALUE} ${OUTPUT_VARIABLE})
endmacro()

macro(converttohex INTVALUE OUTPUT_VARIABLE)
    set(${OUTPUT_VARIABLE} "")
    set(REMAINING ${INTVALUE})
    while(REMAINING)
        math(EXPR REMAINDER "${REMAINING}%16")
        inttohex(${REMAINDER} HEXCHAR)
        math(EXPR REMAINING "${REMAINING}/16")
        set(${OUTPUT_VARIABLE} "${HEXCHAR}${${OUTPUT_VARIABLE}}")
    endwhile()
endmacro()

today(KERNEL_VERSION_BUILD)

set(KERNEL_VERSION_MAJOR "0")
set(KERNEL_VERSION_MINOR "4")
set(KERNEL_VERSION_PATCH_LEVEL "0")
set(COPYRIGHT_YEAR "2015")
# KERNEL_VERSION_BUILD_TYPE is "SVN", "RC1", "RC2" or "" (for the release)
set(KERNEL_VERSION_BUILD_TYPE "SVN")


set(KERNEL_VERSION "${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}")
if(KERNEL_VERSION_PATCH_LEVEL)
    set(KERNEL_VERSION "${KERNEL_VERSION}.${KERNEL_VERSION_PATCH_LEVEL}-${KERNEL_VERSION_BUILD_TYPE}")
else()
    set(KERNEL_VERSION "${KERNEL_VERSION}-${KERNEL_VERSION_BUILD_TYPE}")
endif()

math(EXPR REACTOS_DLL_VERSION_MAJOR "${KERNEL_VERSION_MAJOR}+42")
set(DLL_VERSION_STR "${REACTOS_DLL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}-${KERNEL_VERSION_BUILD_TYPE}")

# get svn revision number
set(REVISION "rUNKNOWN")
set(KERNEL_VERSION_BUILD_HEX "0")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
    find_package(Subversion)
    if(Subversion_FOUND)
        subversion_wc_info(${CMAKE_CURRENT_SOURCE_DIR} SVNINFO)
        if(SVNINFO_WC_REVISION)
            set(REVISION "r${SVNINFO_WC_REVISION}")
            converttohex(${SVNINFO_WC_REVISION} KERNEL_VERSION_BUILD_HEX)
        endif()
    endif()
endif()

configure_file(include/reactos/version.h.cmake ${REACTOS_BINARY_DIR}/include/reactos/version.h)
configure_file(include/reactos/buildno.h.cmake ${REACTOS_BINARY_DIR}/include/reactos/buildno.h)
configure_file(include/reactos/builddir.h.cmake ${REACTOS_BINARY_DIR}/include/reactos/builddir.h)
