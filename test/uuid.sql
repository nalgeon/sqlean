-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/uuid

-- Canonical form: xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx, lowercase,
-- M is the version, N is the variant (8, 9, a or b for RFC 9562).
-- RFC 9562 examples:
--   v4 919108f7-52d1-4320-9bac-f847db4148a8
--   v7 017f22e2-79b0-7cc3-98c4-dc0c0c07398f, unix_ts_ms = 0x017f22e279b0 = 1645557742000
-- PostgreSQL example: a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11
-- 1733668387000 ms = 0x0193a6b038b8

-- uuid4
select '01_01', uuid4() like '________-____-4___-____-____________';
select '01_02', typeof(uuid4()) = 'text';
select '01_03', length(uuid4()) = 36;
select '01_04', (with t as materialized (select uuid4() u) select u = lower(u) from t);
select '01_05', (select count(*) from generate_series(1, 1000) where substr(uuid4(), 20, 1) not in ('8', '9', 'a', 'b')) = 0;
select '01_06', (select count(*) from generate_series(1, 1000) where uuid4() glob '*[^0-9a-f-]*') = 0;
select '01_07', (select count(distinct uuid4()) from generate_series(1, 1000)) = 1000;
-- not deterministic: each call gives a new value
select '01_08', uuid4() <> uuid4();
select '01_09', (with t as materialized (select uuid4() u) select uuid_str(u) = u from t);
-- gen_random_uuid is an alias
select '01_10', gen_random_uuid() like '________-____-4___-____-____________';
select '01_11', (select count(*) from generate_series(1, 1000) where substr(gen_random_uuid(), 20, 1) not in ('8', '9', 'a', 'b')) = 0;
select '01_12', gen_random_uuid() <> gen_random_uuid();

-- uuid7
select '02_01', uuid7() like '________-____-7___-____-____________';
select '02_02', typeof(uuid7()) = 'text';
select '02_03', length(uuid7()) = 36;
select '02_04', (with t as materialized (select uuid7() u) select u = lower(u) from t);
select '02_05', (select count(*) from generate_series(1, 1000) where substr(uuid7(), 20, 1) not in ('8', '9', 'a', 'b')) = 0;
select '02_06', (select count(*) from generate_series(1, 1000) where uuid7() glob '*[^0-9a-f-]*') = 0;
select '02_07', (select count(distinct uuid7()) from generate_series(1, 1000)) = 1000;
select '02_08', uuid7() <> uuid7();
-- the timestamp is the current time in milliseconds
select '02_09', abs(uuid7_timestamp_ms(uuid7()) - unixepoch('subsec') * 1000) < 1000;

-- uuid7(seconds)
select '03_01', uuid7(0) like '00000000-0000-7___-____-____________';
select '03_02', uuid7(1645557742) like '017f22e2-79b0-7___-____-____________';
select '03_03', uuid7(1733668387) like '0193a6b0-38b8-7___-____-____________';
select '03_04', uuid7_timestamp_ms(uuid7(1733668387)) = 1733668387000;
select '03_05', (select count(*) from generate_series(1, 1000) where substr(uuid7(0), 20, 1) not in ('8', '9', 'a', 'b')) = 0;
select '03_06', (select count(distinct uuid7(0)) from generate_series(1, 1000)) = 1000;
-- random bits make each call different, even with the same timestamp
select '03_07', uuid7(0) <> uuid7(0);
-- later timestamps sort after earlier ones
select '03_08', uuid7(1733668387) < uuid7(1733668388);
select '03_09', uuid_blob(uuid7(1733668387)) < uuid_blob(uuid7(1733668388));
-- the largest timestamp that fits in 48 bits: 281474976710000 ms = 0xfffffffffd70
select '03_10', uuid7(281474976710) like 'ffffffff-fd70-7___-____-____________';
-- timestamps outside the 48-bit range give NULL
select '03_11', uuid7(281474976711) is null;
select '03_12', uuid7(-1) is null;
select '03_13', uuid7(9223372036854775807) is null;
select '03_14', uuid7(-9223372036854775808) is null;
-- numeric text is converted: 1000 sec = 0x0f4240 ms
select '03_15', uuid7('1000') like '0000000f-4240-7___-____-____________';
-- other non-integer arguments give NULL
select '03_16', uuid7(null) is null;
select '03_17', uuid7(1000.0) is null;
select '03_18', uuid7(1.5) is null;
select '03_19', uuid7('abc') is null;
select '03_20', uuid7(x'01') is null;

