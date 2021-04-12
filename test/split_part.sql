.load dist/text
select
    "split_part(NULL, ',', 2) is NULL",
    split_part(NULL, ',', 2) is NULL
;
select
    "split_part('', ',', 2) = ''",
    split_part('', ',', 2) = ''
;
select
    "split_part('one,two,three', ',', 2) = 'two'",
    split_part('one,two,three', ',', 2) = 'two'
;
select
    "split_part('one∞two∞three', '∞', 2) = 'two'",
    split_part('one∞two∞three', '∞', 2) = 'two'
;
select
    "split_part('один,два,три', ',', 2) = 'два'",
    split_part('один,два,три', ',', 2) = 'два'
;
select
    "split_part('one,two,three', ',', 10) = ''",
    split_part('one,two,three', ',', 10) = ''
;
select
    "split_part('one,two,three', ';', 2) = ''",
    split_part('one,two,three', ';', 2) = ''
;
select
    "split_part('one,two,three', '', 1) = 'one,two,three'",
    split_part('one,two,three', '', 1) = 'one,two,three'
;
select
    "split_part('one, two, three', ', ', 2) = 'two'",
    split_part('one, two, three', ', ', 2) = 'two'
;
select
    "split_part('one,two,three', NULL, 2) is NULL",
    split_part('one,two,three', NULL, 2) is NULL
;
select split_part('one,two,three', ',');
select split_part('one,two,three', ',', 0);
select split_part('one,two,three', ',', 'a');