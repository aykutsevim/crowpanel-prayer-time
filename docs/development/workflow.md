# Development workflow

The firmware is built one slice at a time, in the order of [`../specification/06-roadmap.md`](../specification/06-roadmap.md), with GitHub Spec Kit and Claude Code. Each slice is one Spec Kit feature on its own branch, with its records in `specs/NNN-name/`. The prompts for every step of every slice are in [`prompts/`](prompts/).

## Before the first slice

- The exact board model and revision, read from the board (Slice 1 records it).
- A development machine with Git, Python 3.11 or newer, `uv`, PlatformIO Core and Claude Code.
- A USB-C data cable, a 2.4 GHz Wi-Fi network and a phone.
- Answers, when convenient, to the open questions at the end of [`../specification/01-product-brief.md`](../specification/01-product-brief.md). Defaults are in place for all of them.

Slice 0 ([`prompts/00-bootstrap.md`](prompts/00-bootstrap.md)) set up the repository, Spec Kit and the constitution; it is done.

## The loop for every slice

Each slice prompt file contains the same numbered steps. A step that starts with a slash command is pasted into Claude Code as it is. In this installation the commands are spelled with a hyphen (`/speckit-specify`); the prompt files use the dotted form (`/speckit.specify`), which means the same command.

| Step | Command | What happens |
|---|---|---|
| 1 | `/speckit-specify …` | creates the feature branch and `spec.md` from the prompt |
| 2 | `/speckit-clarify …` | Claude asks up to five questions; answer them or accept the defaults suggested in the slice file |
| 3 | `/speckit-plan …` | research, technical plan, data model, contracts |
| 4 | `/speckit-tasks` | ordered task list |
| 5 | `/speckit-analyze` | consistency check; fix findings at their source and repeat until clean |
| 6 | `/speckit-implement …` | builds the slice, in the stages the slice file suggests |
| 7 | owner checklist | the owner checks the device and answers the yes/no questions |
| 8 | `/speckit-converge` | confirms nothing was missed; repeat step 6 if it appends tasks |
| 9 | merge | merge the feature branch, tag if the slice says so |

## Habits that make this work

- **Start each slice in a fresh Claude Code session.** The specification, plan and `CLAUDE.md` carry the context; a long session does not need to.
- **Let Claude verify what it can before asking the owner.** Host tests and rendered snapshots come first. The owner is asked only for things that need eyes, ears or a phone.
- **Keep the documents true.** When something in `docs/` turns out to be wrong, correct it in the same branch and note it in [`../README.md`](../README.md#document-history).
- **Run `python tools/check.py` before every commit.**
