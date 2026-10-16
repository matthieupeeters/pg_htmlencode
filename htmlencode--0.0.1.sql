create function htmlencode (str TEXT)
returns TEXT
as 'MODULE_PATHNAME' language 'c' immutable strict;
