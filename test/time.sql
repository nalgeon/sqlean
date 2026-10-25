-- Copyright (c) 2024 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/time

-- 2011-11-18 00:00:00 = 1321574400 sec
-- 2011-11-18 15:56:35 = 1321631795 sec
-- 2011-11-18 15:56:35.666777888 = 1321631795666777888 nsec

-- time_date
select '01_01', time_to_unix(time_date(2011, 11, 18)) = 1321574400;
select '01_02', time_to_unix(time_date(2011, 11, 18, 15, 56, 35)) = 1321631795;
select '01_03', time_to_unix(time_date(2011, 11, 18, 15, 56, 35, 666777888)) = 1321631795;
select '01_04', time_to_nano(time_date(2011, 11, 18, 15, 56, 35, 666777888)) = 1321631795666777888;
select '01_05', time_to_unix(time_date(2011, 11, 18, 16, 56, 35, 0, 3600)) = 1321631795;
select '01_06', time_to_unix(time_date(2011, 11, 18, 14, 56, 35, 0, -3600)) = 1321631795;

-- time_get_x
-- 2011-11-18 15:56:35.666777888
select '11_01', time_get_year(time_unix(1321631795, 666777888)) = 2011;
select '11_02', time_get_month(time_unix(1321631795, 666777888)) = 11;
select '11_03', time_get_day(time_unix(1321631795, 666777888)) = 18;
select '11_04', time_get_hour(time_unix(1321631795, 666777888)) = 15;
select '11_05', time_get_minute(time_unix(1321631795, 666777888)) = 56;
select '11_06', time_get_second(time_unix(1321631795, 666777888)) = 35;
select '11_07', time_get_nano(time_unix(1321631795, 666777888)) = 666777888;
select '11_08', time_get_weekday(time_unix(1321631795, 666777888)) = 5;
select '11_09', time_get_yearday(time_unix(1321631795, 666777888)) = 322;
select '11_10', time_get_isoyear(time_unix(1321631795, 666777888)) = 2011;
select '11_11', time_get_isoweek(time_unix(1321631795, 666777888)) = 46;

-- time_get
-- 2011-11-18 15:56:35.666777888
select '12_01', time_get(time_unix(1321631795, 666777888), 'millennium') = 2;
select '12_02', time_get(time_unix(1321631795, 666777888), 'century') = 20;
select '12_03', time_get(time_unix(1321631795, 666777888), 'decade') = 201;
select '12_04', time_get(time_unix(1321631795, 666777888), 'year') = 2011;
select '12_05', time_get(time_unix(1321631795, 666777888), 'quarter') = 4;
select '12_06', time_get(time_unix(1321631795, 666777888), 'month') = 11;
select '12_07', time_get(time_unix(1321631795, 666777888), 'day') = 18;
select '12_08', time_get(time_unix(1321631795, 666777888), 'hour') = 15;
select '12_09', time_get(time_unix(1321631795, 666777888), 'minute') = 56;
select '12_10', time_get(time_unix(1321631795, 666777888), 'second') = 35.666777888;
select '12_11', time_get(time_unix(1321631795, 666777888), 'milli') = 666;
select '12_12', time_get(time_unix(1321631795, 666777888), 'micro') = 666777;
select '12_13', time_get(time_unix(1321631795, 666777888), 'nano') = 666777888;
select '12_14', time_get(time_unix(1321631795, 666777888), 'isoyear') = 2011;
select '12_15', time_get(time_unix(1321631795, 666777888), 'isoweek') = 46;
select '12_16', time_get(time_unix(1321631795, 666777888), 'isodow') = 5;
select '12_17', time_get(time_unix(1321631795, 666777888), 'yearday') = 322;
select '12_18', time_get(time_unix(1321631795, 666777888), 'weekday') = 5;
select '12_19', time_get(time_unix(1321631795, 666777888), 'epoch') = 1321631795.666777888;

-- time_unix
select '21_01', time_unix(1321574400) = time_date(2011, 11, 18);
select '21_02', time_unix(1321631795) = time_date(2011, 11, 18, 15, 56, 35);
select '21_03', time_unix(1321631795, 666777888) = time_date(2011, 11, 18, 15, 56, 35, 666777888);
select '21_04', time_unix(0, 1321631795666777888) = time_date(2011, 11, 18, 15, 56, 35, 666777888);

-- time_milli
select '22_01', time_milli(1321574400000) = time_date(2011, 11, 18);
select '22_02', time_milli(1321631795000) = time_date(2011, 11, 18, 15, 56, 35);
select '22_03', time_milli(1321631795666) = time_date(2011, 11, 18, 15, 56, 35, 666000000);

-- time_micro
select '23_01', time_micro(1321574400000000) = time_date(2011, 11, 18);
select '23_02', time_micro(1321631795000000) = time_date(2011, 11, 18, 15, 56, 35);
select '23_03', time_micro(1321631795666777) = time_date(2011, 11, 18, 15, 56, 35, 666777000);

-- time_nano
select '24_01', time_nano(1321574400000000000) = time_date(2011, 11, 18);
select '24_02', time_nano(1321631795000000000) = time_date(2011, 11, 18, 15, 56, 35);
select '24_03', time_nano(1321631795666777888) = time_date(2011, 11, 18, 15, 56, 35, 666777888);

