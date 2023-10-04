/*****************************************************************************
 * chroma.c: libavutil <-> libvlc conversion routines
 *****************************************************************************
 * Copyright (C) 1999-2008 VLC authors and VideoLAN
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Gildas Bazin <gbazin@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 021100301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <vlc_common.h>
#include <vlc_codec.h>

/*
 * Common header for swscale, avcodec (and avformat).
 * Only libavutil can be depended on here.
 */
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include "chroma.h"

/*****************************************************************************
 * Chroma fourcc -> libavutil pixfmt mapping
 *****************************************************************************/
#if defined(WORDS_BIGENDIAN)
#   define VLC_RGB_ES( fcc, leid, beid ) \
    { fcc, beid },
#else
#   define VLC_RGB_ES( fcc, leid, beid ) \
    { fcc, leid },
#endif

#define VLC_RGB( fcc, leid, beid, rmask, gmask, bmask ) \
    { fcc, leid, rmask, gmask, bmask }, \
    { fcc, beid, bmask, gmask, rmask }, \
    VLC_RGB_ES( fcc, leid, beid )


static const struct
{
    vlc_fourcc_t  i_chroma;
    enum AVPixelFormat i_chroma_id;
    video_color_range_t range;

} chroma_table[] =
{
    /* Planar YUV formats */
    {VLC_CODEC_I444, AV_PIX_FMT_YUV444P,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I444, AV_PIX_FMT_YUVJ444P,  COLOR_RANGE_FULL },

    {VLC_CODEC_I440, AV_PIX_FMT_YUV440P,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I440, AV_PIX_FMT_YUVJ440P,  COLOR_RANGE_FULL },

    {VLC_CODEC_I422, AV_PIX_FMT_YUV422P,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I422, AV_PIX_FMT_YUVJ422P,  COLOR_RANGE_FULL },

    {VLC_CODEC_I420, AV_PIX_FMT_YUV420P,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_YV12, AV_PIX_FMT_YUV420P,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I420, AV_PIX_FMT_YUVJ420P,  COLOR_RANGE_FULL },
    {VLC_CODEC_I411, AV_PIX_FMT_YUV411P,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I411, AV_PIX_FMT_YUVJ411P,  COLOR_RANGE_FULL },
    {VLC_CODEC_I410, AV_PIX_FMT_YUV410P,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_YV9, AV_PIX_FMT_YUV410P,  COLOR_RANGE_UNDEF },

    {VLC_CODEC_NV12, AV_PIX_FMT_NV12,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_NV21, AV_PIX_FMT_NV21,  COLOR_RANGE_UNDEF },

    {VLC_CODEC_I420_9L, AV_PIX_FMT_YUV420P9LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I420_9B, AV_PIX_FMT_YUV420P9BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I420_10L, AV_PIX_FMT_YUV420P10LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I420_10B, AV_PIX_FMT_YUV420P10BE,  COLOR_RANGE_UNDEF },
#ifdef AV_PIX_FMT_YUV420P12 /* 54, 17, 100 */
    {VLC_CODEC_I420_12L, AV_PIX_FMT_YUV420P12LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I420_12B, AV_PIX_FMT_YUV420P12BE,  COLOR_RANGE_UNDEF },
#endif
    {VLC_CODEC_I420_16L, AV_PIX_FMT_YUV420P16LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I420_16B, AV_PIX_FMT_YUV420P16BE,  COLOR_RANGE_UNDEF },
#ifdef AV_PIX_FMT_P010LE
    {VLC_CODEC_P010, AV_PIX_FMT_P010LE,  COLOR_RANGE_UNDEF },
#endif
#ifdef AV_PIX_FMT_P016LE
    {VLC_CODEC_P016, AV_PIX_FMT_P016LE,  COLOR_RANGE_UNDEF },
#endif

    {VLC_CODEC_I422_9L, AV_PIX_FMT_YUV422P9LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I422_9B, AV_PIX_FMT_YUV422P9BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I422_10L, AV_PIX_FMT_YUV422P10LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I422_10B, AV_PIX_FMT_YUV422P10BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I422_16L, AV_PIX_FMT_YUV422P16LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I422_16B, AV_PIX_FMT_YUV422P16BE,  COLOR_RANGE_UNDEF },
#ifdef AV_PIX_FMT_YUV422P12 /* 54, 17, 100 */
    {VLC_CODEC_I422_12L, AV_PIX_FMT_YUV422P12LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I422_12B, AV_PIX_FMT_YUV422P12BE,  COLOR_RANGE_UNDEF },
#endif

    {VLC_CODEC_YUV420A, AV_PIX_FMT_YUVA420P,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_YUV422A, AV_PIX_FMT_YUVA422P,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_YUVA,    AV_PIX_FMT_YUVA444P,  COLOR_RANGE_UNDEF },

    {VLC_CODEC_YUVA_444_10L, AV_PIX_FMT_YUVA444P10LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_YUVA_444_10B, AV_PIX_FMT_YUVA444P10BE,  COLOR_RANGE_UNDEF },

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 24, 101)
    {VLC_CODEC_YUVA_444_12L, AV_PIX_FMT_YUVA444P12LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_YUVA_444_12B, AV_PIX_FMT_YUVA444P12BE,  COLOR_RANGE_UNDEF },
