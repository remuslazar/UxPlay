# UxPlay, ha-display branch

This is a fork of [FDH2/UxPlay](https://github.com/FDH2/UxPlay), kept for
ha-display, a Raspberry Pi display project that is not public yet. There UxPlay
is the AirPlay receiver for the YouTube app, Safari, Vimeo and screen mirroring.

The default branch, `ha-display`, is upstream's `master` plus the changes listed
below, and nothing else. UxPlay's own documentation is in
[README.md](../README.md); questions and issues that are not about these changes
belong [upstream](https://github.com/FDH2/UxPlay/issues).

## What this branch adds to upstream

### Always on

- **The YouTube app's audio track plays.** The track picked on the iPhone
  (an original or an AI dub) takes precedence over `-lang` and the system
  language. Picking another one during the video, playing or paused, continues
  at the same position in the new language, and the app's controls keep
  working. This includes two fixes it depends on: the video renderers are
  forgotten when they are destroyed, and the start position goes to the new
  HLS pipeline, not the old one. Upstream
  [#579](https://github.com/FDH2/UxPlay/pull/579).
- **HLS seeks land where they were asked to, also with subtitles.** A seek no
  longer snaps to the start of the subtitle segment, which in YouTube's
  playlists could put it up to ten minutes early. Internal
  [#1](https://github.com/remuslazar/UxPlay/pull/1).
- **A paused HLS video stays paused when it is scrubbed.** It shows the frame it
  was scrubbed to, and the client is told the scrub is done. Internal
  [#2](https://github.com/remuslazar/UxPlay/pull/2).

### Options

- **`-hls-select [list]`** picks HLS variants by codec and maximum resolution, in
  order of preference, for example `-hls-select avc1@1920x1080:vp09@1920x1080`.
  It applies to the YouTube app's playlist and to master playlists GStreamer
  fetches itself (Vimeo, Safari). Without it nothing is filtered. Documented
  under `-hls-select` in [README.md](../README.md). Upstream
  [#572](https://github.com/FDH2/UxPlay/pull/572), merged only into upstream's
  `pruning` branch, and [#575](https://github.com/FDH2/UxPlay/pull/575).
  **This option will go:** upstream is replacing it with `-rpi` and `-custom`
  (branch `hlspi`, with [#581](https://github.com/FDH2/UxPlay/pull/581) for the
  master playlists GStreamer fetches), and this branch follows once that is in
  upstream `master`.
- **`-vs` can name a chain for HLS video**, for example
  `-vs "videoconvert ! waylandsink"`, as it already could for mirroring. A single
  element works as before. Internal
  [#3](https://github.com/remuslazar/UxPlay/pull/3).

### Build

- A CTest suite in `tests/`: HLS variant selection, playlist pruning, playback
  over HTTP and seeking. It is built by default (`include(CTest)`);
  `-DBUILD_TESTING=OFF` leaves it out. The playback tests need Python 3.
  Upstream has no tests.

## Branches and pull requests

- **`ha-display`** (default): upstream `master` with every change above merged
  into it (`--no-ff`). This is what ha-display builds.
- **`master`**: an exact copy of upstream `master`, never committed to.
  `gh repo sync remuslazar/UxPlay -b master` brings it up to date.
- **Feature branches** start from `master`, so each can go upstream unchanged.

Each change has a pull request: upstream, or an internal one here against
`master`. An internal pull request is never merged, because `master` only
mirrors upstream; it records the change and is closed once the same branch is
opened upstream, after a display has proven it. Upstream is welcome to take any
of them. Once upstream has a change, this branch gets it with the next merge of
`master`, and it leaves the list above.

ha-display pins a commit, not a branch. Every pinned commit is tagged
`ha-display/<version>-<first 12 digits of the commit>`, so a pinned build stays
downloadable whatever happens to the branches.

All pull requests from this fork to upstream:
[FDH2/UxPlay, author remuslazar](https://github.com/FDH2/UxPlay/pulls?q=is%3Apr+author%3Aremuslazar).
