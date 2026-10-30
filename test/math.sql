-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/math

-- Golden values for transcendental functions come from Python's math module.
-- They are compared with a 1e-12 tolerance, since libm results may differ in the last bit.
-- A NaN result is NULL in SQLite, an infinite result is +-Inf (9e999).

-- ceil, ceiling
select '01_01', math_ceil(3.3) = 4.0;
select '01_02', math_ceil(-3.9) = -3.0;
select '01_03', ceil(3.3) = 4.0;
select '01_04', ceiling(3.3) = 4.0;
select '01_05', ceiling(-3.9) = -3.0;
select '01_06', math_ceil(-0.5) = 0.0;
-- an integer is returned as is, a real gives a real
select '01_07', typeof(math_ceil(3)) = 'integer';
select '01_08', typeof(math_ceil(3.0)) = 'real';
select '01_09', math_ceil(9223372036854775807) = 9223372036854775807;
-- numeric text is converted
select '01_10', math_ceil('3.3') = 4.0;
select '01_11', typeof(math_ceil('3')) = 'integer';

-- floor
select '02_01', math_floor(3.9) = 3.0;
select '02_02', math_floor(-3.9) = -4.0;
select '02_03', floor(3.9) = 3.0;
select '02_04', math_floor(-0.5) = -1.0;
select '02_05', typeof(math_floor(3)) = 'integer';
select '02_06', typeof(math_floor(3.0)) = 'real';
select '02_07', math_floor(-9223372036854775808) = -9223372036854775808;
select '02_08', math_floor('3.9') = 3.0;

-- trunc
select '03_01', math_trunc(3.3) = 3.0;
select '03_02', math_trunc(3.9) = 3.0;
select '03_03', math_trunc(-3.3) = -3.0;
select '03_04', math_trunc(-3.9) = -3.0;
select '03_05', trunc(3.9) = 3.0;
select '03_06', typeof(math_trunc(3)) = 'integer';
select '03_07', typeof(math_trunc(3.0)) = 'real';
select '03_08', math_trunc('-3.9') = -3.0;

-- math_round
select '04_01', math_round(42.123456789, 9) = 42.123456789;
select '04_02', math_round(42.123456789, 8) = 42.12345679;
select '04_03', math_round(42.123456789, 7) = 42.1234568;
select '04_04', math_round(42.123456789, 6) = 42.123457;
select '04_05', math_round(42.123456789, 5) = 42.12346;
select '04_06', math_round(42.123456789, 4) = 42.1235;
select '04_07', math_round(42.123456789, 3) = 42.123;
select '04_08', math_round(42.123456789, 2) = 42.12;
select '04_09', math_round(42.123456789, 1) = 42.1;
select '04_10', math_round(42.123456789, 0) = 42.0;
select '04_11', math_round(42.123456789) = 42.0;
-- halves round away from zero
select '04_12', math_round(2.5) = 3.0;
select '04_13', math_round(-2.5) = -3.0;
select '04_14', math_round(0.125, 2) = 0.13;
-- the result is always real
select '04_15', typeof(math_round(42)) = 'real';
select '04_16', math_round(42) = 42.0;
-- a negative number of digits is 0, more than 30 is 30
select '04_17', math_round(1.25, -1) = 1.0;
select '04_18', math_round(0.1234567890123456789, 40) = math_round(0.1234567890123456789, 30);
-- a value beyond 2^52 has no fractional part and is returned as is
select '04_19', math_round(1e20) = 1e20;
select '04_20', math_round(-1e20, 2) = -1e20;
-- like the built-in round(), the value and the digits are converted with
-- the usual casting rules, so non-numeric text is 0
select '04_21', math_round('2.5') = 3.0;
select '04_22', math_round('abc') = 0.0;
select '04_23', math_round(1.5, 'abc') = 2.0;
select '04_24', math_round(1.25, '1') = 1.3;

-- ln
select '05_01', math_ln(1) = 0.0;
select '05_02', abs(math_ln(10) - 2.302585092994046) < 1e-12;
select '05_03', abs(ln(2.718281828459045) - 1.0) < 1e-12;
select '05_04', typeof(math_ln(1)) = 'real';
-- the logarithm of zero or a negative number is NULL
select '05_05', math_ln(0) is null;
select '05_06', math_ln(-1) is null;

-- log, log10: base 10
select '06_01', math_log(100) = 2.0;
select '06_02', math_log10(1000) = 3.0;
select '06_03', math_log10(0.001) = -3.0;
select '06_04', log(100) = 2.0;
select '06_05', log10(1000) = 3.0;
select '06_06', math_log(1) = 0.0;
select '06_07', math_log(0) is null;
select '06_08', math_log10(-1) is null;

-- log2
select '07_01', math_log2(1024) = 10.0;
select '07_02', math_log2(0.5) = -1.0;
select '07_03', log2(4) = 2.0;
select '07_04', math_log2(0) is null;
select '07_05', math_log2(-1) is null;