-- to unix time
-- 2011-11-18 15:56:35.666777888
select '25_01', time_to_unix(time_unix(1321631795, 666777888)) = 1321631795;
select '25_02', time_to_milli(time_unix(1321631795, 666777888)) = 1321631795666;
select '25_03', time_to_micro(time_unix(1321631795, 666777888)) = 1321631795666777;
select '25_04', time_to_nano(time_unix(1321631795, 666777888)) = 1321631795666777888;

-- time_after
select '31_01', time_after(time_date(2011, 11, 19), time_date(2011, 11, 18)) = 1;
select '31_02', time_after(time_date(2011, 11, 18, 15, 56, 35), time_date(2011, 11, 18)) = 1;
select '31_03', time_after(time_date(2011, 11, 18, 15, 56, 35, 666777888), time_date(2011, 11, 18, 15, 56, 35)) = 1;

-- time_before
select '32_01', time_before(time_date(2011, 11, 18), time_date(2011, 11, 19)) = 1;
select '32_02', time_before(time_date(2011, 11, 18), time_date(2011, 11, 18, 15, 56, 35)) = 1;
select '32_03', time_before(time_date(2011, 11, 18, 15, 56, 35), time_date(2011, 11, 18, 15, 56, 35, 666777888)) = 1;

-- time_compare
select '33_01', time_compare(time_date(2011, 11, 18), time_date(2011, 11, 18)) = 0;
select '33_02', time_compare(time_date(2011, 11, 18), time_date(2011, 11, 19)) = -1;
select '33_03', time_compare(time_date(2011, 11, 19), time_date(2011, 11, 18)) = 1;
select '33_04', time_compare(time_date(2011, 11, 18, 15, 56, 35), time_date(2011, 11, 18, 15, 56, 35)) = 0;
select '33_05', time_compare(time_date(2011, 11, 18), time_date(2011, 11, 18, 15, 56, 35)) = -1;
select '33_06', time_compare(time_date(2011, 11, 18, 15, 56, 35), time_date(2011, 11, 18)) = 1;
select '33_07', time_compare(time_date(2011, 11, 18, 15, 56, 35, 666777888), time_date(2011, 11, 18, 15, 56, 35, 666777888)) = 0;
select '33_08', time_compare(time_date(2011, 11, 18, 15, 56, 35), time_date(2011, 11, 18, 15, 56, 35, 666777888)) = -1;
select '33_09', time_compare(time_date(2011, 11, 18, 15, 56, 35, 666777888), time_date(2011, 11, 18, 15, 56, 35)) = 1;

-- time_equal
select '34_01', time_date(2011, 11, 18) = time_date(2011, 11, 18);
select '34_02', time_date(2011, 11, 18) <> time_date(2011, 11, 19);
select '34_03', time_equal(time_date(2011, 11, 18), time_date(2011, 11, 18)) = 1;
select '34_04', time_equal(time_date(2011, 11, 18), time_date(2011, 11, 19)) = 0;
select '34_05', time_date(2011, 11, 18, 15, 56, 35) = time_date(2011, 11, 18, 15, 56, 35);
select '34_06', time_date(2011, 11, 18, 15, 56, 35) <> time_date(2011, 11, 18);
select '34_07', time_equal(time_date(2011, 11, 18, 15, 56, 35), time_date(2011, 11, 18, 15, 56, 35)) = 1;
select '34_08', time_equal(time_date(2011, 11, 18, 15, 56, 35), time_date(2011, 11, 18)) = 0;
select '34_09', time_date(2011, 11, 18, 15, 56, 35, 666777888) = time_date(2011, 11, 18, 15, 56, 35, 666777888);
select '34_10', time_date(2011, 11, 18, 15, 56, 35) <> time_date(2011, 11, 18, 15, 56, 35, 666777888);
select '34_11', time_equal(time_date(2011, 11, 18, 15, 56, 35, 666777888), time_date(2011, 11, 18, 15, 56, 35, 666777888)) = 1;
select '34_12', time_equal(time_date(2011, 11, 18, 15, 56, 35), time_date(2011, 11, 18, 15, 56, 35, 666777888)) = 0;

-- time_add
select '41_01', time_add(time_date(2011, 11, 18), 24*dur_h()) = time_date(2011, 11, 19);
select '41_02', time_add(time_date(2011, 11, 18, 15, 56, 35), 3*dur_h()) = time_date(2011, 11, 18, 18, 56, 35);
select '41_03', time_add(time_date(2011, 11, 18, 15, 56, 35), 60*dur_m()) = time_date(2011, 11, 18, 16, 56, 35);
select '41_04', time_add(time_date(2011, 11, 18, 15, 56, 35), 5*dur_m()) = time_date(2011, 11, 18, 16, 1, 35);
select '41_05', time_add(time_date(2011, 11, 18, 15, 56, 35), 60*dur_s()) = time_date(2011, 11, 18, 15, 57, 35);
select '41_06', time_add(time_date(2011, 11, 18, 15, 56, 35), 5*dur_s()) = time_date(2011, 11, 18, 15, 56, 40);
select '41_07', time_add(time_unix(1321631795, 0), 5*dur_s()) = time_unix(1321631795, 5000000000);
select '41_08', time_add(time_unix(1321631795, 0), 5*dur_ms()) = time_unix(1321631795, 5000000);
select '41_09', time_add(time_unix(1321631795, 0), 5*dur_us()) = time_unix(1321631795, 5000);
select '41_10', time_add(time_unix(1321631795, 0), 5*dur_ns()) = time_unix(1321631795, 5);
select '41_11', time_add(time_unix(1321631795, 0), 5) = time_unix(1321631795, 5);

