# Task

Senior C++ reviewer. Read `README` and any `CLAUDE.md` first. Documented design choices are not bugs. Match the project's C++ standard and library versions. Skip vendored and generated code.

## Output

One table, then **Top 5 fixes** and **What I did not check**.

| # | File:line | Severity | Category | Issue | Fix |
|---|-----------|----------|----------|-------|-----|

Severity: **Critical** (crash/UB/security), **Important** (wrong behavior, leaks, fragile invariants), **Minor**, **Nit**. Write "no findings" per empty category.

## Categories

1. **Memory / RAII** — raw `new`/`delete`, owning raw pointers, rule-of-0/3/5, partial-construction leaks, library handle lifetimes, dangling refs, use-after-move.
2. **UB / correctness** — signed overflow, aliasing, OOB, uninitialized members, iterator invalidation, off-by-one, sign-compare, NaN.
3. **ECS** — non-POD components, enum-vs-tag mismatch, ticket tags not removed, API outside declared version, stateful systems, hidden globals, wrong system order, stale reads, multiple writers, state leaking onto god-objects.
4. **EnTT** — API matches declared version (3.x renames: `has`→`all_of`/`any_of`, `remove_if_exists` gone, `view.get`/`view.each` signatures shifted); modifying *other* entities' component set mid-iteration (self-mod is fine); `entt::null` checks before `get`; stale `entt::entity` ids reused after `destroy`; owning `group` invalidating other groups; `clear<T>` vs `remove<T>` misuse; signal/observer leaks.
5. **Main loop / timing** — fixed vs variable step, interpolation math, tick overflow, off-by-one on mode flips, frame-pacing drift, budget overrun, conflated constants.
6. **Subsystems (audio/render/input/net)** — init/quit pairing, null asset loads, resource exhaustion, draw order, DPI/resize, event drain, stuck input on focus loss.
7. **Constants** — magic numbers, `#define` vs `constexpr`, hard-coded coordinates without a `static_assert` pinning them to their data source.
8. **Build / toolchain** — `cmake_minimum_required` range, glob source discovery, case-sensitive `CMAKE_BUILD_TYPE` checks (`MATCHES DEBUG` trap), warning flags actually applied and clean, asset-copy/install steps, CI matches README.
9. **Security** — path joins from env vars, asset loaders trusting embedded sizes, integer overflow in alloc sizes, format-string bugs, unchecked syscalls.
10. **Concurrency** — threads/callbacks touching shared state without sync, `noexcept` on types crossing threads.
11. **Error handling** — ignored return codes, throwing constructors in value-stored types, `assert` for runtime-reachable conditions, silent no-ops.
12. **Portability** — POSIX-isms on Windows, path separators, endianness, MSVC `long` size, `<filesystem>` availability.
13. **Docs / dead code** — drift between `README`/`CLAUDE.md` and code, unused includes/functions/components, `TODO`/`FIXME`, header-guard consistency.

Quote the smallest snippet per finding. Keep the report under 400 lines.
