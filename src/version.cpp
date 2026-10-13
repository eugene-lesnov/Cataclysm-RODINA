#include "get_version.h" // IWYU pragma: associated

#if (defined(_WIN32) || defined(MINGW)) && !defined(GIT_VERSION) && !defined(CROSS_LINUX) && !defined(_MSC_VER)

#ifndef VERSION
#define VERSION "unknown"
#endif

#else

#include "version.h"

#endif

#ifndef RODINA_VERSION
#define RODINA_VERSION "0.1-dev"
#endif

const char *getRodinaVersion()
{
    return RODINA_VERSION;
}

const char *getVersionString()
{
    return "RODINA " RODINA_VERSION " (based on CDDA 0.I-1, commit " VERSION ")";
}
