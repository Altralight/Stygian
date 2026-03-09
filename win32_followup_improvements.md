# Win32 Follow-Up Improvements

This file captures the next serious improvement work that should happen after
the recent borderless maximize fix. The short version: the current code now
passes and the flake is under control, but there are still some obvious places
where the design is carrying too much risk or too much weight.

## 1. Split The Win32 Backend Into Focused Internal Units

### Why this needs to happen

`window/platform/stygian_win32.c` is still doing too many jobs in one file.
Right now it owns:

- window creation and teardown
- style and extended-style policy
- message dispatch
- borderless/manual maximize behavior
- titlebar behavior and system actions
- OpenGL pixel format and context helpers
- swap and vsync policy
- display/monitor tracking
- live resize / non-client drag behavior

That much logic in one file is how regressions hide. The borderless maximize
bug is a good example: the actual failure was not "maximize is broken" in some
simple isolated sense. The failure lived in the overlap between style policy,
Win32 message churn, and OpenGL-window behavior. A giant file makes those
cross-couplings harder to see and harder to test.

### What to split out

Use internal-only translation units or at least internal sections with a hard
boundary. The clean split would look like this:

- `window/platform/win32/stygian_win32_window.c`
  Owns window creation, destruction, core getters/setters, and common rect
  application helpers.
- `window/platform/win32/stygian_win32_messages.c`
  Owns the window proc, event queue integration, and Win32 message routing.
- `window/platform/win32/stygian_win32_borderless.c`
  Owns borderless maximize/restore/fullscreen, work-area queries, and any
  corrective geometry logic.
- `window/platform/win32/stygian_win32_titlebar.c`
  Owns titlebar hints, behavior policy, double-click behavior, and snap/menu
  actions.
- `window/platform/win32/stygian_win32_gl.c`
  Owns pixel format setup, WGL context helpers, swap interval handling, and
  present fallback behavior.
- `window/platform/win32/stygian_win32_monitor.c`
  Owns monitor keys, display-change serial logic, and monitor/work-area
  queries.

If the build system or current repo layout makes that annoying right now, the
minimum acceptable first step is a private header plus a few `static` helper
clusters moved behind clearer section boundaries. But the end state should be
real separation, not one giant "temporary" file forever.

### Practical sequencing

Do it in this order so the repo does not turn into a mess:

1. Extract monitor/work-area helpers and display-change tracking first.
2. Extract borderless/manual maximize helpers second.
3. Extract titlebar behavior and snap-menu logic third.
4. Extract WGL / swap / vsync code last.

That order keeps the highest-risk geometry logic small first, before touching
the GL path.

### Guard rails for the refactor

- Do not change public API names during this split.
- Keep `StygianWindow` layout stable unless there is a direct need to move
  fields.
- Add one tiny internal header for shared helper declarations instead of
  copy-pasting helper prototypes into multiple files.
- Preserve current behavior first; do not mix this with feature work.
- Keep the Win32-specific borderless tests running after each extraction step.

### Concrete success criteria

- The main Win32 file becomes mostly orchestration, not the dumping ground.
- A geometry bug can be traced by reading one focused file instead of a 1.5k+
  line slab.
- The borderless/manual maximize path no longer shares a file with raw WGL
  setup unless there is a very good reason.

## 2. Add Native Geometry Tracing For Borderless / Maximize Paths

### Why this needs to happen

The project already had present tracing, which helped a bit, but it did not
have geometry tracing when the borderless maximize flake showed up. That was a
real gap. The failure was:

- maximize request fired
- maximized state looked true
- final geometry was sometimes full-size at the old centered origin

That is exactly the sort of bug where you want a cheap native trace showing:

- requested rect
- actual rect after apply
- relevant Win32 messages arriving around the transition
- current styles and topmost state
- whether the window is in manual borderless maximize mode

Without that, diagnosis gets slower than it should be.

### What trace data should exist

Add a Win32 geometry trace path behind an env var, something in the spirit of:

- `STYGIAN_WIN32_GEOMETRY_TRACE=1`

When enabled, it should log only the transitions that matter, not every random
mouse move. Useful events:

- maximize request
- restore request
- fullscreen enter/exit
- manual rect apply
- `WM_GETMINMAXINFO`
- `WM_WINDOWPOSCHANGED`
- `WM_MOVE`
- `WM_SIZE`
- titlebar snap action apply

For each one, log a compact line with:

- event name
- window rect
- client rect if cheap
- target rect if one exists
- `borderless_manual_maximized`
- `fullscreen`
- `maximized`
- `IsZoomed(hwnd)`
- topmost bit