-- log(B, X): base B
select '08_01', math_log(2, 8) = 3.0;
select '08_02', abs(math_log(3, 9) - 2.0) < 1e-12;
select '08_03', abs(log(2, 1024) - 10.0) < 1e-12;
select '08_04', math_log(10, 1) = 0.0;
select '08_05', math_log(2, 0) is null;
select '08_06', math_log(2, -8) is null;
select '08_07', math_log(0, 8) is null;
select '08_08', math_log(-2, 8) is null;
-- a base of 1 or less is NULL, though log(0.5, 8) = -3 mathematically
select '08_09', math_log(1, 8) is null;
select '08_10', math_log(0.5, 8) is null;
-- the value is converted with the usual casting rules, so non-numeric text is 0, giving NULL
select '08_11', math_log(2, '8') = 3.0;
select '08_12', math_log(2, 'abc') is null;

-- exp
select '09_01', math_exp(0) = 1.0;
select '09_02', abs(math_exp(1) - 2.718281828459045) < 1e-12;
select '09_03', abs(exp(-1) - 0.36787944117144233) < 1e-12;
select '09_04', math_exp(1000) = 9e999;
select '09_05', math_exp(-1000) = 0.0;

-- pow, power
select '10_01', math_pow(2, 10) = 1024.0;
select '10_02', pow(2, 10) = 1024.0;
select '10_03', power(2, 10) = 1024.0;
select '10_04', math_pow(2, -1) = 0.5;
select '10_05', math_pow(4, 0.5) = 2.0;
select '10_06', math_pow(0, 0) = 1.0;
select '10_07', typeof(math_pow(2, 2)) = 'real';
select '10_08', math_pow(0, -1) = 9e999;
select '10_09', math_pow(10, 400) = 9e999;
-- a fractional power of a negative number is NaN
select '10_10', math_pow(-8, 1.0/3) is null;

-- sqrt
select '11_01', math_sqrt(100) = 10.0;
select '11_02', sqrt(2.25) = 1.5;
select '11_03', abs(math_sqrt(2) - 1.4142135623730951) < 1e-12;
select '11_04', math_sqrt(0) = 0.0;
select '11_05', typeof(math_sqrt(4)) = 'real';
select '11_06', math_sqrt(-1) is null;
select '11_07', math_sqrt('4') = 2.0;

-- mod: the result has the sign of the dividend
select '12_01', math_mod(10, 3) = 1.0;
select '12_02', mod(10, 3) = 1.0;
select '12_03', math_mod(7.5, 2) = 1.5;
select '12_04', math_mod(-7, 3) = -1.0;
select '12_05', math_mod(7, -3) = 1.0;
select '12_06', typeof(math_mod(10, 3)) = 'real';
select '12_07', math_mod(1, 0) is null;

-- cos, sin, tan
select '13_01', math_cos(0) = 1.0;
select '13_02', abs(math_cos(1) - 0.5403023058681398) < 1e-12;
select '13_03', abs(cos(1) - 0.5403023058681398) < 1e-12;
select '13_04', abs(math_cos(math_pi()/2)) < 1e-12;
select '13_05', math_sin(0) = 0.0;
select '13_06', abs(math_sin(1) - 0.8414709848078965) < 1e-12;
select '13_07', abs(sin(1) - 0.8414709848078965) < 1e-12;
select '13_08', abs(math_sin(math_pi()/2) - 1.0) < 1e-12;
select '13_09', math_tan(0) = 0.0;
select '13_10', abs(math_tan(1) - 1.557407724654902) < 1e-12;
select '13_11', abs(tan(1) - 1.557407724654902) < 1e-12;

-- cosh, sinh, tanh
select '14_01', math_cosh(0) = 1.0;
select '14_02', abs(math_cosh(1) - 1.5430806348152437) < 1e-12;
select '14_03', abs(cosh(1) - 1.5430806348152437) < 1e-12;
select '14_04', math_sinh(0) = 0.0;
select '14_05', abs(math_sinh(1) - 1.1752011936438014) < 1e-12;
select '14_06', abs(sinh(1) - 1.1752011936438014) < 1e-12;
select '14_07', math_tanh(0) = 0.0;
select '14_08', abs(math_tanh(1) - 0.7615941559557649) < 1e-12;
select '14_09', abs(tanh(1) - 0.7615941559557649) < 1e-12;
select '14_10', math_cosh(1000) = 9e999;