-- time_sub
select '42_01', time_sub(time_date(2011, 11, 19), time_date(2011, 11, 18)) = 24*dur_h();
select '42_02', time_sub(time_date(2011, 11, 18, 18, 56, 35), time_date(2011, 11, 18, 15, 56, 35)) = 3*dur_h();
select '42_03', time_sub(time_date(2011, 11, 18, 16, 56, 35), time_date(2011, 11, 18, 15, 56, 35)) = 60*dur_m();
select '42_04', time_sub(time_date(2011, 11, 18, 16, 1, 35), time_date(2011, 11, 18, 15, 56, 35)) = 5*dur_m();
select '42_05', time_sub(time_date(2011, 11, 18, 15, 57, 35), time_date(2011, 11, 18, 15, 56, 35)) = 60*dur_s();
select '42_06', time_sub(time_date(2011, 11, 18, 15, 56, 40), time_date(2011, 11, 18, 15, 56, 35)) = 5*dur_s();
select '42_07', time_sub(time_unix(1321631795, 5000000000), time_unix(1321631795, 0)) = 5*dur_s();
select '42_08', time_sub(time_unix(1321631795, 5000000), time_unix(1321631795, 0)) = 5*dur_ms();
select '42_09', time_sub(time_unix(1321631795, 5000), time_unix(1321631795, 0)) = 5*dur_us();
select '42_10', time_sub(time_unix(1321631795, 5), time_unix(1321631795, 0)) = 5*dur_ns();
select '42_11', time_sub(time_unix(1321631795, 5), time_unix(1321631795, 0)) = 5;

-- time_since, time_until
select '43_01', time_since(time_add(time_now(), -3*dur_h()-dur_s())) / dur_h() = 3;
select '43_02', time_until(time_add(time_now(), 3*dur_h()+dur_s())) / dur_h() = 3;

-- time_add_date: years
select '44_01', time_add_date(time_date(2011, 11, 18), 0) = time_date(2011, 11, 18);
select '44_02', time_add_date(time_date(2011, 11, 18), 1) = time_date(2012, 11, 18);
select '44_03', time_add_date(time_date(2011, 11, 18), 5) = time_date(2016, 11, 18);
select '44_04', time_add_date(time_date(2011, 11, 18), -1) = time_date(2010, 11, 18);
select '44_05', time_add_date(time_date(2011, 11, 18), -5) = time_date(2006, 11, 18);

-- time_add_date: years, months
select '44_11', time_add_date(time_date(2011, 11, 18), 0, 0) = time_date(2011, 11, 18);
select '44_12', time_add_date(time_date(2011, 11, 18), 0, 1) = time_date(2011, 12, 18);
select '44_13', time_add_date(time_date(2011, 11, 18), 0, 5) = time_date(2012, 4, 18);
select '44_14', time_add_date(time_date(2011, 11, 18), 0, 18) = time_date(2013, 5, 18);
select '44_15', time_add_date(time_date(2011, 11, 18), 0, -1) = time_date(2011, 10, 18);
select '44_16', time_add_date(time_date(2011, 11, 18), 0, -5) = time_date(2011, 6, 18);
select '44_17', time_add_date(time_date(2011, 11, 18), 0, -18) = time_date(2010, 5, 18);
select '44_18', time_add_date(time_date(2011, 11, 18), 3, 5) = time_date(2015, 4, 18);
select '44_19', time_add_date(time_date(2011, 11, 18), 3, -5) = time_date(2014, 6, 18);

-- time_add_date: years, months, days
select '44_21', time_add_date(time_date(2011, 11, 18), 0, 0, 0) = time_date(2011, 11, 18);
select '44_22', time_add_date(time_date(2011, 11, 18), 0, 0, 1) = time_date(2011, 11, 19);
select '44_23', time_add_date(time_date(2011, 11, 18), 0, 0, 5) = time_date(2011, 11, 23);
select '44_24', time_add_date(time_date(2011, 11, 18), 0, 0, 30) = time_date(2011, 12, 18);
select '44_25', time_add_date(time_date(2011, 11, 18), 0, 0, 500) = time_date(2013, 4, 1);
select '44_26', time_add_date(time_date(2011, 11, 18), 0, 0, -1) = time_date(2011, 11, 17);
select '44_27', time_add_date(time_date(2011, 11, 18), 0, 0, -5) = time_date(2011, 11, 13);
select '44_28', time_add_date(time_date(2011, 11, 18), 0, 0, -30) = time_date(2011, 10, 19);
select '44_29', time_add_date(time_date(2011, 11, 18), 0, 0, -500) = time_date(2010, 7, 6);
select '44_30', time_add_date(time_date(2011, 11, 18), 0, 5, 10) = time_date(2012, 4, 28);
select '44_31', time_add_date(time_date(2011, 11, 18), 0, 5, -10) = time_date(2012, 4, 8);
select '44_32', time_add_date(time_date(2011, 11, 18), 3, 5, 10) = time_date(2015, 4, 28);
select '44_33', time_add_date(time_date(2011, 11, 18), -3, -5, -10) = time_date(2008, 6, 8);
select '44_34', time_add_date(time_date(2011, 11, 18), 3, 18, 500) = time_date(2017, 9, 30);

