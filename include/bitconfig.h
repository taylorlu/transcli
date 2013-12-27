#ifndef __BIT_CONFIG_H__
#define __BIT_CONFIG_H__

#define TS_MAJOR_VERSION "2.3"
#define TS_MINOR_VERSION "0"
/* =========Version Change Records==============
  ------Version 2.3.0.62000 change log------------:
  1. Add x265 encoder, support HEVC encoding.
  2. Fix timeout issue.
  3. Fix invalide audio/video track in source file.

  ------Version 2.2.0.60800 change log------------:
  1. Move clip trim to ffmpeg decoder(use trim filter), more accurate
  2. Add discard_first_not_key command to ffmpeg to discard bad frames before first key frame
  3. Change aresample commandline to ensure a/v synth when audio timestamp is far more behind video

  ------Version 2.1.1.59400 change log------------:
  1. Update x264 rate control algorithm, raise video base bitrate, allow complex video cosume more bitrate
  2. Fix remux function.
  3. Video base bitrate: BD: 3M, CQ: 1.5M, HD: 800k, SD: 500k
  ------Minor version 1 main change(2012-05-21)---------:
  1. Update x264 to r124
  2. Add intelligent video enhancer
  3. Add MinQp bitrate control
  ------Major version 2.0->2.1 main change(2012-11-08)---------:
  1. Update x264 to r125
  2. Use TargetQp bitrate control method(better than MinQp, save more bitrate)
  3. Use FFMpeg decoding all files except DVD/VCD
  4. Use FFProbe to parse media information instead of Mediainfo
  5. Use FFMpeg to auto dectect black band, remove Mplayer
*/

//#define MEDIACODERNT_EVAL_PPLIVE

#define HAVE_GPL
#define DEBUG_EXTERNAL_CMD 1

//////////////////////////////////////////////////////////////////////////////
#if defined(HAVE_GPL)
#define HAVE_LIBX264 1
#define HAVE_LIBX265 1
#define HAVE_LIBFAAC 1
#define HAVE_LIBEAC3 1
#else
#undef HAVE_LIBX264
#undef HAVE_LIBX265
#undef HAVE_LIBFAAC
#endif

#endif
