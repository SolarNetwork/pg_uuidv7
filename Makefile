MODULES = pg_uuidv7
EXTENSION = pg_uuidv7
DATA = pg_uuidv7--1.3.sql

REGRESS = setup \
			uuid_timestamptz_to_v7 \
			uuid_v7_to_timestamptz

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
REGRESS_OPTS = --dbname=$(EXTENSION)_regression
