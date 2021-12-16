-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/math

-- total of 30 functions

-- rounding (4)
select '01', ceil(3.3) = 4;
select '02', ceil(-3.9) = -3;
select '03', ceiling(3.3) = 4;
select '04', ceiling(-3.9) = -3;
select '05', floor(3.9) = 3;
select '06', floor(-3.9) = -4;
select '07', trunc(3.3) = 3;
select '08', trunc(3.9) = 3;
select '09', trunc(-3.3) = -3;
select '10', trunc(-3.9) = -3;

-- log (5)
select '11', round(ln(2.71828*2.71828)) = 2;
select '12', round(log(100)) = 2;
select '13', round(log10(100)) = 2;
select '14', round(log2(4)) = 2;
select '15', round(log(3,9)) = 2;

-- power (4)
select '16', round(exp(2), 3) = round(2.71828*2.71828, 3);
select '17', pow(2, 10) = 1024;
select '18', power(2, 10) = 1024;
select '19', sqrt(100) = 10;

-- trigonometric (3)
select '21', cos(0) = 1;
select '22', round(cos(pi()/2)) = 0;
select '23', sin(0) = 0;
select '24', round(sin(pi()/2)) = 1;
select '25', tan(0) = 0;
select '26', round(tan(0.8)) = 1;

-- hyperbolic (3)
select '31', cosh(0) = 1;
select '32', round(cosh(2.07)) = 4;
select '33', sinh(0) = 0;
select '34', round(sinh(2.1)) = 4;
select '35', tanh(0) = 0;
select '36', round(tanh(3)) = 1;

-- inverse trigonometric (4)
select '41', acos(1) = 0;
select '42', round(acos(0), 2) = round(pi()/2, 2);
select '43', asin(0) = 0;
select '44', round(asin(1), 2) = round(0.5*pi(), 2);
select '45', atan(0) = 0;
select '46', round(atan(pi()/2)) = 1;
select '47', round(atan2(1, 2), 2) = 0.46;
select '48', round(atan2(pi(), 2)) = 1;

-- inverse hyperbolic (3)
select '51', acosh(1) = 0;
select '52', round(acosh(4)) = 2;
select '53', asinh(0) = 0;
select '54', round(asinh(4)) = 2;
select '55', atanh(0) = 0;
select '56', round(atanh(0.8)) = 1;

-- angular measures (2)
select '61', radians(0) = 0;
select '62', round(radians(180), 2) = round(pi(), 2);
select '63', degrees(0) = 0;
select '64', round(degrees(pi())) = 180;

-- other (2)
select '71', mod(10,3) = 1;
select '72', round(pi(), 5) = 3.14159;