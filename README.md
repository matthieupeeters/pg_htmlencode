# pg_htmlencode

Fast way to encode strings to include in html. Replaces:

" => &quot;
& => &amp;
' => &apos;
< => &lt;
> => &gt;

and leaves the rest alone.

## Installation:

```
git clone https://github.com/matthieupeeters/pg_htmlencode
make USE_PGXS=1 
sudo make USE_PGXS=1 install
```