-- acos, asin, atan
select '15_01', math_acos(1) = 0.0;
select '15_02', abs(math_acos(0.5) - 1.0471975511965976) < 1e-12;
select '15_03', abs(acos(0.5) - 1.0471975511965976) < 1e-12;
select '15_04', math_asin(0) = 0.0;
select '15_05', abs(math_asin(0.5) - 0.5235987755982988) < 1e-12;
select '15_06', abs(asin(0.5) - 0.5235987755982988) < 1e-12;
select '15_07', math_atan(0) = 0.0;
select '15_08', abs(math_atan(1) - 0.7853981633974483) < 1e-12;
select '15_09', abs(atan(1) - 0.7853981633974483) < 1e-12;
-- outside [-1, 1] is NaN
select '15_10', math_acos(2) is null;
select '15_11', math_asin(-2) is null;

-- atan2(Y, X)
select '16_01', math_atan2(0, 1) = 0.0;
select '16_02', abs(math_atan2(1, 2) - 0.4636476090008061) < 1e-12;
select '16_03', abs(atan2(1, 2) - 0.4636476090008061) < 1e-12;
-- the quadrant is determined by the signs of both arguments
select '16_04', abs(math_atan2(0, -1) - math_pi()) < 1e-12;
select '16_05', abs(math_atan2(-1, 0) + math_pi()/2) < 1e-12;

-- acosh, asinh, atanh
select '17_01', math_acosh(1) = 0.0;
select '17_02', abs(math_acosh(2) - 1.3169578969248166) < 1e-12;
select '17_03', abs(acosh(2) - 1.3169578969248166) < 1e-12;
select '17_04', math_acosh(0.5) is null;
select '17_05', math_asinh(0) = 0.0;
select '17_06', abs(math_asinh(1) - 0.881373587019543) < 1e-12;
select '17_07', abs(asinh(1) - 0.881373587019543) < 1e-12;
select '17_08', math_atanh(0) = 0.0;
select '17_09', abs(math_atanh(0.5) - 0.5493061443340549) < 1e-12;
select '17_10', abs(atanh(0.5) - 0.5493061443340549) < 1e-12;
select '17_11', math_atanh(1) = 9e999;
select '17_12', math_atanh(2) is null;

-- radians, degrees
select '18_01', math_radians(0) = 0.0;
select '18_02', abs(math_radians(1) - 0.017453292519943295) < 1e-12;
select '18_03', abs(radians(180) - math_pi()) < 1e-12;
select '18_04', math_degrees(0) = 0.0;
select '18_05', abs(math_degrees(1) - 57.29577951308232) < 1e-12;
select '18_06', abs(degrees(math_pi()) - 180.0) < 1e-12;

-- pi
select '19_01', math_pi() = 3.141592653589793;
select '19_02', pi() = 3.141592653589793;
select '19_03', typeof(math_pi()) = 'real';

-- NULL in any argument gives NULL
select '20_01', math_ceil(null) is null;
select '20_02', math_floor(null) is null;
select '20_03', math_trunc(null) is null;
select '20_04', math_round(null) is null;
select '20_05', math_round(null, 2) is null;
select '20_06', math_round(1.5, null) is null;
select '20_07', math_ln(null) is null;
select '20_08', math_log(null) is null;
select '20_09', math_log10(null) is null;
select '20_10', math_log2(null) is null;
select '20_11', math_log(null, 8) is null;
select '20_12', math_log(2, null) is null;
select '20_13', math_exp(null) is null;
select '20_14', math_pow(null, 2) is null;
select '20_15', math_pow(2, null) is null;
select '20_16', math_sqrt(null) is null;
select '20_17', math_mod(null, 3) is null;
select '20_18', math_mod(10, null) is null;
select '20_19', math_cos(null) is null;
select '20_20', math_sin(null) is null;
select '20_21', math_tan(null) is null;
select '20_22', math_cosh(null) is null;
select '20_23', math_sinh(null) is null;
select '20_24', math_tanh(null) is null;
select '20_25', math_acos(null) is null;
select '20_26', math_asin(null) is null;
select '20_27', math_atan(null) is null;
select '20_28', math_atan2(null, 1) is null;
select '20_29', math_atan2(1, null) is null;
select '20_30', math_acosh(null) is null;
select '20_31', math_asinh(null) is null;
select '20_32', math_atanh(null) is null;
select '20_33', math_radians(null) is null;
select '20_34', math_degrees(null) is null;

-- a non-numeric text or a blob gives NULL
select '21_01', math_ceil('abc') is null;
select '21_02', math_floor(x'33') is null;
select '21_03', math_trunc('3abc') is null;
select '21_04', math_ln('abc') is null;
select '21_05', math_log('abc', 8) is null;
select '21_06', math_exp('abc') is null;
select '21_07', math_pow('abc', 2) is null;
select '21_08', math_pow(2, 'abc') is null;
select '21_09', math_sqrt(x'34') is null;
select '21_10', math_mod(10, 'abc') is null;
select '21_11', math_sin('abc') is null;
select '21_12', math_atan2('abc', 1) is null;
select '21_13', math_atan2(1, x'01') is null;
select '21_14', math_radians('abc') is null;
