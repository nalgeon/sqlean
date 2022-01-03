-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/math2

-- constants
select '01', round(m_e(), 3) = 2.718;
select '02', round(m_log2e(), 3) = 1.443;
select '03', round(m_log10e(), 3) = 0.434;
select '04', round(m_ln2(), 3) = 0.693;
select '05', round(m_ln10(), 3) = 2.303;
select '06', round(m_pi(), 3) = 3.142;
select '07', round(m_pi_2(), 3) = round(m_pi()/2, 3);
select '08', round(m_pi_4(), 3) = round(m_pi()/4, 3);
select '09', round(m_1_pi(), 3) = round(1/m_pi(), 3);
select '10', round(m_2_pi(), 3) = round(2/m_pi(), 3);
select '11', round(m_2_sqrtpi(), 3) = round(2/sqrt(m_pi()), 3);
select '12', round(m_sqrt2(), 3) = round(sqrt(2), 3);
select '13', round(m_sqrt1_2(), 3) = round(sqrt(0.5), 3);
select '14', round(m_deg2rad(), 3) = round(radians(1), 3);
select '15', round(m_rad2deg(), 3) = round(degrees(1), 3);

-- abs value
select '21', fabs(-5) = 5;
-- x * 2^y
select '22', ldexp(5, 4) = 5 * pow(2, 4);

-- x = mantissa * 2^exponent
-- 8 = 0.5      * 2^4
select '23', mantissa(8) = 0.5;
select '24', exponent(8) = 4;

-- integer and fractional parts
select '25', trunc(7.8) = 7.0;
select '26', round(frac(7.8),1) = 0.8;

-- hexadecimal to decimal
select '27', fromhex('FF') = 255;

-- bit functions
-- 6 = 110b: bit 0 is clear, bits 1 and 2 are set (right to left)
select '31', isset(6, 0) = false;
select '32', isset(6, 1, 2) = true;
select '33', isclr(6, 0) = true;
select '34', isclr(6, 1) = false;
select '35', isclr(6, 2) = false;

-- 42 = 101010b, mask 10 = 1010b
select '36', ismaskset(42, 10) = true;
-- 42 = 101010b, mask 5 = 101b
select '37', ismaskclr(42, 5) = true;
-- 42 = 101010b
select '38', bitmask(1, 3, 5) = 42;

-- 42 = 101010b, clear bits 1 and 3 -> 100000b = 32
select '39', setbits(32, 1, 3) = 42;
select '40', clrbits(42, 1, 3) = 32;

-- 111b = 7
select '41', aggbitmask(v) = 7 from (select 0 as v union select 1 union select 2);
