# UxPlay for ha-display

This is a fork of [FDH2/UxPlay](https://github.com/FDH2/UxPlay), kept for
ha-display: an Ansible collection that makes a Raspberry Pi a Home Assistant
dashboard display, with UxPlay as its AirPlay receiver for the YouTube app,
Safari, Vimeo and screen mirroring. The branch shown here, `ha-display`, is
upstream's `master` plus the changes made for those displays that upstream does
not have yet.

UxPlay's own documentation is in [README.md](../README.md). UxPlay is developed
upstream, and questions and issues that are not about the changes below belong
there.

## Branches

- **`ha-display`** (default): upstream `master` with every change below merged
  into it. This is what ha-display builds.
- **`master`**: an exact copy of upstream `master`, never committed to.
  `gh repo sync remuslazar/UxPlay -b master` brings it up to date.
- **Feature branches** start from `master`, so each one can go upstream
  unchanged.

ha-display pins a commit, not a branch. Every pinned commit is tagged
`ha-display/<version>-<first 12 digits of the commit>`, so a pinned build stays
downloadable whatever happens to the branches.

## Changes

A change starts as a branch from `master` and gets a pull request: upstream, or
an internal one here against `master`. An internal pull request is never
merged, because `master` only mirrors upstream; it records the change and is
closed once the same branch is opened upstream. The branch is merged into
`ha-display` (`--no-ff`) when a display is to run it. A change is offered
upstream once a display has proven it, and upstream is welcome to take any of
them. Once upstream has a change, `ha-display` gets it with the next merge of
`master`, and its row below goes.

In `ha-display`, not in upstream `master`:

| Change | Branch | Pull request |
| --- | --- | --- |
| Pick HLS variants by codec and resolution (`-hls-select`), also for master playlists GStreamer fetches itself (Vimeo, Safari) | `codex/direct-hls-selection` | upstream [#572](https://github.com/FDH2/UxPlay/pull/572) (merged into upstream's `pruning`, not `master`), [#575](https://github.com/FDH2/UxPlay/pull/575) |
| Play the audio track picked in the YouTube app, also when it is changed during the video | `fix/hls-audio-track-switch` | upstream [#579](https://github.com/FDH2/UxPlay/pull/579) |
| Seek an HLS video with subtitles to the position asked for, not to the start of a subtitle segment | `fix/hls-accurate-seek` | internal [#1](https://github.com/remuslazar/UxPlay/pull/1) |
| Keep a paused HLS video paused when the client scrubs it | `fix/hls-paused-seek` | internal [#2](https://github.com/remuslazar/UxPlay/pull/2) |
| Let `-vs` give playbin a chain, such as a converter in front of the sink | `feat/videosink-chain` | internal [#3](https://github.com/remuslazar/UxPlay/pull/3) |

Upstream is replacing `-hls-select` with its own filter, `-rpi` and `-custom`
on its `hlspi` branch, and
[#581](https://github.com/FDH2/UxPlay/pull/581) brings the filtering of
master playlists GStreamer fetches itself to it. Once that is in upstream
`master`, `ha-display` follows it and drops `-hls-select`.

All pull requests from this fork to upstream:
[FDH2/UxPlay, author remuslazar](https://github.com/FDH2/UxPlay/pulls?q=is%3Apr+author%3Aremuslazar).
