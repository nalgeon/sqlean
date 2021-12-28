-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/json2

select '01', json_pretty('{"answer":42}') = '{
  "answer": 42
}';

select '11', json_length('[1,2,3,4]') = 4;
select '12', json_length('{"a":1, "b":2, "c":3}') = 3;
select '13', json_length('{"a":1, "b":[2,3], "c":4}', '$.b') = 2;

select '21', json_equal('{"a":1, "b":2}', '{"b":2, "a":1}') = 1;
select '22', json_equal('{"a":10}', '{"a":11}') = 0;

select '31', json_keys('{"a":1, "b":2, "c":3}') = '["a","b","c"]';
select '32', json_keys('{"a":1, "b": {"c":3, "d":4} }', '$.b') = '["c","d"]';

select '41', json_contains_path('{"a":1, "b":[2,3], "c":4}', '$.b[1]') = 1;
select '42', json_contains_path('{"a":1, "b":[2,3], "c":4}', '$.b[2]') = 0;
select '43', json_contains_path('{"a":1, "b":[2,3], "c":4}', '$.d') = 0;
