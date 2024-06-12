-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/uuid

-- uuid4
select '01', uuid4() like '________-____-4___-____-____________';
select '02', gen_random_uuid() like '________-____-4___-____-____________';

-- uuid_str
select '03', uuid_str('d5a80b20-0d8f-11e5-b8cb-080027b6ec40') = 'd5a80b20-0d8f-11e5-b8cb-080027b6ec40';
select '04', uuid_str('d5a80b200d8f11e5b8cb080027b6ec40') = 'd5a80b20-0d8f-11e5-b8cb-080027b6ec40';
select '05', uuid_str('{d5a80b20-0d8f-11e5-b8cb-080027b6ec40}') = 'd5a80b20-0d8f-11e5-b8cb-080027b6ec40';
select '06', uuid_str('D5A80B20-0D8F-11E5-B8CB-080027B6EC40') = 'd5a80b20-0d8f-11e5-b8cb-080027b6ec40';
select '07', uuid_str(randomblob(16)) like '________-____-____-____-____________';
select '08', uuid_str(uuid4()) like '________-____-4___-____-____________';
select '09', uuid_str('hello') is null;
select '10', uuid_str('') is null;
select '11', uuid_str(null) is null;

-- uuid_blob
select '12', typeof(uuid_blob('d5a80b20-0d8f-11e5-b8cb-080027b6ec40')) = 'blob';
select '13', typeof(uuid_blob(uuid4())) = 'blob';
select '14', uuid_blob('hello') is null;
select '15', uuid_blob('') is null;
select '16', uuid_blob(null) is null;

-- uuid7
select '17', uuid7() like '________-____-7___-____-____________';

-- uuid7_timestamp_ms
select '18', uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2') = 1717777901001;
select '19', uuid7_timestamp_ms('00000000-0000-7000-0000-000000000000') = 0;
select '20', uuid7_timestamp_ms('ffffffff-ffff-7000-0000-000000000000') = 281474976710655;
select '21', typeof(uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2')) = 'integer';
select '22', typeof(uuid7_timestamp_ms(uuid7())) = 'integer';
select '23', uuid7_timestamp_ms('hello') is null;
select '24', uuid7_timestamp_ms('') is null;
select '25', uuid7_timestamp_ms(null) is null;
select '26', uuid7_timestamp_ms('b2df66e7-bd9a-45f4-8c0d-b9fd73cc9f18') is null;
select '27', uuid7_timestamp_ms(uuid4()) is null;
