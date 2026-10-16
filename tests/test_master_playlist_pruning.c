#include "raop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define AUDIO_EN "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"a\",LANGUAGE=\"en\",URI=\"en.m3u8\",DEFAULT=YES,AUTOSELECT=YES\n"
#define AUDIO_DE "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"a\",LANGUAGE=\"de\",URI=\"de.m3u8\""
#define SUBS_DE "#EXT-X-MEDIA:TYPE=SUBTITLES,GROUP-ID=\"s\",LANGUAGE=\"de\",URI=\"subs.m3u8\""
#define YES ",DEFAULT=YES,AUTOSELECT=YES\n"
#define AVC "#EXT-X-STREAM-INF:CODECS=\"avc1\",RESOLUTION=1920x1080\navc.m3u8\n"
#define VP9 "#EXT-X-STREAM-INF:CODECS=\"vp09\",RESOLUTION=1920x1080\nvp9.m3u8\n"

static void expect_pruning(const char *input, const char *language_result,
                           const char *video_result, int removed) {
    hls_codec_t *codecs;
    size_t count;
    CHECK(hls_select_parse("avc1@1920x1080:vp09@1920x1080", &codecs, &count));
    /* Initialization only stores this opaque pointer; these tests do not use RAOP. */
    airplay_video_t *video = airplay_video_init((raop_t *) &count, 7000, "de", "de", "en");
    CHECK(video != NULL);
    char *playlist = malloc(strlen(input) + 1);
    CHECK(playlist != NULL);
    strcpy(playlist, input);
    playlist = select_master_playlist_language(video, playlist);
    CHECK(!strcmp(playlist, language_result));
    CHECK(select_master_playlist_video(playlist, codecs, count) == removed);
    CHECK(!strcmp(playlist, video_result));
    free(playlist);
    airplay_video_destroy(video);
    free(codecs);
}

/* The YouTube app lists an audio rendition (group 233, for its smallest
 * variants) and answers 404 when it is requested: the video plays without it. */
#define LOCAL "http://localhost:7000/itag/"
#define AUDIO_233 "#EXT-X-MEDIA:URI=\"" LOCAL "233/mediadata.m3u8\",TYPE=AUDIO,GROUP-ID=\"233\",DEFAULT=YES,AUTOSELECT=YES,NAME=\"Default\"\n"
#define AUDIO_234 "#EXT-X-MEDIA:URI=\"" LOCAL "234/mediadata.m3u8\",TYPE=AUDIO,GROUP-ID=\"234\",DEFAULT=YES,AUTOSELECT=YES,NAME=\"Default\"\n"
#define V229_233 "#EXT-X-STREAM-INF:BANDWIDTH=385253,CODECS=\"avc1.4D4015,mp4a.40.5\",RESOLUTION=426x240,AUDIO=\"233\"\n" LOCAL "229/mediadata.m3u8\n"
#define V229_234 "#EXT-X-STREAM-INF:BANDWIDTH=469991,CODECS=\"avc1.4D4015,mp4a.40.2\",RESOLUTION=426x240,AUDIO=\"234\"\n" LOCAL "229/mediadata.m3u8\n"
#define V269_233 "#EXT-X-STREAM-INF:BANDWIDTH=206254,CODECS=\"avc1.4D400C,mp4a.40.5\",RESOLUTION=256x144,AUDIO=\"233\"\n" LOCAL "269/mediadata.m3u8\n"
#define V270_234 "#EXT-X-STREAM-INF:BANDWIDTH=4759759,CODECS=\"avc1.640028,mp4a.40.2\",RESOLUTION=1920x1080,AUDIO=\"234\"\n" LOCAL "270/mediadata.m3u8\n"

static void expect_removal(const char *input, const char *uri, const char *result, int variants) {
    int port = 0;
    airplay_video_t *video = airplay_video_init((raop_t *) &port, 7000, "en", "en", "en");
    CHECK(video != NULL);
    set_uri_prefix(video, "mlhls://localhost", strlen("mlhls://localhost"));
    char *playlist = malloc(strlen(input) + 1);
    CHECK(playlist != NULL);
    strcpy(playlist, input);
    store_master_playlist(video, playlist);
    CHECK(remove_master_playlist_media(video, uri) == variants);
    CHECK(!strcmp(get_master_playlist(video), result));
    airplay_video_destroy(video);
}

static void test_missing_media(void) {
    /* The rendition goes, and with it the variants of its group. */
    expect_removal("#EXTM3U\n#EXT-X-INDEPENDENT-SEGMENTS\n" AUDIO_233 AUDIO_234 V229_233 V229_234 V269_233 V270_234,
                   "mlhls://localhost/itag/233/mediadata.m3u8",
                   "#EXTM3U\n#EXT-X-INDEPENDENT-SEGMENTS\n" AUDIO_234 V229_234 V270_234, 2);
    /* A variant's own playlist; the other variant with that URI goes too. */
    expect_removal("#EXTM3U\n" AUDIO_233 AUDIO_234 V229_233 V229_234 V270_234,
                   "mlhls://localhost/itag/229/mediadata.m3u8",
                   "#EXTM3U\n" AUDIO_233 AUDIO_234 V270_234, 1);
    /* Nothing left to play; a URI of another prefix is not this video's. */
    expect_removal("#EXTM3U\n" AUDIO_233 V269_233, "mlhls://localhost/itag/233/mediadata.m3u8", "#EXTM3U\n", 0);
    expect_removal("#EXTM3U\n" AUDIO_233 V269_233, "https://example.com/itag/233/mediadata.m3u8",
                   "#EXTM3U\n" AUDIO_233 V269_233, -1);
}

