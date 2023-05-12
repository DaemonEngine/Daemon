#include "common/Common.h"



#ifdef _MSC_VER
#pragma warning(push, 0) // Disable warnings
#endif

// Make sure these includes are spelled the same as when the libraries are used elsewhere,
// so that we don't get errors due to this file. And so that if there are multiple versions
// of the header installed, we get the one we are using...
#include <zlib.h>
#if defined(BUILD_SERVER) || defined(BUILD_GRAPHICAL_CLIENT) || defined(BUILD_TTY_CLIENT)
#include <nettle/version.h>
#include <gmp.h>
#endif
#if defined(BUILD_GRAPHICAL_CLIENT) || defined(BUILD_TTY_CLIENT)
#include <curl/curl.h>
#endif
#if defined(BUILD_GRAPHICAL_CLIENT) || defined(_WIN32)
#include <SDL.h>
#endif
#ifdef BUILD_GRAPHICAL_CLIENT
#include <al.h>
#include <GL/glew.h>
#include <gmp.h>
#include <jpeglib.h>
#include <opusfile.h>
#include <png.h>
#include <webp/decode.h>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Print versions of libraries which we don't build from source
class PrintDepVersionsCommand : public Cmd::StaticCmd
{
public:
    PrintDepVersionsCommand() : StaticCmd(
        "depversions", Cmd::BASE, "show versions of external library dependencies") {}

    void Run(const Cmd::Args&) const override {
        Print("zlib: header %s, binary %s", ZLIB_VERSION, zlibVersion());
        // TODO: ncursesw for *nix

#if defined(BUILD_SERVER) || defined(BUILD_GRAPHICAL_CLIENT) || defined(BUILD_TTY_CLIENT)
        Print("nettle: header %d.%d, binary %d.%d",
            NETTLE_VERSION_MAJOR, NETTLE_VERSION_MINOR,
            nettle_version_major(), nettle_version_minor());
        Print("gmp: header %d.%d.%d, binary %s",
            __GNU_MP_VERSION, __GNU_MP_VERSION_MINOR, __GNU_MP_VERSION_PATCHLEVEL,
            gmp_version);
#endif

#if defined(BUILD_GRAPHICAL_CLIENT) || defined(BUILD_TTY_CLIENT)
        Print("curl: header %d.%d.%d, binary %s",
            LIBCURL_VERSION_MAJOR, LIBCURL_VERSION_MINOR, LIBCURL_VERSION_PATCH,
            curl_version());
#endif

#if defined(BUILD_GRAPHICAL_CLIENT) || defined(_WIN32)
        {
            SDL_version header, binary;
            SDL_VERSION(&header);
            SDL_GetVersion(&binary);
            Print("sdl: header %d.%d.%d binary %d.%d.%d",
                header.major, header.minor, header.patch,
                binary.major, binary.minor, binary.patch);
        }
#endif

#ifdef BUILD_GRAPHICAL_CLIENT
        Print("glew: header %d.%d.%d",
            GLEW_VERSION_MAJOR, GLEW_VERSION_MINOR, GLEW_VERSION_MICRO);
        Print("opus: binary %s", opus_get_version_string());
#ifdef LIBJPEG_TURBO_VERSION
        Print("jpeg-turbo: header %s", XSTRING(LIBJPEG_TURBO_VERSION));
#else
        // not sure if this is possible
        Print("jpeg: unknown (non-turbo header)");
#endif
        Print("png: header %s", PNG_LIBPNG_VER_STRING);
        {
            int binary = WebPGetDecoderVersion();
            Print("webp (decoder): binary %d.%d.%d",
                binary >> 16, (binary >> 8) & 0xFF, binary & 0xFF);
        }
        {
            std::string vendor = alGetString(AL_VENDOR);
            std::string version = alGetString(AL_VERSION);
            Print("al: binary: vendor '%s' version '%s'", vendor, version);
        }
        // couldn't find anything for ogg/vorbis
        // TODO freetype
#endif
    }
};
static PrintDepVersionsCommand cmd;
