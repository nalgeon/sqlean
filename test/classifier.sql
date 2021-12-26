-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/classifier

create table emails(bad_sender int, bad_words int, links int, spam int);
insert into emails(bad_sender, bad_words, links, spam) values
(1, 1, 1, 1), (1, 0, 0, 1), (1, 0, 1, 1), (1, 0, 0, 0), (0, 0, 0, 0),
(1, 0, 1, 1), (1, 1, 0, 1), (0, 1, 1, 1), (0, 0, 1, 0), (0, 1, 0, 0),
(1, 1, 1, 1), (1, 0, 0, 1), (1, 0, 1, 1), (1, 0, 0, 0), (0, 0, 0, 0),
(1, 0, 1, 1), (1, 1, 0, 1), (0, 1, 1, 1), (0, 0, 1, 0), (0, 1, 0, 0),
(1, 1, 1, 1), (1, 0, 0, 1), (1, 0, 1, 1), (1, 0, 0, 0), (0, 0, 0, 0),
(1, 0, 1, 1), (1, 1, 0, 1), (0, 1, 1, 1), (0, 0, 1, 0), (0, 1, 0, 0),
(1, 1, 1, 1), (1, 0, 0, 1), (1, 0, 1, 1), (1, 0, 0, 0), (0, 0, 0, 0),
(1, 0, 1, 1), (1, 1, 0, 1), (0, 1, 1, 1), (0, 0, 1, 0), (0, 1, 0, 0);

select train(bad_sender, bad_words, links, spam) from emails;
select '01', round(classify(1, 1, 1)) = 1;
select '02', round(classify(0, 0, 0)) = 0;
