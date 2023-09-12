SET TIME ZONE UTC;

SELECT uuid_v7_to_timestamptz('018a8920-f523-774f-bbc3-526d11fee5d5'::uuid);

SELECT uuid_v7_to_timestamptz('018a8920-f523-774f-bbc3-526d11fee5d5'::uuid, 12);