-- time_trunc
-- 2011-11-18 15:56:35.666777888
select '51_01', time_trunc(time_unix(1321631795, 666777888), 'millennium') = time_date(2000, 1, 1);
select '51_02', time_trunc(time_unix(1321631795, 666777888), 'century') = time_date(2000, 1, 1);
select '51_03', time_trunc(time_unix(1321631795, 666777888), 'decade') = time_date(2010, 1, 1);
select '51_04', time_trunc(time_unix(1321631795, 666777888), 'year') = time_date(2011, 1, 1);
select '51_05', time_trunc(time_unix(1321631795, 666777888), 'quarter') = time_date(2011, 10, 1);
select '51_06', time_trunc(time_unix(1321631795, 666777888), 'month') = time_date(2011, 11, 1);
-- week truncates to the Monday of the current ISO week (2011-11-18 was a Friday)
select '51_07', time_trunc(time_unix(1321631795, 666777888), 'week') = time_date(2011, 11, 14);
select '51_08', time_trunc(time_date(2023, 1, 4), 'week') = time_date(2023, 1, 2);
select '51_09', time_trunc(time_date(2023, 1, 1), 'week') = time_date(2022, 12, 26);
select '51_10', time_trunc(time_date(2024, 3, 13), 'week') = time_date(2024, 3, 11);
select '51_11', time_trunc(time_date(2023, 12, 31), 'week') = time_date(2023, 12, 25);
select '51_12', time_trunc(time_unix(1321631795, 666777888), 'day') = time_date(2011, 11, 18);
select '51_13', time_trunc(time_unix(1321631795, 666777888), 'hour') = time_date(2011, 11, 18, 15, 0, 0);
select '51_14', time_trunc(time_unix(1321631795, 666777888), 'minute') = time_date(2011, 11, 18, 15, 56, 0);
select '51_15', time_trunc(time_unix(1321631795, 666777888), 'second') = time_date(2011, 11, 18, 15, 56, 35);
select '51_16', time_trunc(time_unix(1321631795, 666777888), 'milli') = time_date(2011, 11, 18, 15, 56, 35, 666000000);
select '51_17', time_trunc(time_unix(1321631795, 666777888), 'micro') = time_date(2011, 11, 18, 15, 56, 35, 666777000);

-- truncate to custom duration
-- 2011-11-18 15:56:35.666777888
select '52_01', time_trunc(time_unix(1321631795, 666777888), dur_s()) = time_date(2011, 11, 18, 15, 56, 35);
select '52_02', time_trunc(time_unix(1321631795, 666777888), 30*dur_s()) = time_date(2011, 11, 18, 15, 56, 30);
select '52_03', time_trunc(time_unix(1321631795, 666777888), dur_m()) = time_date(2011, 11, 18, 15, 56, 0);
select '52_04', time_trunc(time_unix(1321631795, 666777888), 30*dur_m()) = time_date(2011, 11, 18, 15, 30, 0);
select '52_05', time_trunc(time_unix(1321631795, 666777888), dur_h()) = time_date(2011, 11, 18, 15, 0, 0);
select '52_06', time_trunc(time_unix(1321631795, 666777888), 12*dur_h()) = time_date(2011, 11, 18, 12, 0, 0);

-- time_round
-- 2011-11-18 15:56:35.666777888
select '53_01', time_round(time_unix(1321631795, 666777888), dur_s()) = time_date(2011, 11, 18, 15, 56, 36);
select '53_02', time_round(time_unix(1321631795, 666777888), 30*dur_s()) = time_date(2011, 11, 18, 15, 56, 30);
select '53_03', time_round(time_unix(1321631795, 666777888), dur_m()) = time_date(2011, 11, 18, 15, 57, 0);
select '53_04', time_round(time_unix(1321631795, 666777888), 30*dur_m()) = time_date(2011, 11, 18, 16, 00, 0);
select '53_05', time_round(time_unix(1321631795, 666777888), dur_h()) = time_date(2011, 11, 18, 16, 0, 0);
select '53_06', time_round(time_unix(1321631795, 666777888), 12*dur_h()) = time_date(2011, 11, 18, 12, 0, 0);

-- time_fmt_iso
-- 2011-11-18 15:56:35.666777888
select '61_01', time_fmt_iso(time_unix(1321631795, 666777888)) = '2011-11-18T15:56:35.666777888Z';
select '61_02', time_fmt_iso(time_unix(1321631795, 666777888), 0) = '2011-11-18T15:56:35.666777888Z';
select '61_03', time_fmt_iso(time_unix(1321631795, 666777888), 3*3600+30*60) = '2011-11-18T19:26:35.666777888+03:30';
select '61_04', time_fmt_iso(time_unix(1321631795, 666777888), -3*3600-30*60) = '2011-11-18T12:26:35.666777888-03:30';
select '61_05', time_fmt_iso(time_unix(1321631795, 0)) = '2011-11-18T15:56:35Z';
select '61_06', time_fmt_iso(time_unix(1321631795, 0), 0) = '2011-11-18T15:56:35Z';
select '61_07', time_fmt_iso(time_unix(1321631795, 0), 3*3600+30*60) = '2011-11-18T19:26:35+03:30';
select '61_08', time_fmt_iso(time_unix(1321631795, 0), -3*3600-30*60) = '2011-11-18T12:26:35-03:30';
-- negative sub-hour offsets must keep their sign
select '61_09', time_fmt_iso(time_unix(0, 0), -1800) = '1969-12-31T23:30:00-00:30';
select '61_10', time_fmt_iso(time_unix(0, 0), -1799) = '1969-12-31T23:30:01-00:29';
select '61_11', time_fmt_iso(time_unix(0, 0), 19800) = '1970-01-01T05:30:00+05:30';

