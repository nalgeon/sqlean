# math: Mathematics in SQLite

The `sqlean-math` extension provides common math functions for SQLite versions compiled without the `SQLITE_ENABLE_MATH_FUNCTIONS` flag.

Provides following functions:

-   rounding: `math_round`, `math_ceil`, `math_floor`, `math_trunc`;
-   logarithmic: `math_ln`, `math_log10`, `math_log2`, `math_log`;
-   arithmetic: `math_pow`, `math_sqrt`, `math_mod`;
-   trigonometric: `math_cos`, `math_sin`, `math_tan`;
-   hyperbolic: `math_cosh`, `math_sinh`, `math_tanh`;
-   inverse trigonometric: `math_acos`, `math_asin`, `math_atan`, `math_atan2`;
-   inverse hyperbolic: `math_acosh`, `math_asinh`, `math_atanh`;
-   angular measures: `math_radians`, `math_degrees`;
-   constants: `math_pi`.

[Full description](https://sqlite.org/lang_mathfunc.html)

## Acknowledgements

Adapted from SQLite source code ([func.c](https://sqlite.org/src/file/src/func.c)).

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./math
sqlite> select math_sqrt(9);
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
