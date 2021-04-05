# sqlite3-math: Math Functions

Common math functions for SQLite versions before 3.35.
Extracted from SQLite 3.35.4 source code (`func.c`).

Provides following functions:

-   trigonometric: `sin`, `cos`, `tan` etc,
-   logarithmic: `ln`, `log10`, `log2`, `log`,
-   rounding: `ceil`, `floor`, `trunk`,
-   arithmetic: `pow`, `sqrt` and `mod`,
-   `pi`.

[Full functions list](https://sqlite.org/lang_mathfunc.html)

[Download](https://github.com/nalgeon/sqlean/releases/latest)

## Usage

```
sqlite> .load ./sqlite3-math
sqlite> select sqrt(9);
```