-- time_fmt_datetime
-- 2011-11-18 15:56:35.666777888
select '62_01', time_fmt_datetime(time_unix(1321631795, 666777888)) = '2011-11-18 15:56:35';
select '62_02', time_fmt_datetime(time_unix(1321631795, 666777888), 0) = '2011-11-18 15:56:35';
select '62_03', time_fmt_datetime(time_unix(1321631795, 666777888), 3*3600+30*60) = '2011-11-18 19:26:35';
select '62_04', time_fmt_datetime(time_unix(1321631795, 666777888), -3*3600-30*60) = '2011-11-18 12:26:35';
select '62_05', time_fmt_datetime(time_unix(1321631795, 0)) = '2011-11-18 15:56:35';
select '62_06', time_fmt_datetime(time_unix(1321631795, 0), 0) = '2011-11-18 15:56:35';
select '62_07', time_fmt_datetime(time_unix(1321631795, 0), 3*3600+30*60) = '2011-11-18 19:26:35';
select '62_08', time_fmt_datetime(time_unix(1321631795, 0), -3*3600-30*60) = '2011-11-18 12:26:35';

-- time_fmt_date
-- 2011-11-18 15:56:35.666777888
select '63_01', time_fmt_date(time_unix(1321631795, 666777888)) = '2011-11-18';
select '63_02', time_fmt_date(time_unix(1321631795, 0)) = '2011-11-18';
select '63_03', time_fmt_date(time_unix(1321631795, 0), 12*3600) = '2011-11-19';
select '63_04', time_fmt_date(time_unix(1321631795, 0), -12*3600) = '2011-11-18';

-- time_fmt_time
-- 2011-11-18 15:56:35.666777888
select '64_01', time_fmt_time(time_unix(1321631795, 666777888)) = '15:56:35';
select '64_02', time_fmt_time(time_unix(1321631795, 0)) = '15:56:35';
select '64_03', time_fmt_time(time_unix(1321631795, 0), 3*3600+30*60) = '19:26:35';
select '64_04', time_fmt_time(time_unix(1321631795, 0), -3*3600-30*60) = '12:26:35';

-- time_parse
-- 2011-11-18 15:56:35.666777888
select '65_01', time_parse('2011-11-18T15:56:35.666777888Z') = time_unix(1321631795, 666777888);
select '65_02', time_parse('2011-11-18T19:26:35.666777888+03:30') = time_unix(1321631795, 666777888);
select '65_03', time_parse('2011-11-18T12:26:35.666777888-03:30') = time_unix(1321631795, 666777888);
select '65_04', time_parse('2011-11-18T15:56:35Z') = time_unix(1321631795, 0);
select '65_05', time_parse('2011-11-18T19:26:35+03:30') = time_unix(1321631795, 0);
select '65_06', time_parse('2011-11-18T12:26:35-03:30') = time_unix(1321631795, 0);
select '65_07', time_parse('2011-11-18 15:56:35') = time_unix(1321631795, 0);
select '65_08', time_parse('2011-11-18') = time_date(2011, 11, 18);
select '65_09', time_parse('15:56:35') = time_date(1, 1, 1, 15, 56, 35);
select '65_10', time_parse(null) is null;

-- time_parse: fractional seconds, one to nine digits
select '66_01', time_parse('2011-11-18T15:56:35.6Z') = time_unix(1321631795, 600000000);
select '66_02', time_parse('2011-11-18T15:56:35.66Z') = time_unix(1321631795, 660000000);
select '66_03', time_parse('2011-11-18T15:56:35.666Z') = time_unix(1321631795, 666000000);
select '66_04', time_parse('2011-11-18T15:56:35.6667Z') = time_unix(1321631795, 666700000);
select '66_05', time_parse('2011-11-18T15:56:35.666777Z') = time_unix(1321631795, 666777000);
select '66_06', time_parse('2011-11-18T15:56:35.666777888Z') = time_unix(1321631795, 666777888);
-- digits past the ninth are discarded
select '66_07', time_parse('2011-11-18T15:56:35.6667778889Z') = time_unix(1321631795, 666777888);
-- zero fraction
select '66_08', time_parse('2011-11-18T15:56:35.0000Z') = time_unix(1321631795, 0);
select '66_09', time_fmt_iso(time_parse('2011-11-18T15:56:35.0000Z')) = '2011-11-18T15:56:35Z';

-- time_parse: separator, fraction and zone are independent
select '67_01', time_parse('2011-11-18 15:56:35.666777') = time_unix(1321631795, 666777000);
select '67_02', time_parse('2011-11-18T15:56:35.666777') = time_unix(1321631795, 666777000);
select '67_03', time_parse('2011-11-18 15:56:35.666777Z') = time_unix(1321631795, 666777000);
select '67_04', time_parse('2011-11-18 19:26:35.666+03:30') = time_unix(1321631795, 666000000);
select '67_05', time_parse('2011-11-18 19:26:35+03:30') = time_unix(1321631795, 0);
select '67_06', time_parse('15:56:35.666Z') = time_date(1, 1, 1, 15, 56, 35, 666000000);
-- zoned time-only forms
select '67_07', time_parse('15:56:35+03:30') = time_date(1, 1, 1, 12, 26, 35);
select '67_08', time_parse('15:56:35-03:30') = time_date(1, 1, 1, 19, 26, 35);

