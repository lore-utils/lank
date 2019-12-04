# lank
a symlink utility for all things symlink

## Use cases
 - Retarget a single symlink
 - Bulk target rename
 - Regex based bulk target rename
 -
## Usage

```
lank - a symlink utility for all things symlink
Usage:
    lank SRC DEST
    lank MATCH REPLACEMENT FILES...
    lank <OPTIONS>
Options:
    -[-s]rc - the src symlink file to modify
    -[-d]est - the new destination for the input symlink

    -[-m]atch - the regex pattern to apply to the target of the symlinks
    -[-r]eplacement - the replacement pattern for the match

    -[-h]elp - this message
Notes:
    Uses the PCRE2 regex engine for replacements.
```
