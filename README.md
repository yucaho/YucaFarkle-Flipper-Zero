# Farkle Lite for Flipper Zero

A compact Farkle dice game for Flipper Zero built in C with the Flipper/Furi SDK, designed for a 128x64 monochrome display and hardware button controls.

## Screenshots / GIFs
Add media in `docs/screenshots/`:
- `docs/screenshots/gameplay.png`
- `docs/screenshots/hot-dice.gif`
- `docs/screenshots/farkle-screen.png`

## Features
- Six dice rendered in a 2x3 grid.
- Cursor navigation and per-die selection via D-pad and OK button.
- Exact-selection Farkle scoring validation.
- Partial rerolls only animate unheld dice.
- Full-screen FARKLE takeover and HOT DICE celebration overlay.
- Procedural animation and effects (no image assets required).

## Controls
| Input | Action |
|---|---|
| D-pad short | Move cursor across six dice |
| OK short | Toggle selected die |
| OK long | Roll available (unheld) dice |
| Right long | Score selected dice |
| Down long | Bank turn score and start next turn |
| Back | Exit app |

## Scoring Rules
| Combination | Score |
|---|---:|
| Single 1 | 100 |
| Single 5 | 50 |
| Three 1s | 1000 |
| Three 2s-6s | Face × 100 |
| Four of any number | 1000 |
| Five of any number | 2000 |
| Six of any number | 3000 |
| Straight 1-6 | 1500 |
| Straight 1-5 | 500 |
| Straight 2-6 | 750 |
| Three pairs | 1500 |
| Four of a kind + one pair | 1500 |
| Two triplets | 2500 |

> Selection scoring is exact: if selected dice include non-scoring dice, scoring fails.

## Build Instructions (PowerShell)
```powershell
git clone <repo-url>
cd <repo-folder>
ufbt
ufbt launch
```

If `ufbt` is not recognized:

```powershell
py -m pip install --upgrade ufbt
py -m ufbt
py -m ufbt launch
```

## Install / Launch Notes
1. Connect Flipper Zero by USB.
2. Confirm the device is detected in qFlipper or via ufbt.
3. Run `ufbt launch` from this repository folder.

## Development Notes
- Main source file: `applications_user/farkle_lite/farkle_lite.c`.
- App state is centralized in `FarkleLiteApp`.
- Dice selection/hold logic uses bitmasks for compact state updates.
- Animation frames are pre-generated per animating die for consistent 1-second rolls.

## Known Limitations / Roadmap
- No persistent high score yet.
- No difficulty/custom rules screen yet.
- Could add optional sound profile toggles and richer haptics.
- Could add test harnesses for scoring combinations.

## Portfolio Summary
This project demonstrates:
- C-based app development for constrained hardware.
- Event-driven input handling with a message queue.
- Manual canvas rendering on a 128x64 monochrome display.
- Compact game state management for turn flow and overlays.
- Bitmask-driven dice selection and scoring validation.
- Procedural animation/effects and practical ufbt build-debug workflow.
