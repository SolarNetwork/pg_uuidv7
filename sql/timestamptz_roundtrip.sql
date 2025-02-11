-- verify timestamp encode/decode roundtrip with random data
WITH data AS (
	SELECT ts, uuid_v7_to_timestamptz(uuid_timestamptz_to_v7(ts,false,12),12) AS uts
		FROM generate_series(
			  '2023-09-01 11:22:33.000000Z'::timestamptz
			, '2023-09-01 11:22:33.999999Z'::timestamptz
			, INTERVAL '1 microsecond') ts
)
SELECT * FROM data WHERE ts <> uts;

-- verify timestamp encode/decode roundtrip with zeroed data
WITH data AS (
	SELECT ts, uuid_v7_to_timestamptz(uuid_timestamptz_to_v7(ts,true,12),12) AS uts
		FROM generate_series(
			  '2023-09-01 11:22:33.000000Z'::timestamptz
			, '2023-09-01 11:22:33.999999Z'::timestamptz
			, INTERVAL '1 microsecond') ts
)
SELECT * FROM data WHERE ts <> uts;

-- verify timestamp encode/decode roundtrip with 10-bit precision and zeroed data
WITH data AS (
	SELECT ts, uuid_v7_to_timestamptz(uuid_timestamptz_to_v7(ts,true,10),10) AS uts
		FROM generate_series(
			  '2023-09-01 11:22:33.000000Z'::timestamptz
			, '2023-09-01 11:22:33.999999Z'::timestamptz
			, INTERVAL '1 microsecond') ts
)
SELECT * FROM data WHERE ts <> uts;
