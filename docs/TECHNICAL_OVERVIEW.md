# TECHNICAL OVERVIEW

## Stack Overview
- Language: C
- SDK: Flipper/Furi SDK APIs
- Build tooling: `ufbt`
- UI: Flipper `Canvas` + `ViewPort`
- Input: `InputEvent` queue via `FuriMessageQueue`

## App Architecture
Single-file app entrypoint in:
- `applications_user/farkle_lite/farkle_lite.c`

Core components:
- `FarkleLiteApp` state struct
- Rendering callback (`render_callback`)
- Input handling loop in `farkle_lite_app`
- Scoring engine (`score_mask_exact`)
- Roll animation helper (`animate_roll`)

## State Management
`FarkleLiteApp` contains:
- Dice faces (`dice[6]`)
- Selection and held masks (`selected_mask`, `held_mask`)
- Cursor index
- Turn/bank scores
- `must_score_after_roll` rule gate
- Overlay mode (`none`, `hot dice`, `farkle`)

## Input / Event Queue Flow
1. ViewPort input callback pushes `InputEvent` into message queue.
2. Main loop consumes events.
3. Short presses manage cursor/selection.
4. Long presses run game actions (roll, score, bank).
5. State changes trigger `view_port_update`.

## Rendering Pipeline
- `render_callback` draws:
  - HUD (Turn/Bank)
  - 2x3 dice grid
  - selection/held/cursor indicators
  - contextual text (`Score before roll`)
- Overlay branch:
  - HOT DICE comic-style starburst lines + center burst
  - FARKLE vertical-line background with centered text

## Bitmask Design (Dice / Selection / Held)
- Six dice use bits 0-5.
- `selected_mask`: currently selected dice for commit.
- `held_mask`: already scored this turn, excluded from reroll.
- Unheld mask is derived as `FULL_MASK & ~held_mask`.

## Scoring Engine
`score_mask_exact` validates the selected subset only.
- Detects special six-dice sets first (straights, pairs/triplets specials).
- Applies n-of-a-kind scoring (6/5/4/3).
- Applies single 1 and single 5 rules.
- Rejects selection if any die remains non-scoring.

## Roll Animation Design
- Duration: ~1000 ms total.
- Pre-generates random face sequence per animating die (`anim_faces`).
- Iterates frames and updates display each frame.
- Applies only to dice in roll mask; held dice remain stable.

## HOT DICE and FARKLE Flow
- HOT DICE: when held mask reaches all six dice.
  - Show overlay briefly.
  - Reset held mask.
  - Full reroll and continue same turn.
- FARKLE: after roll with no scoring subsets in the rolled dice.
  - Brief reveal of failed roll.
  - Full-screen FARKLE overlay.
  - Turn score reset and full reroll for new turn.

## Future Improvements
- Split rendering/scoring/input into separate files.
- Add unit-testable scoring module.
- Add persistent stats/high score.
- Add optional sound/haptic settings menu.