-- uuid7_timestamp_ms
select '04_01', uuid7_timestamp_ms('017f22e2-79b0-7cc3-98c4-dc0c0c07398f') = 1645557742000;
select '04_02', uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2') = 1717777901001;
select '04_03', typeof(uuid7_timestamp_ms('018ff38a-a5c9-712d-bc80-0550b3ad41a2')) = 'integer';
select '04_04', typeof(uuid7_timestamp_ms(uuid7())) = 'integer';
select '04_05', uuid7_timestamp_ms('00000000-0000-7000-0000-000000000000') = 0;
select '04_06', uuid7_timestamp_ms('ffffffff-ffff-7000-0000-000000000000') = 281474976710655;
-- any form uuid_str accepts
select '04_07', uuid7_timestamp_ms('017F22E2-79B0-7CC3-98C4-DC0C0C07398F') = 1645557742000;
select '04_08', uuid7_timestamp_ms('{017f22e2-79b0-7cc3-98c4-dc0c0c07398f}') = 1645557742000;
select '04_09', uuid7_timestamp_ms('017f22e279b07cc398c4dc0c0c07398f') = 1645557742000;
select '04_10', uuid7_timestamp_ms(x'017f22e279b07cc398c4dc0c0c07398f') = 1645557742000;
select '04_11', uuid7_timestamp_ms(uuid_blob('018ff38a-a5c9-712d-bc80-0550b3ad41a2')) = 1717777901001;
-- other versions give NULL
select '04_12', uuid7_timestamp_ms('919108f7-52d1-4320-9bac-f847db4148a8') is null;
select '04_13', uuid7_timestamp_ms('d5a80b20-0d8f-11e5-b8cb-080027b6ec40') is null;
select '04_14', uuid7_timestamp_ms('017f22e2-79b0-8cc3-98c4-dc0c0c07398f') is null;
select '04_15', uuid7_timestamp_ms('00000000-0000-0000-0000-000000000000') is null;
select '04_16', uuid7_timestamp_ms(uuid4()) is null;
-- the variant is not checked
select '04_17', uuid7_timestamp_ms('017f22e2-79b0-7cc3-08c4-dc0c0c07398f') = 1645557742000;
select '04_18', uuid7_timestamp_ms('017f22e2-79b0-7cc3-f8c4-dc0c0c07398f') = 1645557742000;
-- invalid input gives NULL
select '04_19', uuid7_timestamp_ms(null) is null;
select '04_20', uuid7_timestamp_ms('') is null;
select '04_21', uuid7_timestamp_ms('hello') is null;
select '04_22', uuid7_timestamp_ms(123) is null;
select '04_23', uuid7_timestamp_ms(1.5) is null;
select '04_24', uuid7_timestamp_ms(x'017f22e279b07cc398c4dc0c0c0739') is null;

