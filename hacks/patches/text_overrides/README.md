# Localized text overrides

Add UTF-8 `*.text-override.toml` files to a patch's directory and enable `text_overrides` alongside that patch:

```toml
[TEXT_ID_STAT_CORRUPTION_RESISTANCE]
# Correct the German label.
de = "% Verderbnisresistenz"
```

- Use `TextId` names and language keys `en`, `fr`, `de`, `it`, `es`.
- Missing languages retain ROM text. `""` is an explicit empty replacement.
- Write accents directly. `\n` and placeholders `@1` through `@4` are supported.
- Unknown IDs/languages, duplicate ID/language pairs, and unsupported glyphs are errors.
- Strings must fit 1024 encoded bytes including NUL, plus the caller's buffer and UI limits.

`make hack` converts selected patches' files automatically. To convert separately:

```sh
uv run rotkit hack text-overrides
```

## Review

```sh
uv run rotkit analysis decode-text show TEXT_ID_STAT_CORRUPTION_RESISTANCE
uv run rotkit analysis decode-text diff -o local/text-overrides.diff
```

The diff compares against the base ROM and includes all five languages as context. Use `-l de` to restrict languages.
Both conversion and diff accept `--patches "text_overrides"` using `HACKS` syntax, including `@cfg`.
The default is the `HACKS` environment variable or `hacks/patches.cfg`.
Use `--source DIR` instead to read a single directory.
