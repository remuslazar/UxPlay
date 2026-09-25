# UxPlay, ha-display branch

A fork of [FDH2/UxPlay](https://github.com/FDH2/UxPlay), kept for ha-display, a
Raspberry Pi display project that is not public yet. There UxPlay is the AirPlay
receiver for the YouTube app, Safari, Vimeo and screen mirroring.

The default branch, `ha-display`, is upstream's `master` plus the changes below,
and nothing else. UxPlay's own documentation is in [README.md](../README.md);
questions and issues that are not about these changes belong
[upstream](https://github.com/FDH2/UxPlay/issues).

## What this branch adds to upstream

**Always on**

- The audio track picked in the YouTube app plays, also when it is changed
  during the video: [#6](https://github.com/remuslazar/UxPlay/pull/6), upstream
  [FDH2/UxPlay#579](https://github.com/FDH2/UxPlay/pull/579).
- HLS seeks land where they were asked to, also in a video with subtitles:
  [#1](https://github.com/remuslazar/UxPlay/pull/1).
- A paused HLS video stays paused when it is scrubbed:
  [#2](https://github.com/remuslazar/UxPlay/pull/2).

**Options**

- `-hls-select codec[@WIDTHxHEIGHT[pFPS]]:…` picks HLS variants by codec,
  resolution and frame rate, for the YouTube app and for master playlists
  GStreamer fetches itself (Vimeo, Safari); see [README.md](../README.md):
  [#5](https://github.com/remuslazar/UxPlay/pull/5). Upstream is replacing it
  with `-rpi` and `-custom`, and this branch will follow once those are in
  upstream `master`.
- `-vs` can name a chain for HLS video, such as
  `-vs "videoconvert ! waylandsink"`:
  [#3](https://github.com/remuslazar/UxPlay/pull/3).

**Build**

- A CTest suite in `tests/`, built by default; `-DBUILD_TESTING=OFF` leaves it
  out. The playback tests need Python 3.

## How the branches work

- `master` is an exact copy of upstream `master` and is never committed to:
  `gh repo sync remuslazar/UxPlay -b master`.
- Each change is a branch from `master` with a draft pull request here against
  `master`, never merged, which stays open while `ha-display` carries the
  change and upstream does not. The [open pull
  requests](https://github.com/remuslazar/UxPlay/pulls?q=is%3Apr+is%3Aopen+base%3Amaster) are the list above.
  When upstream merges the same commits, syncing `master` marks the pull request
  as merged; otherwise it is closed by hand.
- `ha-display` merges each change branch (`--no-ff`) and upstream `master`.
- Every commit ha-display pins is tagged
  `ha-display/<version>-<first 12 digits of the commit>`, so a pinned build
  stays downloadable whatever happens to the branches.

Pull requests from this fork to upstream:
[FDH2/UxPlay, author remuslazar](https://github.com/FDH2/UxPlay/pulls?q=is%3Apr+author%3Aremuslazar).