-- time_parse: unparseable values return null
select '68_01', time_parse('2011-11-18T15:56:35.666777888Y') is null;
select '68_03', time_parse(' 2011-11-18T15:56:35Z') is null;
select '68_11', time_parse('2011-11-18T15:56:35.Z') is null;
select '68_12', time_parse('2011-11-18T15:56:35+0500') is null;
select '68_13', time_parse('2011-1-8T15:56:35Z') is null;
select '68_14', time_parse('2011-11-18X15:56:35Z') is null;
select '68_15', time_parse('garbage') is null;
-- RFC 3339 section 5.6 allows a lowercase 't' and 'z'; both are accepted
select '68_16', time_parse('2011-11-18t15:56:35Z') = time_unix(1321631795, 0);
select '68_17', time_parse('2011-11-18T15:56:35z') = time_unix(1321631795, 0);

-- time_parse: boundary values
select '68_20', time_parse('9999-12-31T23:59:59.999999999Z') = time_date(9999, 12, 31, 23, 59, 59, 999999999);
select '68_21', time_parse('0000-01-01T00:00:00Z') = time_date(0, 1, 1);
select '68_22', time_parse('2011-11-18T16:56:35+23:59') = time_date(2011, 11, 17, 16, 57, 35);
select '68_23', time_parse('2011-11-18T16:56:35-23:59') = time_date(2011, 11, 19, 16, 55, 35);
-- text with an embedded NUL is unparseable
select '68_24', time_parse(cast('2011-11-18' || char(0) || 'junk' as text)) is null;

-- ASCII whitespace separators
select '68_25', time_parse('2011-11-18  15:56:35') = time_unix(1321631795, 0);
select '68_26', time_parse('2011-11-18     15:56:35') = time_unix(1321631795, 0);
select '68_27', time_parse(char(50,48,49,49,45,49,49,45,49,56,9,49,53,58,53,54,58,51,53)) = time_unix(1321631795, 0);

select '72_01', time_parse(char(50,48,49,49,45,49,49,45,49,56,11,49,53,58,53,54,58,51,53)) = time_unix(1321631795, 0);
select '72_02', time_parse(char(50,48,49,49,45,49,49,45,49,56,12,49,53,58,53,54,58,51,53)) = time_unix(1321631795, 0);
-- whitespace separates the date from the time and nothing else
select '72_03', time_parse('2011-11-18T20:56:35' || char(9) || '+05:00') is null;
select '72_04', time_parse('2011-11-18T15:56:35 ') is null;
-- a fraction follows seconds, never HH:MM
select '72_05', time_parse('15:56.5Z') is null;
select '72_06', time_parse('2011-11-18T15:56.5Z') is null;
-- the separators inside the date and the time are required
select '72_07', time_parse('2011-11X18T15:56:35Z') is null;
select '72_08', time_parse('2011-11-18T15:56X35Z') is null;

-- out-of-range timezone offsets normalize; malformed offsets are rejected
select '73_01', time_parse('2011-11-18T15:56:35+24:00') = time_date(2011, 11, 17, 15, 56, 35);
select '73_02', time_parse('2011-11-18T15:56:35+05:60') = time_date(2011, 11, 18, 9, 56, 35);
select '73_03', time_parse('2011-11-18T15:56:35+0500') is null;
select '73_04', time_parse('2011-11-18T15:56:35+05:xx') is null;
select '73_05', time_parse('2011-11-18T15:56:35+05X00') is null;

-- time_parse: out-of-range date and clock fields are normalized, not rejected
select '70_01', time_parse('2011-13-18T15:56:35Z') = time_date(2012, 1, 18, 15, 56, 35);
select '70_02', time_parse('2011-11-32T15:56:35Z') = time_date(2011, 12, 2, 15, 56, 35);
select '70_03', time_parse('2011-11-18T24:00:00Z') = time_date(2011, 11, 19);
select '70_04', time_parse('2011-11-18T15:60:35Z') = time_date(2011, 11, 18, 16, 0, 35);
select '70_05', time_parse('2011-11-18T15:56:60Z') = time_date(2011, 11, 18, 15, 57, 0);
select '70_06', time_parse('2012-06-30T23:59:60Z') = time_date(2012, 7, 1);
select '70_07', time_parse('2011-00-18T15:56:35Z') = time_date(2010, 12, 18, 15, 56, 35);

-- time_parse: nonexistent dates are normalized forward
select '69_01', time_parse('2011-02-30') = time_date(2011, 3, 2);
select '69_02', time_parse('2011-04-31') = time_date(2011, 5, 1);
select '69_03', time_parse('2011-02-29') = time_date(2011, 3, 1);
select '69_04', time_parse('2012-02-29') = time_date(2012, 2, 29);

-- duration constants
select '71_01', dur_ns() = 1;
select '71_02', dur_us() = 1000*dur_ns();
select '71_03', dur_ms() = 1000*dur_us();
select '71_04', dur_s() = 1000*dur_ms();
select '71_05', dur_m() = 60*dur_s();
select '71_06', dur_h() = 60*dur_m();

-- storing time as blob
create table data (
    id integer primary key,
    t blob
);
insert into data values (1, time_unix(1321631790));
insert into data values (2, time_unix(1321631795));
insert into data values (3, time_unix(1321631795, 666777888));
select '81_01', t = time_date(2011, 11, 18, 15, 56, 30) from data where id = 1;
select '81_02', t = time_date(2011, 11, 18, 15, 56, 35) from data where id = 2;
select '81_03', t = time_date(2011, 11, 18, 15, 56, 35, 666777888) from data where id = 3;
select '81_04', length(t) = 13 from data where id = 1;
select '81_05', max(t) = time_date(2011, 11, 18, 15, 56, 35, 666777888) from data;
select '81_06', min(t) = time_date(2011, 11, 18, 15, 56, 30) from data;

