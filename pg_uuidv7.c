#include "postgres.h"

#include "fmgr.h"
#include "port/pg_bswap.h"
#include "utils/uuid.h"
#include "utils/timestamp.h"

#include <time.h>

/*
 * Number of microseconds between unix and postgres epoch
 */
#define EPOCH_DIFF_USECS ((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * USECS_PER_DAY)

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(uuid_generate_v7);

Datum uuid_generate_v7(PG_FUNCTION_ARGS)
{
	pg_uuid_t *uuid = palloc(UUID_LEN);
	uint8_t extra_ts_p = 0;
	struct timespec ts;
	uint64_t data;
	uint16_t tmp;

	if (!PG_ARGISNULL(0))
		extra_ts_p = PG_GETARG_INT32(0);

	if (!pg_strong_random(&uuid->data[6], UUID_LEN - 6))
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not generate random values")));

	/*
	 * Set first 48 bits to unix epoch timestamp; optionally
	 * set fractional milliseconds starting from 52nd bit
	 */
	if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not get CLOCK_REALTIME")));

	data = (((uint64_t)ts.tv_sec * 1000) + ((uint64_t)ts.tv_nsec / 1000000)) << 16;
	if (extra_ts_p > 1 && extra_ts_p <= 12) {
		data |= ((((uint64_t)ts.tv_nsec << extra_ts_p) / 1000000)
						& ((1 << extra_ts_p) - 1)) << (12 - extra_ts_p);
		if (extra_ts_p < 12) {
			// preserve generated random bits beyond extra_ts_p
			memcpy(&tmp, &uuid->data[6], 2);
			data |= (tmp & ~((~1) << (12 - extra_ts_p - 1)));
		}
		data = pg_hton64(data);
		memcpy(&uuid->data[0], &data, 8);
	} else {
		data = pg_hton64(data);
		memcpy(&uuid->data[0], &data, 6);
	}

	/*
	 * Set magic numbers for a "version 7" UUID, see
	 * https://www.ietf.org/archive/id/draft-ietf-uuidrev-rfc4122bis-00.html#name-uuid-version-7
	 */
	uuid->data[6] = (uuid->data[6] & 0x0f) | 0x70; /* 4 bit version [0111] */
	uuid->data[8] = (uuid->data[8] & 0x3f) | 0x80; /* 2 bit variant [10]   */

	PG_RETURN_UUID_P(uuid);
}

PG_FUNCTION_INFO_V1(uuid_v7_to_timestamptz);

Datum uuid_v7_to_timestamptz(PG_FUNCTION_ARGS)
{
	pg_uuid_t *uuid = PG_GETARG_UUID_P(0);
	uint8_t extra_ts_p = 0;
	uint64_t ts;
	uint64_t extra_ts;

	if (!PG_ARGISNULL(1))
		extra_ts_p = PG_GETARG_INT32(1);

	memcpy(&ts, &uuid->data[0], 6);
	ts = pg_ntoh64(ts) >> 16;
	ts = 1000 * ts - EPOCH_DIFF_USECS;

	if (extra_ts_p > 1 && extra_ts_p <= 12) {
		// add fractional millis as microseconds to ts, rounding up
		memcpy(&extra_ts, &uuid->data[6], 2);
		extra_ts = ((pg_ntoh64(extra_ts) >> 48) & 0xFFF) >> (12 - extra_ts_p);
		ts += (uint64_t)((((double)extra_ts + 1.0) / (double)(1 << extra_ts_p)) * 1000);
	}

	PG_RETURN_TIMESTAMPTZ(ts);
}

PG_FUNCTION_INFO_V1(uuid_timestamptz_to_v7);

Datum uuid_timestamptz_to_v7(PG_FUNCTION_ARGS)
{
	pg_uuid_t *uuid = palloc(UUID_LEN);
	bool zero = false;
	uint8_t extra_ts_p = 0;
	uint64_t tms;
	uint16_t tmp;

	TimestampTz ts = PG_GETARG_TIMESTAMPTZ(0);

	if (!PG_ARGISNULL(1))
		zero = PG_GETARG_BOOL(1);

	if (!PG_ARGISNULL(2))
		extra_ts_p = PG_GETARG_INT32(2);

	if (zero)
		memset(uuid, 0, UUID_LEN);
	else if (!pg_strong_random(&uuid->data[6], UUID_LEN - 6))
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not generate random values")));

	tms = ((uint64_t)ts + EPOCH_DIFF_USECS) / 1000;
	tms = pg_hton64(tms << 16);
	memcpy(&uuid->data[0], &tms, 6);

	if (extra_ts_p > 1 && extra_ts_p <= 12) {
		tms = ((((uint64_t)ts << extra_ts_p) / 1000)
						& ((1 << extra_ts_p) - 1)) << (12 - extra_ts_p);
		if (extra_ts_p < 12) {
			// preserve generated random bits beyond extra_ts_p
			memcpy(&tmp, &uuid->data[6], 2);
			tms |= (tmp & ~((~1) << (12 - extra_ts_p - 1)));
		}
		tms = pg_hton64(tms << 48);
		memcpy(&uuid->data[6], &tms, 2);
	}

	/*
	 * Set magic numbers for a "version 7" UUID, see
	 * https://www.ietf.org/archive/id/draft-ietf-uuidrev-rfc4122bis-00.html#name-uuid-version-7
	 */
	uuid->data[6] = (uuid->data[6] & 0x0f) | 0x70; /* 4 bit version [0111] */
	uuid->data[8] = (uuid->data[8] & 0x3f) | 0x80; /* 2 bit variant [10]   */

	PG_RETURN_UUID_P(uuid);
}
