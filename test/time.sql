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
select '51_07', time_trunc(time_unix(1321631795, 666777888), 'week') = time_date(2011, 11, 12);
select '51_08', time_trunc(time_unix(1321631795, 666777888), 'day') = time_date(2011, 11, 18);
select '51_09', time_trunc(time_unix(1321631795, 666777888), 'hour') = time_date(2011, 11, 18, 15, 0, 0);
select '51_10', time_trunc(time_unix(1321631795, 666777888), 'minute') = time_date(2011, 11, 18, 15, 56, 0);
select '51_11', time_trunc(time_unix(1321631795, 666777888), 'second') = time_date(2011, 11, 18, 15, 56, 35);
select '51_12', time_trunc(time_unix(1321631795, 666777888), 'milli') = time_date(2011, 11, 18, 15, 56, 35, 666000000);
select '51_13', time_trunc(time_unix(1321631795, 666777888), 'micro') = time_date(2011, 11, 18, 15, 56, 35, 666777000);

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
select '62_04', time_fmt_date(time_unix(1321631795, 0), 12*3600) = '2011-11-19';
select '62_05', time_fmt_date(time_unix(1321631795, 0), -12*3600) = '2011-11-18';

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
