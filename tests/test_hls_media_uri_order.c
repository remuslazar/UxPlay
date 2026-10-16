#include "raop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define PREFIX "mlhls://localhost"

int main(void) {
    const char *master = "#EXTM3U\n"
        "#EXT-X-MEDIA:TYPE=AUDIO,GROUP-ID=\"233\",URI=\"" PREFIX "/itag/233/mediadata.m3u8\"\n"
        "#EXT-X-MEDIA:URI=\"" PREFIX "/itag/234/xtags/original/mediadata.m3u8\",TYPE=AUDIO,GROUP-ID=\"234\"\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=100000,AUDIO=\"233\"\n" PREFIX "/itag/229/mediadata.m3u8\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=200000,AUDIO=\"234\"\n" PREFIX "/itag/229/mediadata.m3u8\n"
        "#EXT-X-STREAM-INF:BANDWIDTH=1000000,AUDIO=\"234\"\n" PREFIX "/itag/270/mediadata.m3u8\n";
    const char *expected[] = {
        PREFIX "/itag/229/mediadata.m3u8",
        PREFIX "/itag/229/mediadata.m3u8",
        PREFIX "/itag/270/mediadata.m3u8",
        PREFIX "/itag/233/mediadata.m3u8",
        PREFIX "/itag/234/xtags/original/mediadata.m3u8"
    };
    char **uris = NULL; int count = 0;
    CHECK(!create_media_uri_table(PREFIX, master, strlen(master), &uris, &count));
    CHECK(count == 5);
    for (int i = 0; i < count; i++) {
        CHECK(!strcmp(uris[i], expected[i]));
        CHECK(strstr(master, uris[i]));
        free(uris[i]);
    }
    free(uris);
    master = "#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=1\n" PREFIX "/only.m3u8\n";
    CHECK(!create_media_uri_table(PREFIX, master, strlen(master), &uris, &count));
    CHECK(count == 1 && !strcmp(uris[0], PREFIX "/only.m3u8"));
    free(uris[0]); free(uris);
    puts("Advertised playlist request order tests passed");
    return 0;
}
