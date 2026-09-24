"""A scrub in HLS video lands where the client asked, with a subtitle rendition in one long segment,
and a scrub while paused leaves the video paused."""
import functools
import http.server
import pathlib
import subprocess
import sys
import threading

MEDIA = pathlib.Path(__file__).parent / "media" / "hls"


class Handler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *args):
        pass


server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), functools.partial(Handler, directory=str(MEDIA)))
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    url = f"http://127.0.0.1:{server.server_port}/seek-master.m3u8"
    result = subprocess.run([sys.argv[1], url], capture_output=True, text=True, timeout=60)
    if result.returncode == 77:
        print(result.stderr)
        sys.exit(77)
    assert result.returncode == 0, result.stdout + result.stderr
    print(result.stdout.strip().splitlines()[-1])
finally:
    server.shutdown()
    server.server_close()
