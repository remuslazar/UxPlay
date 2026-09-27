# UxPlay for Home Assistant displays

A fork of [FDH2/UxPlay](https://github.com/FDH2/UxPlay), the AirPlay receiver,
kept for ha-display, a Raspberry Pi display project that is not public yet.
UxPlay is its AirPlay receiver for the YouTube app, Safari, Vimeo and screen
mirroring, running around the clock on a Pi 4 and a Pi 5. Everything below was
made for that, measured on those boards and runs there every day.

UxPlay's own documentation is in [README.md](../README.md). Questions and issues
that are not about the changes below belong
[upstream](https://github.com/FDH2/UxPlay/issues).

## What this fork adds

| Change | Type | Details | Upstream |
| --- | --- | --- | --- |
| Seeking and resuming past 35:47 in an HLS video land where asked | fix | [FDH2/UxPlay#576](https://github.com/FDH2/UxPlay/pull/576) | merged, removed again on 2026-09-27 |
| Plays at the volume announced to the client, and keeps it across videos | fix | [FDH2/UxPlay#577](https://github.com/FDH2/UxPlay/pull/577) | merged, removed again on 2026-09-27 |
| HLS seeks land where asked, also in a video with subtitles | fix | [#1](https://github.com/remuslazar/UxPlay/pull/1) | not upstream |
| A paused HLS video stays paused when it is scrubbed | fix | [#2](https://github.com/remuslazar/UxPlay/pull/2) | not upstream |
| The HLS language filter no longer leaks memory per playlist | fix | [#7](https://github.com/remuslazar/UxPlay/pull/7) | fixed upstream in its own way |
| Plays the audio track picked in the YouTube app, also when it is changed during the video | feature | [#6](https://github.com/remuslazar/UxPlay/pull/6) | merged in parts, removed again on 2026-09-27 |
| `-hls-select` picks HLS variants by codec, resolution and frame rate, also for Vimeo and Safari | new option | [#5](https://github.com/remuslazar/UxPlay/pull/5) | upstream has its own filter (`-rpi`, `-custom`) |
| `-vs` can name a chain for HLS video, e.g. `videoconvert ! waylandsink` | extended option | [#3](https://github.com/remuslazar/UxPlay/pull/3) | not upstream |
| A CTest suite in `tests/` (HLS selection, playback, seeking), built by default | build | [#5](https://github.com/remuslazar/UxPlay/pull/5) | upstream has no tests |

Fixes and features are always on; an option acts only when it is given.

Build it like upstream (see [README.md](../README.md)); `-DBUILD_TESTING=OFF`
leaves the tests out.

## Branches

- **`master`** (default): the stable state, with every change above. A new
  change is merged into it once it has run on a display.
- **`ha-display`**: what the displays run. That's `master` plus changes still
  under test, whose [pull
  requests](https://github.com/remuslazar/UxPlay/pulls?q=is%3Apr+is%3Aopen+base%3Amaster)
  are open against `master`.
- **`upstream`**: an exact copy of FDH2/UxPlay's `master`. Upstream fixes are
  taken over one by one, like any other change.
- Every build a display runs is tagged `ha-display/<version>-<commit>`, so it
  stays downloadable.

Upstream is welcome to take any of these changes; ask, and I'll prepare a small
pull request against its `master`.
