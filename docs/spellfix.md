# spellfix: String similarity search in SQLite

Provides a mechanism to search a large vocabulary for close matches. This is the 'native' SQLite [Spellfix extension](https://www.sqlite.org/spellfix1.html). See the link for detailed documentation.

## Basic usage

Use `spellfix1` virtual table for fuzzy matching:

1. Create a virtual table:

```sql
create virtual table dictionary using spellfix1;
```

2. Fill it with data:

```sql
insert into dictionary(word)
values ('similarity'), ('search'), ('is'), ('awesome');
```

3. Query to find the nearest match:

```sql
select word
from dictionary
where word match 'awesoem';
```

## Functions

The extension provides several auxiliary string functions, like calculating the phonetic hash of the word or measuring the distance between words. If you are interested, see the [fuzzy](fuzzy.md) extension - it contains them all and some more.

[Download](https://github.com/nalgeon/sqlean/releases/latest)
