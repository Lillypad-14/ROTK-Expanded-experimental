# Community documentation

This repository combines the original-game decompilation with the experimental ROTK Expanded mod. They have separate build outputs and verification requirements. Start with the guide for what you want to do.

| Goal | Start here |
| --- | --- |
| Pick up the work where it stands | [Status](status.md) |
| Learn each hero's mechanic and skills | [Heroes](heroes.md) |
| Play and test the map-based dungeon prototype | [Gauntlet](gauntlet.md) |
| Browse boon effects, duos and hero-specific rewards | [Gauntlet boon catalog](gauntlet-boons.md) |
| See every mod options page and Twist | [Mod options](mod-options.md) |
| Understand what Expanded changes | [Expansion overview](expanded-changes.md), then [engine changes](engine-changes.md) |
| See how the combat system was extended | [Engine changes: combat](engine-changes.md#combat-system) |
| Follow the whole workflow, Ghidra to release | [Pipeline](pipeline.md) |
| See what has been decompiled | [Decompilation status](decompilation.md), then [source inventory](source-inventory.md) |
| Build from your own ROM | [Setup](setup.md) and [workflow](workflow.md) |
| Understand the original mod's features and saves | [Upstream mod user guide](romhack-user.md) |
| Contribute code, documentation or bug reports | [Contributing](../CONTRIBUTING.md) |
| Write a patch | [Hack authoring](../hacks/README.md) and [mod scenes](../hacks/docs/mod-scenes.md) |
| Add or change hero abilities | [Shared ability support](ability-support.md) and [hero authoring notebook](wip-docs/howto-heroes-skills.md) |
| Create effect artwork | [Skill effects](skill-effects.md) and [Effect Studio](effect-editor.md) |
| Define cast timing and projectiles | [Skill deliveries](skill-deliveries.md) |
| Measure and reduce slowdown | [Performance changes](performance.md) and [busy-fight benchmark](test-arena.md#busy-fight-benchmark) |
| Review the latest bug pass | [October 25 bug audit](bug-audit.md) |
| Test gameplay changes | [Test arena](test-arena.md), [Ability Bench](ability-bench.md) and [combat validation](combat-validation.md) |
| Review the four-hero upgrade audit | [Hero move checks](hero-move-checks.md) |
| Follow source naming and layout | [Conventions](conventions.md) |
| Review original mod release history | [Upstream changelog](romhack-changelog.md) |

## Reading status claims

- **Mapped:** a function has a name/address in the symbol store. Its implementation may still come from the base ROM.
- **C implementation:** readable original-game C exists under `src/c/`. The inventory reports manifest membership separately.
- **Byte verified:** a matching check or reconstruction compared compiled code with the original ROM. A source listing alone does not prove this.
- **Host tested:** automated checks exercised code with substitutes for native game services. This does not certify animations, normal input or a whole encounter.
- **In-game tested:** behavior was observed in an emulator or on hardware. State the build, test conditions and limits when reporting it.
- **Experimental:** implementation exists, but balance and complete gameplay compatibility remain open.

The [source inventory](source-inventory.md) is generated from the checkout and can be refreshed without a ROM. Historical validation results in other guides retain their original dates and scope.

## Research notes

`docs/wip-docs/` contains inherited research, compiler probes and design notes. It is useful background, but can describe old implementations or unconfirmed candidates. Follow links to current code before relying on it. New speculative research belongs in gitignored `local/wip-docs/` according to the repository rules.

The [decompilation handoff](decomp-handoff.md) records a particular development session. Its local tooling details and next-task list are historical context rather than a current installation guide.

[Saruman spell balance](saruman-balance.md) lists current costs, damage and resource refunds.
