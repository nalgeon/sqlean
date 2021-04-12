.load dist/text
select "reverse(null) is NULL",
    reverse(null) is NULL
;
select "reverse('hello') = 'olleh'",
    reverse('hello') = 'olleh'
;
select "reverse('привет') = 'тевирп'",
    reverse('привет') = 'тевирп'
;
select "reverse('𐌀𐌁𐌂') = '𐌂𐌁𐌀'",
    reverse("𐌀𐌁𐌂") = '𐌂𐌁𐌀'
;
select "reverse('hello 42@ world') = 'dlrow @24 olleh'",
    reverse('hello 42@ world') = 'dlrow @24 olleh'
;
select reverse();
select reverse('hello', 'world');