-- uuid_str
-- the accepted input forms, hex digits in any case
select '05_01', uuid_str('A0EEBC99-9C0B-4EF8-BB6D-6BB9BD380A11') = 'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11';
select '05_02', uuid_str('{a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11}') = 'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11';
select '05_03', uuid_str('a0eebc999c0b4ef8bb6d6bb9bd380a11') = 'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11';
select '05_04', uuid_str('urn:uuid:a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') = 'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11';
select '05_05', uuid_str('URN:UUID:a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') = 'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11';
select '05_06', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') = 'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11';
select '05_07', uuid_str('A0eeBC99-9c0B-4Ef8-bB6d-6bB9Bd380A11') = 'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11';
select '05_08', typeof(uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11')) = 'text';
-- a 16-byte blob is taken as the raw bytes
select '05_09', uuid_str(x'a0eebc999c0b4ef8bb6d6bb9bd380a11') = 'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11';
select '05_10', uuid_str(x'000102030405060708090a0b0c0d0e0f') = '00010203-0405-0607-0809-0a0b0c0d0e0f';
select '05_11', uuid_str(cast('0123456789abcdef' as blob)) = '30313233-3435-3637-3839-616263646566';
-- the version and variant are not checked
select '05_12', uuid_str(x'00000000000000000000000000000000') = '00000000-0000-0000-0000-000000000000';
select '05_13', uuid_str(x'ffffffffffffffffffffffffffffffff') = 'ffffffff-ffff-ffff-ffff-ffffffffffff';
select '05_14', uuid_str('d5a80b20-0d8f-11e5-b8cb-080027b6ec40') = 'd5a80b20-0d8f-11e5-b8cb-080027b6ec40';
select '05_15', uuid_str(randomblob(16)) like '________-____-____-____-____________';
-- hyphens only in the canonical places, braces balanced
select '05_16', uuid_str('a0-ee-bc-99-9c-0b-4e-f8-bb-6d-6b-b9-bd-38-0a-11') is null;
select '05_17', uuid_str('-a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') is null;
select '05_18', uuid_str('{-a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11}') is null;
select '05_19', uuid_str('{a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') is null;
select '05_20', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11}') is null;
select '05_21', uuid_str('x' || 'a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11' || 'y') is null;
select '05_22', uuid_str('a0ee-bc99-9c0b-4ef8-bb6d-6bb9-bd38-0a11') is null;
select '05_23', uuid_str('{a0eebc99-9c0b4ef8-bb6d6bb9-bd380a11}') is null;
select '05_24', uuid_str('{a0eebc999c0b4ef8bb6d6bb9bd380a11}') is null;
select '05_25', uuid_str('urn:uuid:a0eebc999c0b4ef8bb6d6bb9bd380a11') is null;
-- an embedded NUL makes the string invalid
select '05_26', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11' || char(0) || 'x') is null;
select '05_27', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a' || char(0) || '1') is null;
-- malformed text gives NULL
select '05_28', uuid_str('a0eebc99--9c0b-4ef8-bb6d-6bb9bd380a11') is null;
select '05_29', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11-') is null;
select '05_30', uuid_str('a0eebc9-9c0b-4ef8-bb6d-6bb9bd380a11') is null;
select '05_31', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a1') is null;
select '05_32', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a111') is null;
select '05_33', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a1100') is null;
select '05_34', uuid_str('g0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') is null;
select '05_35', uuid_str(' a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') is null;
select '05_36', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11 ') is null;
select '05_37', uuid_str('{{a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11}}') is null;
select '05_38', uuid_str('urn:uuix:a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') is null;
select '05_39', uuid_str('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a' || char(233)) is null;
select '05_40', uuid_str(char(0)) is null;
select '05_41', uuid_str('hello') is null;
select '05_42', uuid_str('') is null;
-- other types give NULL
select '05_43', uuid_str(null) is null;
select '05_44', uuid_str(123) is null;
select '05_45', uuid_str(1.5) is null;
select '05_46', uuid_str(x'') is null;
select '05_47', uuid_str(x'a0eebc999c0b4ef8bb6d6bb9bd380a') is null;
select '05_48', uuid_str(x'a0eebc999c0b4ef8bb6d6bb9bd380a1100') is null;
-- a blob is never parsed as text
select '05_49', uuid_str(cast('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11' as blob)) is null;
select '05_50', uuid_str(cast('a0eebc999c0b4ef8bb6d6bb9bd380a11' as blob)) is null;

