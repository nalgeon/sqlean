-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/uuid

-- uuid4
select '1_01', uuid4() like '________-____-4___-____-____________';
select '1_02', gen_random_uuid() like '________-____-4___-____-____________';

-- uuid_str
select '2_01', uuid_str('d5a80b20-0d8f-11e5-b8cb-080027b6ec40') = 'd5a80b20-0d8f-11e5-b8cb-080027b6ec40';
select '2_02', uuid_str('d5a80b200d8f11e5b8cb080027b6ec40') = 'd5a80b20-0d8f-11e5-b8cb-080027b6ec40';
select '2_03', uuid_str('{d5a80b20-0d8f-11e5-b8cb-080027b6ec40}') = 'd5a80b20-0d8f-11e5-b8cb-080027b6ec40';
select '2_04', uuid_str('D5A80B20-0D8F-11E5-B8CB-080027B6EC40') = 'd5a80b20-0d8f-11e5-b8cb-080027b6ec40';
select '2_05', uuid_str(randomblob(16)) like '________-____-____-____-____________';
select '2_06', uuid_str(uuid4()) like '________-____-4___-____-____________';
select '2_07', uuid_str(uuid7()) like '________-____-7___-____-____________';
select '2_08', uuid_str('hello') is null;
select '2_09', uuid_str('') is null;
select '2_10', uuid_str(null) is null;

-- uuid_blob
select '3_01', typeof(uuid_blob('d5a80b20-0d8f-11e5-b8cb-080027b6ec40')) = 'blob';
select '3_02', typeof(uuid_blob(uuid4())) = 'blob';
select '3_03', typeof(uuid_blob(uuid7())) = 'blob';
select '3_04', typeof(uuid_blob(randomblob(16))) = 'blob';
select '3_05', uuid_blob('hello') is null;
select '3_06', uuid_blob('') is null;
select '3_07', uuid_blob(null) is null;

-- uuid7
select '4_01', uuid7() like '________-____-7___-____-____________';

-- uuid7_timestamp_ms
select '5_01', uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2') = 1717777901001;
select '5_02', uuid7_timestamp_ms(uuid_blob('018ff38a-a5c9-712d-bc80-0550b3ad41a2')) = 1717777901001;
select '5_03', uuid7_timestamp_ms('00000000-0000-7000-0000-000000000000') = 0;
select '5_04', uuid7_timestamp_ms('ffffffff-ffff-7000-0000-000000000000') = 281474976710655;
select '5_05', typeof(uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2')) = 'integer';
select '5_06', typeof(uuid7_timestamp_ms(uuid7())) = 'integer';
select '5_07', uuid7_timestamp_ms('hello') is null;
select '5_08', uuid7_timestamp_ms('') is null;
select '5_09', uuid7_timestamp_ms(null) is null;
select '5_10', uuid7_timestamp_ms('b2df66e7-bd9a-45f4-8c0d-b9fd73cc9f18') is null;
select '5_11', uuid7_timestamp_ms(uuid4()) is null;
