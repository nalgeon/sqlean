-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/math

-- rounding (5)
select '1_01', math_ceil(3.3) = 4;
select '1_02', math_ceil(-3.9) = -3;
select '1_03', ceiling(3.3) = 4;
select '1_04', ceiling(-3.9) = -3;
select '1_05', math_floor(3.9) = 3;
select '1_06', math_floor(-3.9) = -4;
select '1_07', math_trunc(3.3) = 3;
select '1_08', math_trunc(3.9) = 3;
select '1_09', math_trunc(-3.3) = -3;
select '1_10', math_trunc(-3.9) = -3;
select '1_11', math_round(42.123456789, 9) = 42.123456789;
select '1_12', math_round(42.123456789, 8) = 42.12345679;
select '1_13', math_round(42.123456789, 7) = 42.1234568;
select '1_14', math_round(42.123456789, 6) = 42.123457;
select '1_15', math_round(42.123456789, 5) = 42.12346;
select '1_16', math_round(42.123456789, 4) = 42.1235;
select '1_17', math_round(42.123456789, 3) = 42.123;
select '1_18', math_round(42.123456789, 2) = 42.12;
select '1_19', math_round(42.123456789, 1) = 42.1;
select '1_20', math_round(42.123456789, 0) = 42.0;

-- log (5)
select '2_01', math_round(math_ln(2.71828*2.71828)) = 2;
select '2_02', math_round(math_log(100)) = 2;
select '2_03', math_round(math_log10(100)) = 2;
select '2_04', math_round(math_log2(4)) = 2;
select '2_05', math_round(math_log(3,9)) = 2;

-- power (4)
select '2_06', math_round(math_exp(2), 3) = math_round(2.71828*2.71828, 3);
select '2_07', math_pow(2, 10) = 1024;
select '2_08', power(2, 10) = 1024;
select '2_09', math_sqrt(100) = 10;

-- trigonometric (3)
select '3_01', math_cos(0) = 1;
select '3_02', math_round(math_cos(math_pi()/2)) = 0;
select '3_03', math_sin(0) = 0;
select '3_04', math_round(math_sin(math_pi()/2)) = 1;
select '3_05', math_tan(0) = 0;
select '3_06', math_round(math_tan(0.8)) = 1;

-- hyperbolic (3)
select '4_01', math_cosh(0) = 1;
select '4_02', math_round(math_cosh(2.07)) = 4;
select '4_03', math_sinh(0) = 0;
select '4_04', math_round(math_sinh(2.1)) = 4;
select '4_05', math_tanh(0) = 0;
select '4_06', math_round(math_tanh(3)) = 1;

-- inverse trigonometric (4)
select '5_01', math_acos(1) = 0;
select '5_02', math_round(math_acos(0), 2) = math_round(math_pi()/2, 2);
select '5_03', math_asin(0) = 0;
select '5_04', math_round(math_asin(1), 2) = math_round(0.5*math_pi(), 2);
select '5_05', math_atan(0) = 0;
select '5_06', math_round(math_atan(math_pi()/2)) = 1;
select '5_07', math_round(math_atan2(1, 2), 2) = 0.46;
select '5_08', math_round(math_atan2(math_pi(), 2)) = 1;

-- inverse hyperbolic (3)
select '6_01', math_acosh(1) = 0;
select '6_02', math_round(math_acosh(4)) = 2;
select '6_03', math_asinh(0) = 0;
select '6_04', math_round(math_asinh(4)) = 2;
select '6_05', math_atanh(0) = 0;
select '6_06', math_round(math_atanh(0.8)) = 1;

-- angular measures (2)
select '7_01', math_radians(0) = 0;
select '7_02', math_round(math_radians(180), 2) = math_round(math_pi(), 2);
select '7_03', math_degrees(0) = 0;
select '7_04', math_round(math_degrees(math_pi())) = 180;

-- other (2)
select '8_01', math_mod(10,3) = 1;
select '8_02', math_round(math_pi(), 5) = 3.14159;
