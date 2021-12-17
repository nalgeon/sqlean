-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/stats
.load dist/json1

-- total of 17 functions

select '01', json('{"answer" : 42}') = '{"answer":42}';
select '02', json_array(1, 2, 3, 4) = '[1,2,3,4]';
select '03', json_array_length('[1,2,3,4]') = 4;
select '04', json_object('answer', 42) = '{"answer":42}';

select '11', json_extract('{"answer":42}', '$.answer') = 42;
select '12', json_insert('[1,2,3]', '$[#]', 42) = '[1,2,3,42]';
select '13', json_replace('{"answer":42}', '$.answer', 'no') = '{"answer":"no"}';
select '14', json_set('{"answer":42}', '$.useful', false) = '{"answer":42,"useful":0}';
select '15', json_patch('{"a":1,"b":2,"c":3}', '{"b":10,"d":11}') = '{"a":1,"b":10,"c":3,"d":11}';
select '16', json_remove('{"answer":42,"useful":0}', '$.useful') = '{"answer":42}';

select '21', json_type('{"answer":42}') = 'object';
select '22', json_valid('{"answer":42}') = 1;
select '23', json_quote('answer') = '"answer"';

select '31', json_group_array(value) = '[1,2,3,4]' from generate_series(1,4);
select '32', json_group_object('v', value) = '{"v":1,"v":2,"v":3,"v":4}' from generate_series(1,4);

select '41', sum(value) = 10 from json_each('[1,2,3,4]');
select '42', sum(value) = 10 from json_each('{"a":[1,2,3,4]}', '$.a');
select '43', count(*) = 6 from json_tree('{"a":[1,2,3,4]}');
select '44', count(*) = 5 from json_tree('{"a":[1,2,3,4]}', '$.a');
