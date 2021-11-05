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