-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/sqlar

-- does not compress the value unless it yields a smaller blob
select '01', length(sqlar_compress(cast(printf('%.3c', 'o') as blob))) = 3;
select '02', length(sqlar_compress(cast(printf('%.10c', 'o') as blob))) = 10;
select '03', length(sqlar_compress(cast(printf('%.20c', 'o') as blob))) = 11;
select '04', length(sqlar_compress(cast(printf('%.30c', 'o') as blob))) = 11;
select '05', length(sqlar_compress(zeroblob(1024))) = 17;

-- uncompresses the value if needed given the orizinal value size
select '11', sqlar_uncompress(sqlar_compress(cast(printf('%.3c', 'o') as blob)), 3)
    = cast('ooo' as blob);
select '12', sqlar_uncompress(sqlar_compress(cast(printf('%.10c', 'o') as blob)), 10)
    = cast('oooooooooo' as blob);
select '13', sqlar_uncompress(sqlar_compress(cast(printf('%.20c', 'o') as blob)), 20)
    = cast('oooooooooooooooooooo' as blob);
select '14', sqlar_uncompress(sqlar_compress(cast(printf('%.30c', 'o') as blob)), 30)
    = cast('oooooooooooooooooooooooooooooo' as blob);
select '15', sqlar_uncompress(sqlar_compress(zeroblob(1024)), 1024)
    = zeroblob(1024);