-- uuid_blob
select '06_01', uuid_blob('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') = x'a0eebc999c0b4ef8bb6d6bb9bd380a11';
select '06_02', uuid_blob('A0EEBC99-9C0B-4EF8-BB6D-6BB9BD380A11') = x'a0eebc999c0b4ef8bb6d6bb9bd380a11';
select '06_03', uuid_blob('{a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11}') = x'a0eebc999c0b4ef8bb6d6bb9bd380a11';
select '06_04', uuid_blob('a0eebc999c0b4ef8bb6d6bb9bd380a11') = x'a0eebc999c0b4ef8bb6d6bb9bd380a11';
select '06_05', uuid_blob('urn:uuid:a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') = x'a0eebc999c0b4ef8bb6d6bb9bd380a11';
select '06_06', typeof(uuid_blob('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11')) = 'blob';
select '06_07', length(uuid_blob(uuid4())) = 16;
select '06_08', length(uuid_blob(uuid7())) = 16;
-- a 16-byte blob is returned as is
select '06_09', uuid_blob(x'a0eebc999c0b4ef8bb6d6bb9bd380a11') = x'a0eebc999c0b4ef8bb6d6bb9bd380a11';
select '06_10', uuid_blob(x'00000000000000000000000000000000') = x'00000000000000000000000000000000';
select '06_11', uuid_blob(cast('0123456789abcdef' as blob)) = cast('0123456789abcdef' as blob);
-- invalid input gives NULL
select '06_12', uuid_blob('{a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11') is null;
select '06_13', uuid_blob('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11' || char(0) || 'x') is null;
select '06_14', uuid_blob('a0eebc99--9c0b-4ef8-bb6d-6bb9bd380a11') is null;
select '06_15', uuid_blob('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a1') is null;
select '06_16', uuid_blob('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a111') is null;
select '06_17', uuid_blob('a0ee-bc99-9c0b-4ef8-bb6d-6bb9-bd38-0a11') is null;
select '06_18', uuid_blob('hello') is null;
select '06_19', uuid_blob('') is null;
select '06_20', uuid_blob(null) is null;
select '06_21', uuid_blob(123) is null;
select '06_22', uuid_blob(1.5) is null;
select '06_23', uuid_blob(x'') is null;
select '06_24', uuid_blob(x'a0eebc999c0b4ef8bb6d6bb9bd380a') is null;
select '06_25', uuid_blob(x'a0eebc999c0b4ef8bb6d6bb9bd380a1100') is null;
select '06_26', uuid_blob(cast('a0eebc99-9c0b-4ef8-bb6d-6bb9bd380a11' as blob)) is null;

-- roundtrip through a table
create table uuids(id integer primary key, s text, b blob);
insert into uuids(s) select uuid4() from generate_series(1, 100);
insert into uuids(s) select uuid7() from generate_series(1, 100);
insert into uuids(s) select uuid7(value) from generate_series(1, 100);
update uuids set b = uuid_blob(s);
select '07_01', (select count(*) from uuids where length(b) = 16) = 300;
select '07_02', (select count(*) from uuids where uuid_str(b) = s) = 300;
select '07_03', (select count(*) from uuids where uuid_blob(uuid_str(b)) = b) = 300;
select '07_04', (select count(*) from uuids where uuid_str(upper(s)) = s) = 300;
select '07_05', (select count(distinct b) from uuids) = 300;
-- text and blob forms sort the same way
select '07_06', (select group_concat(id) from (select id from uuids order by s)) = (select group_concat(id) from (select id from uuids order by b));
-- uuid7 with increasing timestamps sorts by timestamp
select '07_07', (select group_concat(id) from (select id from uuids where id > 200 order by b)) = (select group_concat(id) from (select id from uuids where id > 200 order by id));
select '07_08', (select count(*) from uuids where id > 200 and uuid7_timestamp_ms(b) = (id - 200) * 1000) = 100;
drop table uuids;
