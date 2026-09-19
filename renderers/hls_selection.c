/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "hls_selection.h"
#include <string.h>

/* A master is small, but its HTTP response may arrive in many buffers. Bound
 * accumulation independently of Content-Length (which may be absent). */
#define MAX_MANIFEST_SIZE (4 * 1024 * 1024)

typedef struct {
    hls_codec_t *codecs;
    size_t count;
    logger_t *logger;
    GByteArray *manifest;
    gboolean forwarding;
    gboolean failed;
} hls_selection_t;

static hls_selection_t *selection_new(const hls_codec_t *codecs, size_t count,
                                      logger_t *logger) {
    hls_selection_t *s = g_new0(hls_selection_t, 1);
    s->codecs = g_new(hls_codec_t, count);
    memcpy(s->codecs, codecs, count * sizeof(*codecs));
    s->count = count;
    s->logger = logger;
    s->manifest = g_byte_array_new();
    return s;
}

static void selection_free(gpointer data) {
    hls_selection_t *s = data;
    g_byte_array_unref(s->manifest);
    g_free(s->codecs);
    g_free(s);
}

static void selection_error(GstPad *pad, hls_selection_t *s, const char *reason) {
    if (s->failed) return;
    s->failed = TRUE;
    g_byte_array_set_size(s->manifest, 0);
    GstElement *element = gst_pad_get_parent_element(pad);
    if (element) {
        GST_ELEMENT_ERROR(element, STREAM, DEMUX, ("HLS selection: %s", reason), (NULL));
        gst_object_unref(element);
    }
}

static void collect_buffer(GstPad *pad, hls_selection_t *s, GstBuffer *buffer) {
    if (s->failed) return;
    gsize size = gst_buffer_get_size(buffer);
    if (!size) return;
    if (size > MAX_MANIFEST_SIZE - s->manifest->len) {
        selection_error(pad, s, "manifest exceeds 4 MiB");
        return;
    }
    guint offset = s->manifest->len;
    g_byte_array_set_size(s->manifest, offset + size);
    if (gst_buffer_extract(buffer, 0, s->manifest->data + offset, size) != size)
        selection_error(pad, s, "could not read manifest buffer");
}

static gboolean is_master(const char *text) {
    for (const char *line = text; line && *line; ) {
        if (g_str_has_prefix(line, "#EXT-X-STREAM-INF:") ||
            g_str_has_prefix(line, "#EXT-X-I-FRAME-STREAM-INF:")) return TRUE;
        line = strchr(line, '\n');
        if (line) line++;
    }
    return FALSE;
}

static GstPadProbeReturn manifest_probe(GstPad *pad, GstPadProbeInfo *info, gpointer data) {
    hls_selection_t *s = data;
    /* Re-enter the sink chain once at EOS with the complete, filtered input. */
    if (s->forwarding) return GST_PAD_PROBE_OK;
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
        collect_buffer(pad, s, GST_PAD_PROBE_INFO_BUFFER(info));
        return GST_PAD_PROBE_DROP;
    }
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER_LIST) {
        GstBufferList *list = GST_PAD_PROBE_INFO_BUFFER_LIST(info);
        for (guint i = 0; i < gst_buffer_list_length(list); i++)
            collect_buffer(pad, s, gst_buffer_list_get(list, i));
        return GST_PAD_PROBE_DROP;
    }
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_STREAM_START:
    case GST_EVENT_FLUSH_STOP:
        g_byte_array_set_size(s->manifest, 0);
        s->failed = FALSE;
        break;
    case GST_EVENT_EOS: {
        /* Even on failure, finish the empty demuxer input. Swallowing EOS can
         * leave startup pending while the application handles the bus error.
         * No unfiltered bytes have reached the demuxer. */
        if (s->failed) return GST_PAD_PROBE_OK;
        if (!s->manifest->len) return GST_PAD_PROBE_OK;
        if (memchr(s->manifest->data, '\0', s->manifest->len)) {
            selection_error(pad, s, "manifest contains a NUL byte");
            return GST_PAD_PROBE_OK;
        }
        char *text = g_strndup((const char *)s->manifest->data, s->manifest->len);
        g_byte_array_set_size(s->manifest, 0);
        int removed = is_master(text) ? select_master_playlist_video(text, s->codecs, s->count) : 0;
        if (removed < 0) {
            g_free(text);
            selection_error(pad, s, "no eligible video variant or malformed master playlist");
            return GST_PAD_PROBE_OK;
        }
        if (removed && s->logger)
            logger_log(s->logger, LOGGER_INFO, "HLS selection removed %d variant(s) before GStreamer playback", removed);
        GstBuffer *buffer = gst_buffer_new_wrapped(text, strlen(text));
        s->forwarding = TRUE;
        GstFlowReturn flow = gst_pad_chain(pad, buffer);
        s->forwarding = FALSE;
        if (flow != GST_FLOW_OK) {
            if (flow != GST_FLOW_FLUSHING)
                selection_error(pad, s, "could not deliver filtered manifest");
            return GST_PAD_PROBE_DROP;
        }
        break;
    }
    default:
        break;
    }
    return GST_PAD_PROBE_OK;
}

void hls_selection_attach(GstPad *sink, const hls_codec_t *codecs,
                          size_t count, logger_t *logger) {
    if (!count) return;
    gst_pad_add_probe(sink, GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BUFFER_LIST |
                     GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM | GST_PAD_PROBE_TYPE_EVENT_FLUSH,
                     manifest_probe, selection_new(codecs, count, logger), selection_free);
}

static void element_setup(GstElement *playbin, GstElement *element, gpointer data) {
    (void)playbin;
    hls_selection_t *s = data;
    GstElementFactory *factory = gst_element_get_factory(element);
    if (!factory) return;
    const char *name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
    if (strcmp(name, "hlsdemux2") && strcmp(name, "hlsdemux")) return;
    GstPad *sink = gst_element_get_static_pad(element, "sink");
    if (sink) {
        hls_selection_attach(sink, s->codecs, s->count, s->logger);
        gst_object_unref(sink);
    }
}

static void closure_free(gpointer data, GClosure *closure) {
    (void)closure;
    selection_free(data);
}

void hls_selection_install(GstElement *playbin, const hls_codec_t *codecs,
                           size_t count, logger_t *logger) {
    if (!count) return;
    /* Keep HTTP fetching, URI/redirect queries, cookies, and rendition refreshes
     * in GStreamer. Only the initial HLS manifest bytes are changed; relative
     * and signed URLs retain their original base. Non-HLS sources are untouched.
     * Each demuxer owns a policy copy, including after a URI change. */
    g_signal_connect_data(playbin, "element-setup", G_CALLBACK(element_setup),
                          selection_new(codecs, count, logger), closure_free, 0);
}