#endif

    {VLC_CODEC_I444_9L, AV_PIX_FMT_YUV444P9LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I444_9B, AV_PIX_FMT_YUV444P9BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I444_10L, AV_PIX_FMT_YUV444P10LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I444_10B, AV_PIX_FMT_YUV444P10BE,  COLOR_RANGE_UNDEF },
#ifdef AV_PIX_FMT_YUV444P12 /* 54, 17, 100 */
    {VLC_CODEC_I444_12L, AV_PIX_FMT_YUV444P12LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I444_12B, AV_PIX_FMT_YUV444P12BE,  COLOR_RANGE_UNDEF },
#endif
    {VLC_CODEC_I444_16L, AV_PIX_FMT_YUV444P16LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_I444_16B, AV_PIX_FMT_YUV444P16BE,  COLOR_RANGE_UNDEF },

    /* Packed YUV formats */
    {VLC_CODEC_YUYV, AV_PIX_FMT_YUYV422,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_UYVY, AV_PIX_FMT_UYVY422,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_YVYU, AV_PIX_FMT_YVYU422,  COLOR_RANGE_UNDEF },

    /* Packed RGB formats */
    {VLC_CODEC_RGB233, AV_PIX_FMT_RGB8,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_BGR233, AV_PIX_FMT_BGR8,  COLOR_RANGE_UNDEF },

    {VLC_CODEC_RGB565BE, AV_PIX_FMT_RGB565BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_RGB565LE, AV_PIX_FMT_RGB565LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_BGR565BE, AV_PIX_FMT_BGR565BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_BGR565LE, AV_PIX_FMT_BGR565LE,  COLOR_RANGE_UNDEF },

    {VLC_CODEC_RGB555BE, AV_PIX_FMT_RGB555BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_RGB555LE, AV_PIX_FMT_RGB555LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_BGR555BE, AV_PIX_FMT_BGR555BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_BGR555LE, AV_PIX_FMT_BGR555LE,  COLOR_RANGE_UNDEF },

    {VLC_CODEC_RGB24, AV_PIX_FMT_RGB24,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_BGR24, AV_PIX_FMT_BGR24,  COLOR_RANGE_UNDEF },

    {VLC_CODEC_RGBX, AV_PIX_FMT_RGB0,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_XRGB, AV_PIX_FMT_0RGB,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_BGRX, AV_PIX_FMT_BGR0,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_XBGR, AV_PIX_FMT_0BGR,  COLOR_RANGE_UNDEF },

    {VLC_CODEC_RGBA, AV_PIX_FMT_RGBA,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_ARGB, AV_PIX_FMT_ARGB,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_BGRA, AV_PIX_FMT_BGRA,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_ABGR, AV_PIX_FMT_ABGR,  COLOR_RANGE_UNDEF },

#ifdef WORDS_BIGENDIAN
    {VLC_CODEC_RGBA64, AV_PIX_FMT_RGBA64BE,  COLOR_RANGE_UNDEF },
#else /* !WORDS_BIGENDIAN */
    {VLC_CODEC_RGBA64, AV_PIX_FMT_RGBA64LE,  COLOR_RANGE_UNDEF },
#endif /* !WORDS_BIGENDIAN */

#ifdef AV_PIX_FMT_X2BGR10
    {VLC_CODEC_RGBA10, AV_PIX_FMT_X2BGR10,  COLOR_RANGE_UNDEF },
#endif

    {VLC_CODEC_GREY, AV_PIX_FMT_GRAY8,  COLOR_RANGE_UNDEF},
#ifdef AV_PIX_FMT_GRAY10
    {VLC_CODEC_GREY_10L, AV_PIX_FMT_GRAY10LE,  COLOR_RANGE_UNDEF},
    {VLC_CODEC_GREY_10B, AV_PIX_FMT_GRAY10BE,  COLOR_RANGE_UNDEF},
#endif
#ifdef AV_PIX_FMT_GRAY12
    {VLC_CODEC_GREY_12L, AV_PIX_FMT_GRAY12LE,  COLOR_RANGE_UNDEF},
    {VLC_CODEC_GREY_12B, AV_PIX_FMT_GRAY12BE,  COLOR_RANGE_UNDEF},
