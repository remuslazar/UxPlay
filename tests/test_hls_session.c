/* Replay the control sequence used when YouTube replaces its audio language.
 * Exercise the actual HTTP handlers and plist serialization; only playback and
 * the reverse socket are substituted. No external network or media is needed. */
#include <sys/types.h>
#include <sys/socket.h>
#include <math.h>
static char events[65536];
static size_t events_len;
static ssize_t capture_send(int fd, const void *data, size_t len, int flags);
#define send capture_send
#include "../lib/raop.c"
#undef send

static ssize_t capture_send(int fd, const void *data, size_t len, int flags) {
    assert(events_len + len < sizeof(events));
    memcpy(events + events_len, data, len);
    events_len += len;
    events[events_len] = 0;
    return (ssize_t) len;
}

static float player_position = 763.007f;
static int plays, scrubs;
static void play(void *cls, const char *url, float position) {
    assert(strstr(url, "/master.m3u8"));
    player_position = position;
    plays++;
}
static float remove_video(void *cls) { return player_position; }
static void scrub(void *cls, float position) { player_position = position; scrubs++; }
static void info(void *cls, playback_info_t *p) {
    p->position = player_position;
    p->duration = 2400;
    p->rate = 1;
    p->ready_to_play = true;
    p->playback_buffer_empty = false;
    p->playback_buffer_full = true;
    p->playback_likely_to_keep_up = true;
    p->seek_start = 0;
    p->seek_duration = 2400;
}
static const char *session = "00000000-0000-0000-0000-000000000001";
static const char *first = "00000000-0000-0000-0000-000000000002";
static const char *second = "00000000-0000-0000-0000-000000000003";
static const char *third = "00000000-0000-0000-0000-000000000004";