int main(void) {
    /* Both callers use the same pruning code: LANGUAGE grows attributes and
     * deletes a rendition; video selection then compacts complete variants. */
    expect_pruning("#EXTM3U\n" AUDIO_EN AUDIO_DE "\n" SUBS_DE "\n" VP9 AVC,
                   "#EXTM3U\n" AUDIO_DE YES SUBS_DE YES VP9 AVC,
                   "#EXTM3U\n" AUDIO_DE YES SUBS_DE YES AVC, 1);
    /* A rejected video selection must preserve the LANGUAGE-pruned buffer. */
    expect_pruning("#EXTM3U\n" AUDIO_EN AUDIO_DE "\n" SUBS_DE "\n"
                   "#EXT-X-STREAM-INF:CODECS=\"vp09\",RESOLUTION=3840x2160\nuhd.m3u8\n",
                   "#EXTM3U\n" AUDIO_DE YES SUBS_DE YES
                   "#EXT-X-STREAM-INF:CODECS=\"vp09\",RESOLUTION=3840x2160\nuhd.m3u8\n",
                   "#EXTM3U\n" AUDIO_DE YES SUBS_DE YES
                   "#EXT-X-STREAM-INF:CODECS=\"vp09\",RESOLUTION=3840x2160\nuhd.m3u8\n", -1);
    /* Already selected languages and video require no rewrite. */
    expect_pruning("#EXTM3U\n" AUDIO_DE YES SUBS_DE YES AVC,
                   "#EXTM3U\n" AUDIO_DE YES SUBS_DE YES AVC,
                   "#EXTM3U\n" AUDIO_DE YES SUBS_DE YES AVC, 0);
    /* DEFAULT=/AUTOSELECT= as the last attributes leave no empty attribute. */
    expect_pruning("#EXTM3U\n" AUDIO_EN AUDIO_DE ",DEFAULT=NO,AUTOSELECT=YES\n" AVC,
                   "#EXTM3U\n" AUDIO_DE YES AVC,
                   "#EXTM3U\n" AUDIO_DE YES AVC, 0);
    /* DEFAULT=YES in a quoted value is not the attribute. */
    expect_pruning("#EXTM3U\n" AUDIO_EN AUDIO_DE ",NAME=\"xDEFAULT=YESx\",DEFAULT=YES\n" AVC,
                   "#EXTM3U\n" AUDIO_DE ",NAME=\"xDEFAULT=YESx\"" YES AVC,
                   "#EXTM3U\n" AUDIO_DE ",NAME=\"xDEFAULT=YESx\"" YES AVC, 0);
    /* A playlist that repeats an attribute is passed on as it came. */
    expect_pruning("#EXTM3U\n" AUDIO_EN AUDIO_DE ",DEFAULT=YES,DEFAULT=YES\n" AVC,
                   "#EXTM3U\n" AUDIO_EN AUDIO_DE ",DEFAULT=YES,DEFAULT=YES\n" AVC,
                   "#EXTM3U\n" AUDIO_EN AUDIO_DE ",DEFAULT=YES,DEFAULT=YES\n" AVC, 0);
    /* Malformed renditions do not stop the filter: an unknown TYPE is kept as
     * it is, a LANGUAGE with a ',' in it matches no language, and one without
     * its closing '"' on its line is no language. */
    expect_pruning("#EXTM3U\n#EXT-X-MEDIA:TYPE=OTHER,GROUP-ID=\"o\"\n" AUDIO_EN AUDIO_DE "\n" AVC,
                   "#EXTM3U\n#EXT-X-MEDIA:TYPE=OTHER,GROUP-ID=\"o\"\n" AUDIO_DE YES AVC,
                   "#EXTM3U\n#EXT-X-MEDIA:TYPE=OTHER,GROUP-ID=\"o\"\n" AUDIO_DE YES AVC, 0);
    expect_pruning("#EXTM3U\n" AUDIO_DE "\n#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"a\",LANGUAGE=\"de,en\",URI=\"x.m3u8\"\n" AVC,
                   "#EXTM3U\n" AUDIO_DE YES AVC,
                   "#EXTM3U\n" AUDIO_DE YES AVC, 0);
    expect_pruning("#EXTM3U\n" AVC "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"a\",LANGUAGE=\"de\n",
                   "#EXTM3U\n" AVC "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"a\",LANGUAGE=\"de" YES,
                   "#EXTM3U\n" AVC "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"a\",LANGUAGE=\"de" YES, 0);
    test_missing_media();
    puts("Master playlist pruning tests passed");
    return 0;
}
