if(GIT_BINARY)
    set(GIT_EXECUTABLE "${GIT_BINARY}")
else()
    find_program(GIT_EXECUTABLE git)
endif()

set(GIT_VERSION "unknown")
set(_sha1 "unknown")
if(GIT_EXECUTABLE)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _git_result
        OUTPUT_VARIABLE _short_sha
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(_git_result EQUAL 0)
        set(GIT_VERSION "${_short_sha}")
        execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE _sha1
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND "${GIT_EXECUTABLE}" diff --quiet HEAD -- .
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE _dirty)
        if(_dirty)
            string(APPEND GIT_VERSION "-dirty")
        endif()
    endif()
endif()

set(_version_header "// NOLINT(cata-header-guard)\n#define VERSION \"${GIT_VERSION}\"\n")
set(_existing_header "")
if(EXISTS "${CMAKE_SOURCE_DIR}/src/version.h")
    file(READ "${CMAKE_SOURCE_DIR}/src/version.h" _existing_header)
endif()
if(NOT "${_version_header}" STREQUAL "${_existing_header}")
    file(WRITE "${CMAKE_SOURCE_DIR}/src/version.h" "${_version_header}")
endif()

file(WRITE "${CMAKE_SOURCE_DIR}/VERSION.txt" "product: Cataclysm: RODINA\nrodina version: 0.1-dev\nupstream version: CDDA 0.I-1\ncommit sha: ${_sha1}\ncommit url: https://github.com/eugene-lesnov/Cataclysm-RODINA/commit/${_sha1}\n")
message(NOTICE "${GIT_VERSION}")
