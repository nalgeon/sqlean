# math: Mathematics in SQLite

The `sqlean-math` extension provides SQLite's [built-in math functions](https://sqlite.org/lang_mathfunc.html) for SQLite builds compiled without the `SQLITE_ENABLE_MATH_FUNCTIONS` flag.

## Names

Each function is available under two names: the standard SQLite name (`sqrt`) and the same name with the `math_` prefix (`math_sqrt`).

When the extension is loaded into an SQLite build that already has the math functions, the standard names replace the built-in ones for that connection. The two are the same code (taken from SQLite 3.54.1), so the results don't change.

| Group                 | Functions                                                    |
| --------------------- | ------------------------------------------------------------ |
| rounding              | `ceil` (`ceiling`), `floor`, `trunc`, `math_round`           |
| logarithmic           | `ln`, `log` (base 10), `log10`, `log2`, `log(B, X)` (base B) |
| arithmetic            | `exp`, `pow` (`power`), `sqrt`, `mod`                        |
| trigonometric         | `cos`, `sin`, `tan`                                          |
| hyperbolic            | `cosh`, `sinh`, `tanh`                                       |
| inverse trigonometric | `acos`, `asin`, `atan`, `atan2`                              |
| inverse hyperbolic    | `acosh`, `asinh`, `atanh`                                    |
| angular measures      | `radians`, `degrees`                                         |
| constants             | `pi`                                                         |

The aliases `ceiling` and `power` have no prefixed version: use `math_ceil` and `math_pow`.

See the [SQLite documentation](https://sqlite.org/lang_mathfunc.html) for a description of each function.

## Behavior

Like the built-in functions:

- `NULL`, non-numeric text, or a BLOB in any argument gives `NULL`. Numeric text is converted: `sqrt('4')` is `2.0`.
- An undefined result gives `NULL`: `sqrt(-1)`, `ln(0)`, `acos(2)`, `mod(1, 0)`. So does `log(B, X)` with a base `B` of 1 or less.
- An overflow gives infinity: `exp(1000)` and `pow(0, -1)` are `Inf`.
- `ceil`, `floor` and `trunc` return an integer argument as is, and a real for a real argument. The other functions always return a real.

`math_round(X, N)` follows the core `round` function: halves round away from zero, `N` is clamped to 0–30, and the arguments are converted with the usual casting rules, so `math_round('abc')` is `0.0`.

## Acknowledgements

Adapted from SQLite 3.54.1 source code ([func.c](https://sqlite.org/src/file/src/func.c)).

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
