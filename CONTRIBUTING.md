# Contributing to Stygian

Thanks for taking the project seriously.

## Code Style

Follow the style rules in [docs/governance/syntax_bible.md](docs/governance/syntax_bible.md).

That document is the source of truth for formatting, naming, comments, and file structure. If a patch fights the style bible, the patch loses.

## DDI Contract

Stygian is invalidation-driven. That is not optional.

Before opening a PR, read [docs/governance/dod_ddi_nonnegotiables.md](docs/governance/dod_ddi_nonnegotiables.md). Any change that reintroduces input-driven render submission, per-frame blind rebuilds, or hidden busy work in clean-frame paths will be rejected.

Key rule:

- Do not make rendering happen just because input arrived.
- Preserve eval-only behavior and deep-wait idle behavior.
- Keep dirty-range uploads proportional to actual mutation.

## Review Expectations

Use [docs/governance/review_checklist.md](docs/governance/review_checklist.md) before asking for review.

Review is expected to check:

- behavior regressions
- DDI contract violations
- safety and misuse handling
- platform impact
- docs and test coverage for non-trivial changes

## Building

Windows:

- `powershell -NoProfile -ExecutionPolicy Bypass -File compile\run.ps1 -Target <target>`

Linux and macOS:

- `compile/run.sh --target <target>`

The build manifest lives in [compile/targets.json](compile/targets.json).

## Tests

Windows:

- Run the full stack with `powershell -NoProfile -ExecutionPolicy Bypass -File tests\run_all.ps1`

Linux and macOS:

- Build individual targets with `compile/run.sh --target tier1_safety`
- Build `tier2_runtime` and `tier3_misuse` the same way

Linux CI also attempts best-effort headless runtime coverage under `xvfb`.

## AI Transparency

Stygian explicitly discloses AI-assisted contribution policy in [docs/governance/contributor_attribution_policy.md](docs/governance/contributor_attribution_policy.md).

Rules:

- AI assistance is allowed and disclosed.
- Human review is required for every merge.
- Contributors remain responsible for correctness, integration quality, and public-facing claims.
