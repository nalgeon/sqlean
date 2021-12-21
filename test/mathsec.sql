-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/mathsec

-- constants
select round(m_e(), 3) = 2.718;
select round(m_log2e(), 3) = 1.443;
select round(m_log10e(), 3) = 0.434;
select round(m_ln2(), 3) = 0.693;
select round(m_ln10(), 3) = 2.303;
select round(m_pi(), 3) = 3.142;
select round(m_pi_2(), 3) = round(m_pi()/2, 3);
select round(m_pi_4(), 3) = round(m_pi()/4, 3);
select round(m_1_pi(), 3) = round(1/m_pi(), 3);
select round(m_2_pi(), 3) = round(2/m_pi(), 3);
select round(m_2_sqrtpi(), 3) = round(2/sqrt(m_pi()), 3);
select round(m_sqrt2(), 3) = round(sqrt(2), 3);
select round(m_sqrt1_2(), 3) = round(sqrt(0.5), 3);
select round(m_deg2rad(), 3) = round(radians(1), 3);
select round(m_rad2deg(), 3) = round(degrees(1), 3);

-- abs value
select fabs(-5) = 5;
-- x * 2^y
select ldexp(5, 4) = 5 * pow(2, 4);

-- x = mantissa * 2^exponent
-- 8 = 0.5      * 2^4
select mantissa(8) = 0.5;
select exponent(8) = 4;

-- integer and fractional parts
select trunc(7.8) = 7.0;
select round(frac(7.8),1) = 0.8;

-- hexadecimal to decimal
select fromhex('FF') = 255;

-- bit functions
-- 6 = 110b: bit 0 is clear, bits 1 and 2 are set (right to left)
select isset(6, 0) = false;
select isset(6, 1, 2) = true;
select isclr(6, 0) = true;
select isclr(6, 1) = false;
select isclr(6, 2) = false;

-- 42 = 101010b, mask 10 = 1010b
select ismaskset(42, 10) = true;
-- 42 = 101010b, mask 5 = 101b
select ismaskclr(42, 5) = true;
-- 42 = 101010b
select bitmask(1, 3, 5) = 42;

-- 42 = 101010b, clear bits 1 and 3 -> 100000b = 32
select setbits(32, 1, 3) = 42;
select clrbits(42, 1, 3) = 32;

-- 111b = 7
select aggbitmask(v) from (select 0 as v union select 1 union select 2);