### Logging style

Keep it blunt and cheap. This should be debug scaffolding, not pretty prose.
Something like:

`[Stygian Win32] geom apply-rect target=(0,0)-(1920,1079) current=(700,370)-(2620,1449) manual=1 zoomed=0 topmost=0`

That kind of line is enough to answer the real question quickly: who moved the
window, and what state did we think we were in when it happened?

### Guard rails

- Do not make tracing allocate memory.
- Do not make tracing mutate behavior.
- Keep the hot path cheap when disabled.
- Avoid spamming logs for high-frequency messages unless the window is in a
  special geometry-changing mode.

### Concrete success criteria

- Reproducing a geometry regression gives native, ordered evidence.
- We can tell whether the bug is in our helper path or in late Win32 message
  churn.
- Future borderless/fullscreen issues stop being a blind debugging exercise.

## 3. Route Every Native Maximize/Restore Entry Point Through The Same Manual Borderless Path

### Why this needs to happen

Right now the code is much better than it was, but there is still a design
smell: multiple entry paths can influence maximize/restore behavior.

Today the explicit API calls are in good shape:

- `stygian_window_maximize(...)`
- `stygian_window_restore(...)`
- titlebar action routing
- fullscreen toggles

But Win32 also has its own native paths:

- `WM_SYSCOMMAND` with `SC_MAXIMIZE`
- `WM_SYSCOMMAND` with `SC_RESTORE`
- system menu actions
- shell-driven window management
- snap / zone interactions depending on shell behavior

If any of those bypass the manual borderless path, behavior can drift again.
That is how you get weird states where one route respects work-area logic and
another route falls back to default Win32 maximize semantics.

### What to implement

Audit the window proc and intercept native maximize/restore requests for
borderless windows. The goal is simple:

- if the window is borderless and the action is maximize, route to the same
  helper used by `stygian_window_maximize(...)`
- if the window is borderless and the action is restore, route to the same
  helper used by `stygian_window_restore(...)`

That means native titlebar/system-menu/user-shell actions all converge on one
behavioral path instead of each half-doing their own thing.

### Specific cases to inspect

- `WM_SYSCOMMAND` with `SC_MAXIMIZE`
- `WM_SYSCOMMAND` with `SC_RESTORE`
- `WM_SYSCOMMAND` with `SC_SIZE` if it can knock a manual-maximized borderless
  window into an odd intermediate state
- shell snap interactions that may arrive as a restore-plus-resize sequence

### Risks

There is one real risk here: being too aggressive and breaking native shell
behavior. So this should be done carefully.

The right approach is:

- intercept only when the window is actually borderless
- preserve native behavior for regular framed windows
- keep snap/menu behavior intact
- verify that system-menu maximize/restore still work and do not dead-end

### Tests that should exist after this

Add targeted runtime checks for:

- native/system maximize on borderless window reaches work area
- native/system restore returns to stored restore rect
- native maximize on framed window still uses ordinary Win32 behavior
- snap action after manual maximize still lands in the expected half/quarter
  rect

### Concrete success criteria

- There is exactly one behaviorally meaningful maximize path for borderless
  windows.
- API-triggered and native-triggered maximize/restore land in the same final
  state.
- Future fixes land in one place instead of getting patched three times.

## 4. Extra Notes From The Recent Investigation

These are not full backlog items, but they matter:

- The failing geometry signature was "correct size, stale origin." That is not
  random noise; it points straight at a resize-only overwrite after the desired
  move/resize.
- The focused repeatability test was useful because it separated raw window
  behavior from context-backed behavior. Keep that pattern. It saves time.
- The current corrective clamp for manual borderless maximize is practical and
  worth keeping, but it should not become an excuse to leave maximize routing
  fragmented forever.
- The current Win32 layer still mixes policy and mechanism too freely. Refactor
  pressure will keep coming back until that is cleaned up.

## 5. Recommended Execution Order

If this work gets scheduled, do it in this order:

1. Add geometry tracing.
2. Audit and unify native maximize/restore entry points.
3. Split the Win32 backend into smaller internal units.

Why this order:

- tracing makes the next two jobs safer
- unify behavior before moving files around
- refactor last, once behavior is pinned down and observable

## 6. Definition Of Done For This Backlog

This backlog is done when:

- borderless maximize behavior is observable through explicit native tracing
- API and native maximize/restore paths converge on the same manual logic
- the Win32 backend is split enough that geometry policy is readable in
  isolation
- tier coverage includes both focused geometry tests and native-entry tests
- the fix path for future borderless regressions is obvious instead of buried
  in one oversized source file
