# math: Mathematics in SQLite

Common math functions for SQLite versions before 3.35.
Extracted from SQLite 3.35.4 source code ([func.c](https://sqlite.org/src/file/src/func.c)).

Provides following functions:

-   rounding: `ceil`, `floor`, `trunc`;
-   logarithmic: `ln`, `log10`, `log2`, `log`;
-   arithmetic: `pow`, `sqrt`, `mod`;
-   trigonometric: `cos`, `sin`, `tan`;
-   hyperbolic: `cosh`, `sinh`, `tanh`;
-   inverse trigonometric: `acos`, `asin`, `atan`, `atan2`;
-   inverse hyperbolic: `acosh`, `asinh`, `atanh`;
-   angular measures: `radians`, `degrees`;
-   `pi`.

[Full description](https://sqlite.org/lang_mathfunc.html)

## Installation and Usage

SQLite command-line interface:

```
sqlite> .load ./math
sqlite> select sqrt(9);
```

See [How to Install an Extension](install.md) for usage with IDE, Python, etc.

[⬇️ Download](https://github.com/nalgeon/sqlean/releases/latest) •
[✨ Explore](https://github.com/nalgeon/sqlean) •
[🚀 Follow](https://antonz.org/subscribe/)