-- null propagation
-- every argument-taking function returns null if any argument is null
select '90_01', time_date(null, 11, 18) is null;
select '90_02', time_date(2011, null, 18) is null;
select '90_03', time_date(2011, 11, null) is null;
select '90_04', time_date(2011, 11, 18, null, 56, 35) is null;
select '90_05', time_date(2011, 11, 18, 15, null, 35) is null;
select '90_06', time_date(2011, 11, 18, 15, 56, null) is null;
select '90_07', time_date(2011, 11, 18, 15, 56, 35, null) is null;
select '90_08', time_date(2011, 11, 18, 15, 56, 35, 0, null) is null;
select '90_09', time_get_year(null) is null;
select '90_10', time_get_month(null) is null;
select '90_11', time_get_day(null) is null;
select '90_12', time_get_hour(null) is null;
select '90_13', time_get_minute(null) is null;
select '90_14', time_get_second(null) is null;
select '90_15', time_get_nano(null) is null;
select '90_16', time_get_weekday(null) is null;
select '90_17', time_get_yearday(null) is null;
select '90_18', time_get_isoyear(null) is null;
select '90_19', time_get_isoweek(null) is null;
select '90_20', time_get(null, 'year') is null;
select '90_21', time_get(time_unix(1321631795), null) is null;
select '90_22', time_unix(null) is null;
select '90_23', time_unix(1321631795, null) is null;
select '90_24', time_milli(null) is null;
select '90_25', time_micro(null) is null;
select '90_26', time_nano(null) is null;
select '90_27', time_to_unix(null) is null;
select '90_28', time_to_milli(null) is null;
select '90_29', time_to_micro(null) is null;
select '90_30', time_to_nano(null) is null;
select '90_31', time_after(null, time_unix(1321631795)) is null;
select '90_32', time_after(time_unix(1321631795), null) is null;
select '90_33', time_before(null, time_unix(1321631795)) is null;
select '90_34', time_compare(null, time_unix(1321631795)) is null;
select '90_35', time_equal(time_unix(1321631795), null) is null;
select '90_36', time_add(null, dur_h()) is null;
select '90_37', time_add(time_unix(1321631795), null) is null;
select '90_38', time_sub(null, time_unix(1321631795)) is null;
select '90_39', time_sub(time_unix(1321631795), null) is null;
select '90_40', time_since(null) is null;
select '90_41', time_until(null) is null;
select '90_42', time_add_date(null, 1) is null;
select '90_43', time_add_date(time_unix(1321631795), null) is null;
select '90_44', time_add_date(time_unix(1321631795), 1, null) is null;
select '90_45', time_add_date(time_unix(1321631795), 1, 1, null) is null;
select '90_46', time_trunc(null, 'day') is null;
select '90_47', time_trunc(time_unix(1321631795), null) is null;
select '90_48', time_round(null, dur_h()) is null;
select '90_49', time_round(time_unix(1321631795), null) is null;
select '90_50', time_fmt_iso(null) is null;
select '90_51', time_fmt_iso(time_unix(1321631795), null) is null;
select '90_52', time_fmt_datetime(null) is null;
select '90_53', time_fmt_date(null) is null;
select '90_54', time_fmt_time(null) is null;
select '90_55', time_parse(null) is null;

-- null propagation: postgres compatibility layer
select '91_01', age(null, time_unix(1321631795)) is null;
select '91_02', age(time_unix(1321631795), null) is null;
select '91_03', date_add(null, dur_h()) is null;
select '91_04', date_add(time_unix(1321631795), null) is null;
select '91_05', date_part(null, time_unix(1321631795)) is null;
select '91_06', date_part('year', null) is null;
select '91_07', date_trunc(null, time_unix(1321631795)) is null;
select '91_08', date_trunc('day', null) is null;
select '91_09', make_date(null, 11, 18) is null;
select '91_10', make_timestamp(2011, 11, 18, 15, 56, null) is null;
select '91_11', to_timestamp(null) is null;

-- null propagation: functions without arguments are unaffected
select '92_01', time_now() is not null;
select '92_02', now() is not null;
select '92_03', dur_h() = 3600000000000;

-- null propagation: null is checked before types, so it does not depend on
-- argument order and masks an invalid value in another argument
select '93_01', time_add(null, 'not a duration') is null;
select '93_02', time_add('not a time', null) is null;
select '93_03', time_get(null, 'not a field') is null;
select '93_04', time_get('not a time', null) is null;

-- null propagation: composes with outer joins and aggregates
create table t90_users(id integer primary key, name text);
create table t90_logins(user_id integer, at blob);
insert into t90_users values (1, 'ann'), (2, 'bob'), (3, 'cid');
insert into t90_logins values (1, time_date(2024, 1, 15)), (3, time_date(2024, 3, 2));
select '94_01', count(*) = 3 from (
    select time_fmt_date(l.at) as d from t90_users u
    left join t90_logins l on l.user_id = u.id
);
select '94_02', count(*) = 2 from (
    select time_fmt_date(l.at) as d from t90_users u
    left join t90_logins l on l.user_id = u.id
) where d is not null;
select '94_03', max(time_to_unix(l.at)) = time_to_unix(time_date(2024, 3, 2))
    from t90_users u left join t90_logins l on l.user_id = u.id;
select '94_04', group_concat(name) = 'cid' from (
    select u.name from t90_users u
    left join t90_logins l on l.user_id = u.id
    where time_after(l.at, time_date(2024, 2, 1))
);

