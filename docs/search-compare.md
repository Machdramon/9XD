# Find / Coincidences and Compare Data

Both inspectors belong to Hex Viewer and keep separate results for each document.

## Find / Coincidences

Open **Hex Viewer > Find / Coincidences > Find**. Choose hexadecimal wildcards, byte regex, literal text or text regex. Text uses the viewer's current encoding.

- Search the entire document, a specified region, the contiguous selection or from the cursor to the end.
- Text/regex searches accept regions up to **1 MiB**. Hexadecimal searches stream the requested range in blocks.
- Case sensitivity applies to text/regex. Hexadecimal matching supports `??` for a byte and `A?` for a nibble wildcard.
- Enable overlapping results when needed. Absolute byte alignment filters match offsets (for example, alignment 4 accepts offsets divisible by four).
- Up to 10,000 matches are retained. An incomplete-search message appears if further matches exceed the chosen limit.
- Select a result or use previous/next to navigate. Copy results as tab-separated offsets and lengths, or use Mark to create a formatting rule.
- Stop cancels a search. Editing the document or changing search options invalidates previous results.

## Compare Data

Open **Hex Viewer > Compare Data > Compare**, choose a reference file and click Compare.

The current document includes unsaved edits and RAM content. The reference is read from disk. Comparison runs in blocks in the background, with progress and cancellation. It groups consecutive differing bytes and includes extra bytes when lengths differ.

This is a **positional byte comparison**: bytes at the same offsets are compared. It does not realign inserted/deleted regions, merge content or write either source.

Select a range or use Next/Previous difference to navigate. Bytes found only in the reference have no corresponding selectable byte in the current document; the inspector explains this. Copy results exports offsets and lengths to the clipboard.

Results are invalidated after editing the current document or changing the reference path. At most 10,000 difference ranges are retained; exceeding the limit stops comparison with an explicit incomplete-results message. Changes to files detected during reads invalidate the calculation. As with other readers, metadata checks do not replace an exclusive file lock or immutable copy.
