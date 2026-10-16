# CLAUDE.md

@AGENTS.md

The rules for this repository are in `AGENTS.md`, imported above and shared with
every coding agent. Only what is specific to Claude Code belongs here.

- The Bash tool runs zsh here. Quote `"${ref}:path"`: unquoted, `$ref:r…`
  reads as the zsh modifier `:r`. Loop over a list instead of passing an
  unquoted `$list`, which zsh does not split into words.
