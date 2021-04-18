.load dist/text
select '01', reverse(null) is NULL;
select '02', reverse('hello') = 'olleh';
select '03', reverse('привет') = 'тевирп';
select '04', reverse("𐌀𐌁𐌂") = '𐌂𐌁𐌀';
select '05', reverse('hello 42@ world') = 'dlrow @24 olleh';