-- null propagation: a plain expression index covers a nullable column
create table t90_events(at blob);
insert into t90_events values (null), (time_date(2024, 1, 15)), (time_date(2024, 3, 2));
create index t90_events_year on t90_events(time_get_year(at));
select '95_01', count(*) = 3 from t90_events;
select '95_02', count(*) = 2 from t90_events where time_get_year(at) = 2024;
insert into t90_events values (null);
select '95_03', count(*) = 4 from t90_events;

-- null propagation: generated columns accept a null row
create table t90_gen(
    at blob,
    y integer generated always as (time_get_year(at)) stored,
    m integer generated always as (time_get_month(at)) virtual
);
insert into t90_gen(at) values (null), (time_date(2024, 5, 1));
select '95_04', count(*) = 2 from t90_gen;
select '95_05', y is null and m is null from t90_gen where at is null;
select '95_06', y = 2024 and m = 5 from t90_gen where at is not null;

-- null propagation: a CHECK constraint is satisfied by a null result and does
-- not reject a null value; NOT NULL requires one
create table t96_opt(at blob check (time_get_year(at) >= 2000));
insert into t96_opt values (null), (time_date(2024, 1, 15));
select '96_01', count(*) = 2 from t96_opt;
create table t96_req(at blob not null check (time_get_year(at) >= 2000));
insert or ignore into t96_req values (null), (time_date(2024, 1, 15));
select '96_02', count(*) = 1 from t96_req;

-- bad data is null: wrong types, wrong-size and corrupt blobs
-- 13 bytes of version 0, and a version 1 blob with nsec > 999999999
select '97_01', time_get_year('2011-11-18') is null;
select '97_02', time_get_year(1321631795) is null;
select '97_03', time_get_year(x'0100') is null;
select '97_04', time_get_year(zeroblob(13)) is null;
select '97_05', time_get_year(x'01000000000000000040000000') is null;
select '97_06', time_get_isoweek('not a time') is null;
select '97_07', time_get('not a time', 'year') is null;
select '97_08', date_part('year', 'not a time') is null;
select '97_09', time_to_unix(x'00') is null;
select '97_10', time_after('not a time', time_now()) is null;
select '97_11', time_after(time_now(), 'not a time') is null;
select '97_12', time_add('not a time', dur_h()) is null;
select '97_13', time_add(time_now(), 'not a duration') is null;
select '97_14', time_add(time_now(), 1.5) is null;
select '97_15', time_sub('not a time', time_now()) is null;
select '97_16', time_since('not a time') is null;
select '97_17', time_until('not a time') is null;
select '97_18', time_add_date('not a time', 1) is null;
select '97_19', time_add_date(time_now(), 'x') is null;
select '97_20', time_add_date(time_now(), 1, 'x') is null;
select '97_21', time_add_date(time_now(), 1, 1, 'x') is null;
select '97_22', time_trunc('not a time', 'day') is null;
select '97_23', time_trunc(time_now(), 1.5) is null;
select '97_24', time_trunc(time_now(), x'01') is null;
select '97_25', date_trunc('day', 'not a time') is null;
select '97_26', time_round('not a time', dur_h()) is null;
select '97_27', time_round(time_now(), 'x') is null;
select '97_28', time_fmt_iso('not a time') is null;
select '97_29', time_fmt_iso(time_now(), 'x') is null;
select '97_30', time_fmt_date(x'01') is null;
select '97_31', time_date('x', 11, 18) is null;
select '97_32', time_date(2011, 11, 18, 15, 56, 'x') is null;
select '97_33', time_date(2011, 11, 18, 15, 56, 35, 0, 'x') is null;
select '97_34', time_unix('x') is null;
select '97_35', time_unix(1321631795, 'x') is null;
select '97_36', time_milli('x') is null;
select '97_37', time_nano(1.5) is null;
select '97_38', time_parse(x'323031312d31312d3138') is null;
select '97_39', time_parse(20111118) is null;
select '97_40', make_date('x', 11, 18) is null;
select '97_41', to_timestamp('x') is null;
select '97_42', age('not a time', time_now()) is null;

-- integer arguments accept numeric text, as SQLite's own functions do
select '98_01', time_date('2011', '11', '18') = time_date(2011, 11, 18);
select '98_02', time_unix('1321631795') = time_unix(1321631795);
select '98_03', time_add(time_unix(0), '1000000000') = time_unix(1);
select '98_04', time_add_date(time_date(2011, 11, 18), '1') = time_date(2012, 11, 18);
select '98_05', time_fmt_iso(time_unix(0), '3600') = '1970-01-01T01:00:00+01:00';
select '98_06', time_trunc(time_unix(1321631795), '3600000000000') = time_trunc(time_unix(1321631795), dur_h());

-- integer arguments that don't fit in 32 bits are null, not truncated
-- 4294969307 = 2^32 + 2011
select '99_01', time_date(4294969307, 11, 18) is null;
select '99_02', time_date(2011, 11, 18, 15, 56, -2147483649) is null;
select '99_03', time_add_date(time_date(2011, 11, 18), 4294967297) is null;
select '99_04', time_add_date(time_date(2011, 11, 18), 0, 0, 4294967297) is null;
select '99_05', time_fmt_iso(time_unix(0), 4294967296) is null;
-- the 32-bit boundaries themselves are accepted
select '99_06', time_date(2011, 11, 18, 15, 56, 2147483647) is not null;
select '99_07', time_fmt_iso(time_unix(0), -2147483648) is not null;
