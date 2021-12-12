-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/spellfix

create virtual table dictionary using spellfix1;

insert into dictionary(word)
values ('similarity'), ('search'), ('is'), ('awesome');

select '00', word = 'awesome'
from dictionary
where word match 'awesoem';