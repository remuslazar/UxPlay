#include "raop.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static airplay_video_t *video_with_uri(const char *uri) {
    static int opaque;
    airplay_video_t *video = airplay_video_init((raop_t *) &opaque, 7000, "en", "en", "en");
    char *uris[] = { strdup(uri) };
    create_media_data_store(video, uris, 1);
    set_next_media_uri_id(video, 1);
    return video;
}

int main(void) {
    const char *original = "mlhls://localhost/itag/234/mediadata.m3u8";
    airplay_video_t *video = video_with_uri(original);
    CHECK(!next_default_audio_request(video, "mlhls://localhost/itag/233/mediadata.m3u8"));
    char *failed = strdup(original);
    for (int i = 0; i < 4; i++) {
        char *next = next_default_audio_request(video, failed);
        CHECK(next && strcmp(next, failed));
        CHECK(!strcmp(get_media_uri_by_num(video, 0), original));
        /* A stale response must not consume a retry. */
        CHECK(!next_default_audio_request(video, failed));
        free(failed);
        failed = next;
    }
    CHECK(!strcmp(failed, "mlhls://localhost/itag/140/mediadata.m3u8"));
    char *last = next_default_audio_request(video, failed);
    CHECK(last && !strcmp(last, original));
    free(failed);
    failed = last;
    CHECK(!next_default_audio_request(video, failed));
    free(failed);
    /* Recovered data remains reachable through the advertised player URI. */
    char *playlist = strdup("#EXTM3U\n#EXT-X-VERSION:3\n#EXTINF:1,\nhttps://example.org/audio.aac\n#EXT-X-ENDLIST\n");
    int count = 1; float duration = 1; bool endlist = true;
    CHECK(!store_media_playlist(video, playlist, &count, &duration, &endlist, 0));
    CHECK(get_media_playlist(video, &count, &duration, "/itag/234/mediadata.m3u8") == playlist);
    airplay_video_destroy(video);
    const char *untouched[] = {
        "mlhls://localhost/master.m3u8",
        "mlhls://localhost/itag/270/mediadata.m3u8",
        "mlhls://localhost/itag/234/xtags/existing-language/mediadata.m3u8",
        "https://example.org/itag/234/mediadata.m3u8"
    };
    for (size_t i = 0; i < sizeof(untouched) / sizeof(untouched[0]); i++) {
        video = video_with_uri(untouched[i]);
        CHECK(!next_default_audio_request(video, untouched[i]));
        airplay_video_destroy(video);
    }
    puts("Default audio probe bounds and cache-key tests passed");
    return 0;
}