#endif
    {VLC_CODEC_GREY_16L, AV_PIX_FMT_GRAY16LE,  COLOR_RANGE_UNDEF},
    {VLC_CODEC_GREY_16B, AV_PIX_FMT_GRAY16BE,  COLOR_RANGE_UNDEF},

     /* Paletized RGB */
    {VLC_CODEC_RGBP, AV_PIX_FMT_PAL8,  COLOR_RANGE_UNDEF},

    {VLC_CODEC_GBR_PLANAR, AV_PIX_FMT_GBRP,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBR_PLANAR_9L, AV_PIX_FMT_GBRP9LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBR_PLANAR_9B, AV_PIX_FMT_GBRP9BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBR_PLANAR_10L, AV_PIX_FMT_GBRP10LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBR_PLANAR_10B, AV_PIX_FMT_GBRP10BE,  COLOR_RANGE_UNDEF },
#ifdef AV_PIX_FMT_GBRP12 /* 55, 24, 0 / 51, 74, 100 */
    {VLC_CODEC_GBR_PLANAR_12L, AV_PIX_FMT_GBRP12LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBR_PLANAR_12B, AV_PIX_FMT_GBRP12BE,  COLOR_RANGE_UNDEF },
#endif
#ifdef AV_PIX_FMT_GBRP14 /* ffmpeg only */
    {VLC_CODEC_GBR_PLANAR_14L, AV_PIX_FMT_GBRP14LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBR_PLANAR_14B, AV_PIX_FMT_GBRP14BE,  COLOR_RANGE_UNDEF },
#endif
    {VLC_CODEC_GBR_PLANAR_16L, AV_PIX_FMT_GBRP16LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBR_PLANAR_16B, AV_PIX_FMT_GBRP16BE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBRA_PLANAR, AV_PIX_FMT_GBRAP,  COLOR_RANGE_UNDEF },
#ifdef AV_PIX_FMT_GBRAP10 /* 56, 1, 0 / 55, 25, 100 */
    {VLC_CODEC_GBRA_PLANAR_10L, AV_PIX_FMT_GBRAP10LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBRA_PLANAR_10B, AV_PIX_FMT_GBRAP10BE,  COLOR_RANGE_UNDEF },
#endif
#ifdef AV_PIX_FMT_GBRAP12 /* 55, 25, 0, 19, 100 */
    {VLC_CODEC_GBRA_PLANAR_12L, AV_PIX_FMT_GBRAP12LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBRA_PLANAR_12B, AV_PIX_FMT_GBRAP12BE,  COLOR_RANGE_UNDEF },
#endif
    {VLC_CODEC_GBRA_PLANAR_16L, AV_PIX_FMT_GBRAP16LE,  COLOR_RANGE_UNDEF },
    {VLC_CODEC_GBRA_PLANAR_16B, AV_PIX_FMT_GBRAP16BE,  COLOR_RANGE_UNDEF },

    /* XYZ */
    {VLC_CODEC_XYZ12, AV_PIX_FMT_XYZ12BE,  COLOR_RANGE_UNDEF },
    { 0, 0, COLOR_RANGE_UNDEF }
};

/* FIXME special case the RGB formats */
enum AVPixelFormat GetFfmpegChroma( const video_format_t *fmt )
{
    for( int i = 0; chroma_table[i].i_chroma != 0; i++ )
    {
        if( chroma_table[i].i_chroma == fmt->i_chroma &&
            (chroma_table[i].range == COLOR_RANGE_UNDEF || fmt->color_range == chroma_table[i].range) )
        {
            return chroma_table[i].i_chroma_id;
        }
    }
    return AV_PIX_FMT_NONE;
}

int GetVlcChroma( video_format_t *fmt, enum AVPixelFormat i_ffmpeg_chroma )
{
    for( int i = 0; chroma_table[i].i_chroma != 0; i++ )
    {
        if( chroma_table[i].i_chroma_id == i_ffmpeg_chroma )
        {
            fmt->i_chroma = chroma_table[i].i_chroma;
            if (chroma_table[i].range != COLOR_RANGE_UNDEF)
                fmt->color_range = chroma_table[i].range;
            return VLC_SUCCESS;
        }
    }
    return VLC_EGENERIC;
}

enum AVPixelFormat FindFfmpegChroma( vlc_fourcc_t fourcc )
{
    for( int i = 0; chroma_table[i].i_chroma != 0; i++ )
        if( chroma_table[i].i_chroma == fourcc )
            return chroma_table[i].i_chroma_id;
    return AV_PIX_FMT_NONE;
}
