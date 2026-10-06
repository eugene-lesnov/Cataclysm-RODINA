# Translation in RODINA

RODINA uses gettext for player-facing text. The Russian catalog is stored in `lang/po/ru.po` and compiled into a `.mo` file during localized builds. The build does not fetch translations from an external service.

Source strings in C++ and JSON remain the keys for translation. Changing one can invalidate an existing catalog entry, so check the Russian catalog when changing player-facing text.

## C++ strings

Use `_()` for a simple string, `pgettext()` when a string needs context, and `n_gettext()` for plural forms. Strings passed as literals can be extracted automatically. A string assembled only at runtime may not be found by the extractor.

For strings stored before they are displayed, use the `translation` class and its `to_translation()`, `pl_translation()`, or `no_translation()` helpers as appropriate. Translate the value when it is displayed, rather than during static initialization.

## `translation`

JSON fields read as a `translation` may contain a plain string or an object with context and plural forms:

```jsonc
"name": "bar"
```

```jsonc
"name": { "ctxt": "foo", "str": "bar", "str_pl": "bars" }
```

`ctxt` is optional. `str_sp` can be used when the singular and plural source strings are identical. A `//~` entry provides context for translators; `"//~": "NO_I18N"` marks a string that should not be extracted. These forms work only where the JSON field is read through `translation`.

When adding a new translatable JSON field, update `lang/extract_json_strings.py` so it is included in the catalog template. `lang/update_pot.sh`, `lang/merge_po.sh`, and `lang/compile_mo.sh` handle extraction, merging, and compilation. For example, `lang/compile_mo.sh ru` compiles the Russian catalog.
