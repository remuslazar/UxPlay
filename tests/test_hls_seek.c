#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include "video_renderer.h"

/* Invoked by test_hls_seek.py against its loopback HTTP server. Plays an HLS
 * stream through the renderer as a client's video, scrubs it the way the
 * client's slider does, and checks the position the client is told next.
 * Then it scrubs the way the YouTube app does, which pauses the video first:
 * the video stays paused at the new position until the client resumes it. */
#define SCRUB_TO 2.5
#define PAUSED_SCRUB_TO 1.0

/* The HLS playbin plays audio through autoaudiosink: give it a sink that
 * needs no sound device and still keeps time. */
typedef GstBaseSink TestAudioSink;
typedef GstBaseSinkClass TestAudioSinkClass;
G_DEFINE_TYPE(TestAudioSink, test_audio_sink, GST_TYPE_BASE_SINK)

static GstStaticPadTemplate audio_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("audio/x-raw"));

static void test_audio_sink_class_init(TestAudioSinkClass *klass) {
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(element_class, "Test audio sink", "Sink/Audio",
                                          "Discards audio in time", "UxPlay tests");
    gst_element_class_add_static_pad_template(element_class, &audio_template);
}

static void test_audio_sink_init(TestAudioSink *sink) {
    gst_base_sink_set_sync(GST_BASE_SINK(sink), TRUE);
}

static GMainLoop *loop;
static enum { STARTING, SCRUBBED, PAUSED, RESUMED } step = STARTING;
static gint64 step_at;
static int result = 1;

static void log_message(void *data, int level, const char *message) {
    g_print("%s\n", message);
}

static void next_step(int next) {
    step = next;
    step_at = g_get_monotonic_time();
}

static gboolean finish(int code) {
    result = code;
    g_main_loop_quit(loop);
    return G_SOURCE_REMOVE;
}

static gboolean poll_position(gpointer data) {
    double duration, position, seek_start, seek_duration;
    float rate;
    bool buffer_empty, buffer_full;
    video_get_playback_info(&duration, &position, &seek_start, &seek_duration, &rate,
                            &buffer_empty, &buffer_full);
    gint64 elapsed = g_get_monotonic_time() - step_at;
    switch (step) {
    case STARTING:
        /* playing and seekable, as when the client's slider is dragged */
        if (rate == 1.0f && seek_duration > 0.0 && position >= 0.3) {
            g_print("Playing at %.3f s, scrubbing to %.3f s\n", position, SCRUB_TO);
            video_renderer_seek(SCRUB_TO);
            next_step(SCRUBBED);
        }
        break;
    case SCRUBBED:
        /* the first position reported once playback goes on after the scrub */
        if (elapsed < 300 * G_TIME_SPAN_MILLISECOND || rate != 1.0f || position < 0.0) {
            break;
        }
        g_print("Position after the scrub: %.3f s\n", position);
        if (position < SCRUB_TO - 0.05 || position >= SCRUB_TO + 1.0) {
            g_printerr("Scrubbed to %.3f s, playing at %.3f s\n", SCRUB_TO, position);
            return finish(1);
        }
        g_print("Pausing, scrubbing to %.3f s\n", PAUSED_SCRUB_TO);
        video_renderer_pause();
        video_renderer_seek(PAUSED_SCRUB_TO);
        next_step(PAUSED);
        break;
    case PAUSED:
        /* still paused through the seek and the buffering after it */
        if (elapsed >= 200 * G_TIME_SPAN_MILLISECOND && rate != 0.0f) {
            g_printerr("Scrubbed while paused, playing at %.3f s\n", position);
            return finish(1);
        }
        if (elapsed < 1500 * G_TIME_SPAN_MILLISECOND) {
            break;
        }
        g_print("Position after the paused scrub: %.3f s\n", position);
        if (position < PAUSED_SCRUB_TO - 0.05 || position >= PAUSED_SCRUB_TO + 0.2) {
            g_printerr("Scrubbed to %.3f s while paused, paused at %.3f s\n", PAUSED_SCRUB_TO, position);
            return finish(1);
        }
        video_renderer_resume();
        next_step(RESUMED);
        break;
    case RESUMED:
        if (elapsed < 300 * G_TIME_SPAN_MILLISECOND || rate != 1.0f) {
            break;
        }
        g_print("Playing again at %.3f s\n", position);
        if (position < PAUSED_SCRUB_TO - 0.05 || position >= PAUSED_SCRUB_TO + 1.0) {
            g_printerr("Resumed at %.3f s, playing at %.3f s\n", PAUSED_SCRUB_TO, position);
            return finish(1);
        }
        return finish(0);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean time_out(gpointer data) {
    static const char *steps[] = {"before playback", "after the scrub", "while paused", "after the resume"};
    g_printerr("Timed out %s\n", steps[step]);
    g_main_loop_quit(loop);
    return G_SOURCE_REMOVE;
}

int main(int argc, char **argv) {
    gst_init(&argc, &argv);
    if (argc != 2) return 2;
    const char *required[] = {"playbin3", "hlsdemux2", "souphttpsrc", "avdec_h264", "avdec_aac", "subparse"};
    for (guint i = 0; i < G_N_ELEMENTS(required); i++) {
        GstElementFactory *factory = gst_element_factory_find(required[i]);
        if (!factory) {
            g_printerr("Missing test plugin: %s\n", required[i]);
            return 77;
        }
        /* software decoders, as in test_hls_playback.c */
        if (g_str_has_prefix(required[i], "avdec_"))
            gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE(factory), GST_RANK_PRIMARY + 100);
        gst_object_unref(factory);
    }
    gst_element_register(NULL, "uxplaytestaudiosink", GST_RANK_PRIMARY + 1000, test_audio_sink_get_type());
    logger_t *logger = logger_init();
    logger_set_callback(logger, log_message, NULL);
    logger_set_level(logger, LOGGER_INFO);
    videoflip_t videoflip[2] = {NONE, NONE};
    /* "-vs fakesink sync=true": in time, as a real video sink */
    video_renderer_init(logger, "test", videoflip, "h264parse", "", "decodebin", "videoconvert", "fakesink", " sync=true",
                        false, true, false, false, 3, argv[1], NULL, 0);
    loop = g_main_loop_new(NULL, FALSE);
    video_renderer_listen(loop, 0);
    video_renderer_start();
    g_timeout_add(50, poll_position, NULL);
    g_timeout_add_seconds(20, time_out, NULL);
    g_main_loop_run(loop);
    video_renderer_stop();
    video_renderer_destroy();
    g_main_loop_unref(loop);
    logger_destroy(logger);
    return result;
}
