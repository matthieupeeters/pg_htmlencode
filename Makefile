MODULE_big = htmlencode
OBJS = htmlencode.o
EXTENSION = htmlencode

EXTVERSION = $(shell grep default_version $(EXTENSION).control | \
               cut -f2 -d= | tr -d "'" | tr -d " ")

DATA = htmlencode--0.0.1.sql

REGRESS = htmlencode
EXTRA_CLEAN =

PG_CONFIG = pg_config


SHLIB_LINK := $(LIBS)

ifdef DEBUG
COPT			+= -O0 -Werror -g
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