typedef void (*handler_t)(raop_conn_t *, http_request_t *, http_response_t *, char **, int *);
static plist_t request(raop_conn_t *conn, handler_t handler, const char *method,
                       const char *url, plist_t body, int code) {
    char *bin = NULL;
    uint32_t len = 0;
    if (body) plist_to_bin(body, &bin, &len);
    char headers[1024];
    int n = snprintf(headers, sizeof(headers), "%s %s HTTP/1.1\r\n"
                     "X-Apple-Session-ID: %s\r\nContent-Type: application/x-apple-binary-plist\r\n"
                     "Content-Length: %u\r\n\r\n", method, url, session, len);
    http_request_t *req = http_request_init();
    http_request_add_data(req, headers, n);
    if (len) http_request_add_data(req, bin, len);
    plist_mem_free(bin);
    assert(http_request_is_complete(req));
    http_response_t *res = http_response_create();
    http_response_init(res, "HTTP/1.1", 200, "OK");
    char *data = NULL;
    int data_len = 0;
    handler(conn, req, res, &data, &data_len);
    http_response_finish(res, data, data_len);
    int raw_len;
    const char *raw = http_response_get_data(res, &raw_len);
    assert(atoi(raw + 9) == code);
    plist_t result = NULL;
    if (data_len) plist_from_xml(data, data_len, &result);
    free(data);
    http_response_destroy(res);
    http_request_destroy(req);
    if (body) plist_free(body);
    return result;
}
static plist_t action(const char *type, const char *uuid) {
    plist_t root = plist_new_dict(), params = plist_new_dict(), item = plist_new_dict();
    plist_dict_set_item(root, "type", plist_new_string(type));
    plist_dict_set_item(root, "params", params);
    plist_dict_set_item(params, "item", item);
    plist_dict_set_item(item, "uuid", plist_new_string(uuid));
    if (!strcmp(type, "playlistInsert"))
        plist_dict_set_item(item, "Content-Location", plist_new_string("mlhls://localhost/master.m3u8"));
    return root;
}
static void ok(plist_t p) {
    assert(p);
    uint64_t code = 99;
    plist_get_uint_val(plist_dict_get_item(p, "errorCode"), &code);
    assert(code == 0);
    plist_free(p);
}
static void select_language(raop_conn_t *conn, const char *language) {
    plist_t root = plist_new_dict(), value = plist_new_array(), option = plist_new_dict();
    plist_dict_set_item(option, "MediaSelectionGroupMediaType", plist_new_string("soun"));
    plist_dict_set_item(option, "MediaSelectionOptionsExtendedLanguageTag", plist_new_string(language));
    plist_array_append_item(value, option);
    plist_dict_set_item(root, "value", value);
    ok(request(conn, http_handler_set_property, "PUT", "/setProperty?selectedMediaArray", root, 200));
}
static void playlist(raop_conn_t *conn, const char *url, const char *text) {
    plist_t root = plist_new_dict(), params = plist_new_dict();
    plist_dict_set_item(root, "type", plist_new_string("unhandledURLResponse"));
    plist_dict_set_item(root, "params", params);
    plist_dict_set_item(params, "FCUP_Response_StatusCode", plist_new_uint(200));
    plist_dict_set_item(params, "FCUP_Response_RequestID", plist_new_uint(1));
    plist_dict_set_item(params, "FCUP_Response_URL", plist_new_string(url));
    plist_dict_set_item(params, "FCUP_Response_Data", plist_new_data(text, strlen(text)));
    ok(request(conn, http_handler_action, "POST", "/action", root, 200));
}
static void complete_playlists(raop_conn_t *conn, const char *language) {
    playlist(conn, "mlhls://localhost/master.m3u8",
             "#EXTM3U\n"
             "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"audio\",LANGUAGE=\"de-DE\",NAME=\"German\",DEFAULT=YES,AUTOSELECT=YES,URI=\"mlhls://localhost/de.m3u8\"\n"
             "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"audio\",LANGUAGE=\"en-US\",NAME=\"English\",DEFAULT=NO,AUTOSELECT=YES,URI=\"mlhls://localhost/en.m3u8\"\n"
             "#EXT-X-STREAM-INF:BANDWIDTH=100000,CODECS=\"avc1.64001f,mp4a.40.2\",RESOLUTION=1280x720,AUDIO=\"audio\"\n"
             "mlhls://localhost/video.m3u8\n");
    airplay_video_t *video = hls_get_current_video(conn->raop);
    const char *master = get_master_playlist(video);
    assert(strstr(master, language));
    assert(!strstr(master, !strcmp(language, "en-US") ? "de-DE" : "en-US"));
    int count = get_num_media_uri(video);
    for (int i = 0; i < count; i++) {
        char *url = strdup(get_media_uri_by_num(video, i));
        playlist(conn, url, "#EXTM3U\n#EXT-X-TARGETDURATION:10\n#EXTINF:10,\nhttps://example.org/segment.ts\n#EXT-X-ENDLIST\n");
        free(url);
    }
    assert(!get_fetching_playlists(video));
}
int main(void) {
    raop_t raop = {0};
    raop.logger = logger_init();
    raop.current_video = raop.removed_video = -1;
    raop.lang = ""; raop.lang_subtitles = ""; raop.lang_system = "en-GB";
    raop.callbacks.on_video_play = play;
    raop.callbacks.on_video_playlist_remove = remove_video;
    raop.callbacks.on_video_scrub = scrub;
    raop.callbacks.on_video_acquire_playback_info = info;
    /* A real httpd object supplies connection lookup; send() captures the bytes. */
    httpd_callbacks_t callbacks = {0};
    raop.httpd = httpd_init(raop.logger, &callbacks, false);
    raop_conn_t conn = {0}; conn.raop = &raop;
    airplay_video_t *initial = hls_add_video(&raop, session, first);
    set_client_audio_language(initial, "de-DE");
    hls_set_master_location(initial, "mlhls://localhost/master.m3u8");
    const char *uuids[] = {first, second, third};
    for (int turn = 0; turn < 2; turn++) {
        ok(request(&conn, http_handler_action, "POST", "/action", action("playlistRemove", uuids[turn]), 200));
        assert(raop.current_video == -1);
        events_len = 0;
        ok(request(&conn, http_handler_action, "POST", "/action", action("playlistInsert", uuids[turn+1]), 200));
        assert(strstr(events, "currentItemChanged"));
        assert(strstr(events, uuids[turn+1]));
        assert(strstr(events, "unhandledURLRequest"));
        /* The new item gets the seek; the old pipeline must not be resumed. */
        const char *url = turn ? "/scrub?position=0" : "/scrub?position=763.007";
        assert(!request(&conn, http_handler_scrub, "POST", url, NULL, 200));
        assert(scrubs == 0);
        const char *language = turn ? "de-DE" : "en-US";
        select_language(&conn, language);
        complete_playlists(&conn, language);
        assert(plays == turn + 1);
        assert(fabs(player_position - (turn ? 0 : 763.007f)) < 0.01);
        plist_t p = request(&conn, http_handler_playback_info, "GET", "/playback-info", NULL, 200);
        char *uuid = NULL;
        plist_get_string_val(plist_dict_get_item(p, "uuid"), &uuid);
        assert(uuid && !strcmp(uuid, uuids[turn+1]));
        plist_mem_free(uuid); plist_free(p);
    }
    for (int i = 0; i < MAX_AIRPLAY_VIDEO; i++)
        if (raop.airplay_video[i]) airplay_video_destroy(raop.airplay_video[i]);
    httpd_destroy(raop.httpd);
    logger_destroy(raop.logger);
    puts("HLS replacement, selection, current-item event and repeated seeks passed");
    return 0;
}
