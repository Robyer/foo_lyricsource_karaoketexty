#define _VER "0.1"

#if _M_IX86_FP > 0
#define _VERSION _VER " (SSE" TOSTRING(_M_IX86_FP) ")"
#elif _M_IX86_FP == 1
#define _VERSION _VER " (SSE)"
#else
#define _VERSION _VER
#endif

#define VERSION _VERSION

#define COMPONENT_NAME "Lyric Source - Karaoketexty.cz"

#define ABOUT "foo_lyricsource_karaoketexty " VERSION "\n" \
 "Lyrics source support for website www.karaoketexty.cz with synced and unsynced lyrics.\n\n" \
 "Author: Robyer\n" \
 "Web: http://www.robyer.cz\n" \
 "E-mail: robyer@seznam.cz\n" \
 "Build Date: " __DATE__ " " __TIME__ "\n"