# Teleport controls

Local implementation for game 1.0.0.2976. In-game acceptance is pending; the
runtime has only been built and exercised in owned offline fixtures.

Controls work while playing once the matching runtime is installed before game
startup. Installing/updating its ASI requires closing the game.

- **Save current position:** capture native global position and facing into
  one of 16 named slots. This does not move the player. Names allow 47 UTF-8
  bytes.
- **Return to saved position:** submit the normal field transition to the
  captured point. Its FieldInfo must match the current character's field.
- **Map pin / Waypoint:** select from the current player's captured lists.
  Mainland targets need a native ground hit and valid character placement.
  Alternate/unknown layers and unavailable/unsuitable geometry are rejected.
  A marker's map Y value is never treated as terrain height.
- **Remove saved position:** delete that bookmark.

`Native queued` means the game accepted its field operation, not that loading
finished or the player arrived. `Position observed` is a separate later native
observation. A world/player change, quest spawn override, timeout or fault can
leave the result uncertain. The tool does not auto-repeat uncertain commands.

Messages distinguish missing marker, unsupported world/layer, missing geometry,
unsuitable placement, changed marker and unavailable runtime. Failed or stale
lists must refresh rather than appear to be new successful captures.

Bookmarks live in Workbench's own local application data, not the game save.
Dirty storage remains visible after a failed explicit write. Polling does not
retry; capturing a position again or removing a slot allows one new attempt.
