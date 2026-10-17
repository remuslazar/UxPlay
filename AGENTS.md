# AGENTS.md

This is a downstream fork of [FDH2/UxPlay](https://github.com/FDH2/UxPlay),
kept for ha-display, a Raspberry Pi display project that is not public yet.
What the fork adds is listed on its front page,
[`.github/README.md`](.github/README.md); UxPlay's own documentation is in
`README.md`.

These rules hold for every coding agent working here. What is specific to one
harness lives beside it: Claude Code's `CLAUDE.md` imports this file.

## Rules

- **The fork is public.** No site data: no host names or addresses of
  displays, user names, secrets, or links to private repositories.
- **English everywhere** — code, comments, commit messages, pull requests.
- **Commit messages say what changes for a user or on a display.**
- **Minimal, surgical diffs** in upstream's code style: a bounds or NULL check
  rather than a restructure; fewer changed lines is a goal in itself. Propose
  a larger rework separately instead of bundling it.
- **Self-contained changes**, so each can be tested, reverted or taken
  upstream on its own. Related fixes can share a pull request as separate
  commits.
- **AI assistance is disclosed** with a `Co-Authored-By` trailer.

## Branches

- **`master`** (default): the stable state. It changes only through a merged
  pull request, with a merge commit (no squash or rebase: `ha-display` must
  hold the same commits). A GitHub ruleset enforces this, without bypass.
- **`ha-display`**: what the displays run, `master` plus changes under test,
  merged with `--no-ff`. Never rewritten: ha-display pins its commits. Direct
  pushes are fine; a dropped change is undone with `git revert -m 1 <merge>`.
- **`upstream`**: an exact copy of FDH2/UxPlay's `master`, force-updated on
  purpose (`git push origin +upstream/master:refs/heads/upstream`).
- **Tags `ha-display/<version>-<sha12>`**: one annotated, pushed tag for every
  commit a display runs. Never moved or deleted.

## A change

1. Branch from `master`: `fix/…` or `feat/…`.
2. Open a pull request against `master`, always with
   `gh pr create -R remuslazar/UxPlay` (without `-R`, `gh` picks FDH2/UxPlay
   as the base). It stays open while the change is tested.
3. Merge the branch into `ha-display` with `--no-ff`, push, tag the merge, and
   pin that commit in the ha-display repository. Several changes can share a
   pin.
4. Once it has run on a display, add or update the change's row in the table
   of `.github/README.md` on the branch, and merge the pull request.
   Docs-only changes need no display run.
5. Merge `master` into `ha-display` (a fast-forward when possible), so it
   stays a superset of `master`.

## Upstream

- **Never post anything** to FDH2/UxPlay (issues, comments, pull requests),
  GStreamer or any other project without the user's approval of the exact
  text: draft it, show it, wait. Reading is fine.
- **No pull requests upstream on our own initiative** (agreed in
  FDH2/UxPlay#591). When the maintainer asks for a change, prepare a small pull
  request against upstream's `master`: branch from `upstream`, cherry-pick.
  Otherwise a fix is best offered as an issue with a short description and a
  patch to apply by hand — again only with the user's approval.
- **Security bugs in code shared with upstream**: ask the user how to disclose
  them before anything public (commit, pull request, issue) describes them.
  Upstream has no private reporting channel.
- **Upstream rewrites its `master`** and re-creates commits: check whether
  something is upstream by content (`git cherry`, `grep`), never by SHA.
- **Take upstream fixes one by one**: cherry-pick onto a `fix/…` branch and
  follow the flow above. A full merge of `upstream` only rarely, e.g. at an
  upstream release.

## Build and test

In a Debian trixie aarch64 container, like the Pis: image
`uxplay-select-test-deps`, OrbStack must run (`orb start`).

```sh
docker run --rm -v "$PWD":/src:ro uxplay-select-test-deps sh -c '
  cmake -S /src -B /build -DCMAKE_BUILD_TYPE=Release -DNO_X11_DEPS=ON &&
  cmake --build /build -j && cd /build && ctest --output-on-failure'
```

- `ctest` runs the fork's own suite in `tests/`; upstream has none.
- For memory work, a second build with
  `-fsanitize=address,undefined -fno-sanitize-recover=all` in
  `CMAKE_C_FLAGS`, `CMAKE_CXX_FLAGS` and `CMAKE_EXE_LINKER_FLAGS`. GStreamer's HLS demuxer leaks on its own in
  `hls_http`: run `ctest` with `LSAN_OPTIONS=suppressions=<file>`, the file
  holding `leak:libgstadaptivedemux2`.
- The version is `1.74-ha` (`uxplay.cpp`, `uxplay.1`); `uxplay.spec` keeps
  `1.74`, since RPM forbids a hyphen in a version.
- A display run goes through the ha-display repository and needs the user. To
  measure memory there, sample VmRSS, RssAnon and VmSwap of the `uxplay`
  process every 20 s during one session (zram swap can hide a leak from RSS);
  the first ~5 min are warm-up. One UxPlay process runs from one session to
  the next, so a leak adds up across sessions: note the process's PID and its
  memory before the first session a measurement covers.

## Intended behaviour

- `-hls-select` is the fork's own option and stays; upstream's `-rpi` and
  `-custom` filters do not replace it.
- The YouTube app sends the audio track picked in it as
  `PUT /setProperty?selectedMediaArray`, and the fork follows it, also during
  a video. There a switch starts with `playlistRemove`, so the removed video is
  kept in case the client asks for it again; the comments in
  `lib/http_handlers.h` describe the sequence.
- HLS with FairPlay Streaming keys (`#EXT-X-KEY` with an `skd://` URI, e.g.
  Prime Video) cannot play: the keys go only to Apple-certified devices. Don't
  debug it; failing fast with a clear message is the only useful work.
