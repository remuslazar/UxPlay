/* SPDX-License-Identifier: GPL-3.0-or-later
 * Apply the shared HLS selection policy before GStreamer parses a manifest. */
#ifndef HLS_SELECTION_H
#define HLS_SELECTION_H

#include <gst/gst.h>
#include "../lib/raop.h"

#ifdef __cplusplus
extern "C" {
#endif
void hls_selection_install(GstElement *playbin, const hls_codec_t *codecs,
                           size_t count, logger_t *logger);
/* Attach to an HLS demuxer's sink pad; the probe owns a copy of the policy. */
void hls_selection_attach(GstPad *sink, const hls_codec_t *codecs,
                          size_t count, logger_t *logger);
#ifdef __cplusplus
}
#endif
#endif
