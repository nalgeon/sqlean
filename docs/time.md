# time: High-precision date/time in SQLite

The `sqlean-time` extension provides functionality for working with time and duration with nanosecond precision.

[Concepts](#concepts) •
[Creating values](#creating-time-values) •
[Extracting fields](#extracting-time-fields) •
[Unix time](#unix-time) •
[Time comparison](#time-comparison) •
[Time arithmetic](#time-arithmetic) •
[Rounding](#rounding) •
[Formatting](#formatting) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Concepts

This extension works with two types of values: Time and Duration.

```text
  Time

  since     within
  0-time    second
┌─────────┬─────────────┐
│ seconds │ nanoseconds │
└─────────┴─────────────┘
  64 bit    32 bit
```

Time is a pair (seconds, nanoseconds), where `seconds` is the 64-bit number of seconds since zero time (0001-01-01 00:00:00 UTC) and `nanoseconds` is the number of nanoseconds within the current second (0-999999999).

For maximum flexibility, you can store time values in their internal representation (a 13-byte BLOB). This allows you to represent dates for billions of years in the past and future with nanosecond precision.

Alternatively, you can store time values as a NUMBER (64-bit integer) of seconds (milli-, micro- or nanoseconds) since the Unix epoch (1970-01-01 00:00:00 UTC). In this case, the range of representable dates depends on the unit of time used:

-   Seconds: billions of years into the past or future with second precision.
-   Milliseconds: 292 million years before or after 1970 with millisecond precision.
-   Microseconds: years from -290307 to 294246 with microsecond precision.
-   Nanoseconds: years from 1678 to 2262 with nanosecond precision.

Time is always stored and operated in UTC, but you can convert it from/to a specific timezone.

```text
  Duration
┌─────────────┐
│ nanoseconds │
└─────────────┘
  64 bit
```

Duration is a 64-bit number of nanoseconds, so it can represent values up to about 290 years. You can store duration values as NUMBER.

The calendrical calculations always assume a Gregorian calendar, with no leap seconds.

## Creating time values

There are two basic constructors — one for the current time and one for a specific date/time.

### time_now

```text
time_now()
```

Returns the current time in UTC.

```sql
select time_fmt_iso(time_now());
-- 2024-08-06T21:22:15.431295000Z
```

Aliased as Postgres-like `now()`.

### time_date

```text
time_date(year, month, day[, hour, min, sec[, nsec[, offset_sec]]])
```

Returns the Time corresponding to a given date/time. The time part (hour+minute+second), the nanosecond part, and the timezone offset part are all optional.

The `month`, `day`, `hour`, `min`, `sec`, and `nsec` values may be outside their usual ranges and will be normalized during the conversion. For example, October 32 converts to November 1.

If `offset_sec` is not 0, the source time is treated as being in a given timezone (with an offset in seconds east of UTC) and converted back to UTC.

```sql
select time_fmt_iso(time_date(2011, 11, 18));
-- 2011-11-18T00:00:00Z

select time_fmt_iso(time_date(2011, 11, 18, 15, 56, 35));
-- 2011-11-18T15:56:35Z

select time_fmt_iso(time_date(2011, 11, 18, 15, 56, 35, 666777888));
-- 2011-11-18T15:56:35.666777888Z

select time_fmt_iso(time_date(2011, 11, 18, 15, 56, 35, 0, 3*3600));
-- 2011-11-18T12:56:35Z

select time_fmt_iso(time_date(2011, 11, 18, 15, 56, 35, 666777888, 3*3600));
-- 2011-11-18T12:56:35.666777888Z
```

Aliased as Postgres-like `make_date` and `make_timestamp`:

```text
make_date(year, month, day)
make_timestamp(year, month, day, hour, min, sec)
```

## Extracting time fields

There are a number of functions for extracting different time fields, and a generic function for extracting time fields by name.

### time_get_year

```text
time_get_year(t)
```

Returns the year in which t occurs.

```sql
select time_get_year(time_now());
-- 2024
```

### time_get_month

```text
time_get_month(t)
```

Returns the month of the year specified by t (1-12).

```sql
select time_get_month(time_now());
-- 8
```

### time_get_day

```text
time_get_day(t)
```

Returns the day of the month specified by t (1-31).

```sql
select time_get_day(time_now());
-- 6
```

### time_get_hour

```text
time_get_hour(t)
```

Returns the hour within the day specified by t (0-23).

```sql
select time_get_hour(time_now());
-- 21
```

### time_get_minute

```text
time_get_minute(t)
```

Returns the minute offset within the hour specified by t.

```sql
select time_get_minute(time_now());
-- 22
```

### time_get_second

```text
time_get_second(t)
```

Returns the second offset within the minute specified by t (0-59).

```sql
select time_get_second(time_now());
-- 15
```

### time_get_nano

```text
time_get_nano(t)
```

Returns the nanosecond offset within the second specified by t (0-999999999).

```sql
select time_get_nano(time_now());
-- 431295000
```

### time_get_weekday

```text
time_get_weekday(t)
```

Returns the day of the week specified by t (0-6, Sunday = 0).

```sql
select time_get_weekday(time_now());
-- 2
```

### time_get_yearday

```text
time_get_yearday(t)
```

Returns the day of the year specified by t (1-366).

```sql
select time_get_yearday(time_now());
-- 219
```

### time_get_isoyear

```text
time_get_isoyear(t)
```

Returns the ISO 8601 year in which t occurs.

```sql
select time_get_isoyear(time_now());
-- 2024
```

### time_get_isoweek

```text
time_get_isoweek(t)
```

Returns the ISO 8601 week number of the year specified by t (1-53).

```sql
select time_get_isoweek(time_now());
-- 32
```

### time_get

```text
time_get(t, field)
```

Returns the value of a given field from a time value specified by t.

Supported fields:

```text
millennium    hour             isoyear
century       minute           isoweek
decade        second           isodow
year          milli[second]    yearday
quarter       micro[second]    weekday
month         nano[second]     epoch
day
```

```sql
select time_get(time_now(), 'millennium');
-- 2
select time_get(time_now(), 'century');
-- 20
select time_get(time_now(), 'decade');
-- 202

select time_get(time_now(), 'year');
-- 2024
select time_get(time_now(), 'quarter');
-- 3
select time_get(time_now(), 'month');
-- 8
select time_get(time_now(), 'day');
-- 6

select time_get(time_now(), 'hour');
-- 21
select time_get(time_now(), 'minute');
-- 22
select time_get(time_now(), 'second');
-- 15.431295

select time_get(time_now(), 'milli');
-- 431
select time_get(time_now(), 'micro');
-- 431295
select time_get(time_now(), 'nano');
-- 431295000

select time_get(time_now(), 'isoyear');
-- 2024
select time_get(time_now(), 'isoweek');
-- 32
select time_get(time_now(), 'isodow');
-- 2

select time_get(time_now(), 'yearday');
-- 219
select time_get(time_now(), 'weekday');
-- 2

select time_get(time_now(), 'epoch');
-- 1722979335.43129
```

Aliased as Postgres-like `date_part(field, t)`.

## Unix time

These are functions for converting time values to/from Unix time (time since the Unix epoch - January 1, 1970 UTC).

### time_unix

```text
time_unix(sec[, nsec])
```

Returns the Time corresponding to the given Unix time, `sec` seconds and `nsec` nanoseconds since January 1, 1970 UTC.

```sql
select time_fmt_iso(time_unix(1321631795));
-- 2011-11-18T15:56:35Z
select time_fmt_iso(time_unix(1321631795, 666777888));
-- 2011-11-18T15:56:35.666777888Z
```

Aliased as Postgres-like `to_timestamp(sec)`.

### time_milli

```text
time_milli(msec)
```

Returns the Time corresponding to the given Unix time, `msec` milliseconds since January 1, 1970 UTC.

```sql
select time_fmt_iso(time_milli(1321631795666));
-- 2011-11-18T15:56:35.666000000Z
```

### time_micro

```text
time_micro(usec)
```

Returns the Time corresponding to the given Unix time, `usec` microseconds since January 1, 1970 UTC.

```sql
select time_fmt_iso(time_micro(1321631795666777));
-- 2011-11-18T15:56:35.666777000Z
```

### time_nano

```text
time_nano(nsec)
```

Returns the Time corresponding to the given Unix time, `nsec` nanoseconds since January 1, 1970 UTC.

```sql
select time_fmt_iso(time_nano(1321631795666777888));
-- 2011-11-18T15:56:35.666777888Z
```

### time_to_unix

```text
time_to_unix(t)
```

Returns t as a Unix time, the number of seconds elapsed since January 1, 1970 UTC.

Unix-like operating systems often record time as a 32-bit number of seconds, but since `time_to_unix` returns a 64-bit value, it is valid for billions of years into the past or future.

```sql
select time_to_unix(time_now());
-- 1722979335
```

### time_to_milli

```text
time_to_milli(t)
```

Returns t as a Unix time, the number of milliseconds elapsed since January 1, 1970 UTC.

The result is undefined if the Unix time in milliseconds cannot be represented by a 64-bit integer (a date more than 292 million years before or after 1970).

```sql
select time_to_milli(time_now());
-- 1722979335431
```

### time_to_micro

```text
time_to_micro(t)
```

Returns t as a Unix time, the number of microseconds elapsed since January 1, 1970 UTC.

The result is undefined if the Unix time in microseconds cannot be represented by a 64-bit integer (a date before year -290307 or after year 294246).

```sql
select time_to_micro(time_now());
-- 1722979335431295
```

### time_to_nano

```text
time_to_nano(t)
```

Returns t as a Unix time, the number of nanoseconds elapsed since January 1, 1970 UTC.

The result is undefined if the Unix time in nanoseconds cannot be represented by a 64-bit integer (a date before the year 1678 or after 2262).

```sql
select time_to_nano(time_now());
-- 1722979335431295000
```

## Time comparison

These are functions for comparing time values.

### time_after

```text
time_after(t, u)
```

Reports whether the time instant t is after u.

```sql
select time_after(time_now(), time_date(2011, 11, 18));
-- 1
```

### time_before

```text
time_before(t, u)
```

Reports whether the time instant t is before u.

```sql
select time_before(time_now(), time_date(2011, 11, 18));
-- 0
```

### time_compare

```text
time_compare(t, u)
```

Compares the time instant t with u:

-   if t is before u, it returns -1;
-   if t is after u, it returns +1;
-   if they're the same, it returns 0.

```sql
select time_compare(time_now(), time_date(2011, 11, 18));
-- 1
select time_compare(time_date(2011, 11, 18), time_now());
-- -1
select time_compare(time_date(2011, 11, 18), time_date(2011, 11, 18));
-- 0
```

### time_equal

```text
time_equal(t, u)
```

Reports whether t and u represent the same time instant.

```sql
select time_equal(time_now(), time_date(2011, 11, 18));
-- 0
select time_equal(time_date(2011, 11, 18), time_date(2011, 11, 18));
-- 1
```

## Time arithmetic

These are functions for adding time and duration values, and functions for subtracting time values.

### time_add

```text
time_add(t, d)
```

Returns the time t plus the duration d. Use negative d to subtract duration.

You can use the following duration constants:

-   `dur_us()` - 1 microsecond;
-   `dur_ms()` - 1 millisecond;
-   `dur_s()` - 1 second;
-   `dur_m()` - 1 minute;
-   `dur_h()` - 1 hour.

```sql
select time_fmt_iso(time_add(time_now(), 24*dur_h()));
-- 2024-08-07T21:22:15.431295000Z

select time_fmt_iso(time_add(time_now(), 60*dur_m()));
-- 2024-08-06T22:22:15.431295000Z

select time_fmt_iso(time_add(time_now(), 5*dur_m()+30*dur_s()));
-- 2024-08-06T21:27:45.431295000Z
```

Do not use `time_add` to add days, months or years. Use `time_add_date` instead.

Aliased as Postgres-like `date_add(t, d)`.

### time_add_date

```text
time_add_date(t, years[, months[, days]])
```

Returns the time corresponding to adding the given number of years, months, and days to t.
For example, `time_add_date(-1, 2, 3)` applied to January 1, 2011 returns March 4, 2010.

Normalizes its result in the same way that `time_date` does, so, for example, adding one month to October 31 yields December 1, the normalized form for November 31.

Use negative values to subtract years, months, and days.

```sql
select time_fmt_date(time_add_date(time_date(2011, 11, 18), 5));
-- 2016-11-18
select time_fmt_date(time_add_date(time_date(2011, 11, 18), 3, 5));
-- 2015-04-18
select time_fmt_date(time_add_date(time_date(2011, 11, 18), 3, 5, -10));
-- 2015-04-08
```

### time_sub

```text
time_sub(t, u)
```

Returns the duration between two time values t and u (in nanoseconds). If the result exceeds the maximum (or minimum) value that can be stored in a Duration, the maximum (or minimum) duration will be returned.

```sql
select time_sub(time_date(2011, 11, 19), time_date(2011, 11, 18));
-- 86400000000000

select time_sub(
  time_date(2011, 11, 18, 16, 56, 35),
  time_date(2011, 11, 18, 15, 56, 35)
);
-- 3600000000000

select time_sub(time_unix(1321631795, 5000000), time_unix(1321631795, 0));
-- 5000000
```

Aliased as Postgres-like `age(t, u)`.

### time_since

```text
time_since(t)
```

Returns the time elapsed since t (in nanoseconds). It is shorthand for `time_sub(time_now(), t)`.

```sql
select time_since(time_now());
-- 5000
```

### time_until

```text
time_until(t)
```

Returns the duration until t (in nanoseconds). It is shorthand for `time_sub(t, time_now())`.

```sql
select time_until(time_date(2024, 9, 1));
-- 2144479530297000
```

## Rounding

These are functions for truncating and rounding the time.

### time_trunc

```text
time_trunc(t, field)
time_trunc(t, d)
```

Truncates t to a precision specified by field, or rounds down t to a multiple of the duration d.

Supported fields:

```text
millennium    hour
century       minute
decade        second
year          milli[second]
quarter       micro[second]
month
week
day
```

```sql
with t as (
    select time_date(2011, 11, 18, 15, 56, 35, 666777888) as v
)
select 'original = '   || time_fmt_iso(t.v) from t union all
select 'millennium = ' || time_fmt_iso(time_trunc(t.v, 'millennium')) from t union all
select 'century = '    || time_fmt_iso(time_trunc(t.v, 'century')) from t union all
select 'decade = '     || time_fmt_iso(time_trunc(t.v, 'decade')) from t union all
select 'year = '       || time_fmt_iso(time_trunc(t.v, 'year')) from t union all
select 'quarter = '    || time_fmt_iso(time_trunc(t.v, 'quarter')) from t union all
select 'month = '      || time_fmt_iso(time_trunc(t.v, 'month')) from t union all
select 'week = '       || time_fmt_iso(time_trunc(t.v, 'week')) from t union all
select 'day = '        || time_fmt_iso(time_trunc(t.v, 'day')) from t union all
select 'hour = '       || time_fmt_iso(time_trunc(t.v, 'hour')) from t union all
select 'minute = '     || time_fmt_iso(time_trunc(t.v, 'minute')) from t union all
select 'second = '     || time_fmt_iso(time_trunc(t.v, 'second')) from t union all
select 'milli = '      || time_fmt_iso(time_trunc(t.v, 'milli')) from t union all
select 'micro = '      || time_fmt_iso(time_trunc(t.v, 'micro')) from t;
```

```text
original   = 2011-11-18T15:56:35.666777888Z
millennium = 2000-01-01T00:00:00Z
century    = 2000-01-01T00:00:00Z
decade     = 2010-01-01T00:00:00Z
year       = 2011-01-01T00:00:00Z
quarter    = 2011-10-01T00:00:00Z
month      = 2011-11-01T00:00:00Z
week       = 2011-11-12T00:00:00Z
day        = 2011-11-18T00:00:00Z
hour       = 2011-11-18T15:00:00Z
minute     = 2011-11-18T15:56:00Z
second     = 2011-11-18T15:56:35Z
milli      = 2011-11-18T15:56:35.666000000Z
micro      = 2011-11-18T15:56:35.666777000Z
```

Supported durations: any duration that is a multiple of 1 second. If d <= 0, returns t unchanged.

```sql
with t as (
    select time_date(2011, 11, 18, 15, 56, 35, 666777888) as v
)
select 't = '   || time_fmt_iso(t.v) from t union all
select '12h = ' || time_fmt_iso(time_trunc(t.v, 12*dur_h())) from t union all
select '1h = '  || time_fmt_iso(time_trunc(t.v, dur_h())) from t union all
select '30m = ' || time_fmt_iso(time_trunc(t.v, 30*dur_m())) from t union all
select '1m = '  || time_fmt_iso(time_trunc(t.v, dur_m())) from t union all
select '30s = ' || time_fmt_iso(time_trunc(t.v, 30*dur_s())) from t union all
select '1s = '  || time_fmt_iso(time_trunc(t.v, dur_s())) from t;
```

```text
t   = 2011-11-18T15:56:35.666777888Z
12h = 2011-11-18T12:00:00Z
1h  = 2011-11-18T15:00:00Z
30m = 2011-11-18T15:30:00Z
1m  = 2011-11-18T15:56:00Z
30s = 2011-11-18T15:56:30Z
1s  = 2011-11-18T15:56:35Z
```

Aliased as Postgres-like `date_trunc(field, t)`.

### time_round

```text
time_round(t, d)
```

Rounds t to the nearest multiple of the duration d.

Supports any duration that is a multiple of 1 second. The rounding behavior for halfway values is to round up. If d <= 0, returns t unchanged.

```sql
with t as (
    select time_date(2011, 11, 18, 15, 56, 35, 666777888) as v
)
select 't = '   || time_fmt_iso(t.v) from t union all
select '12h = ' || time_fmt_iso(time_round(t.v, 12*dur_h())) from t union all
select '1h = '  || time_fmt_iso(time_round(t.v, dur_h())) from t union all
select '30m = ' || time_fmt_iso(time_round(t.v, 30*dur_m())) from t union all
select '1m = '  || time_fmt_iso(time_round(t.v, dur_m())) from t union all
select '30s = ' || time_fmt_iso(time_round(t.v, 30*dur_s())) from t union all
select '1s = '  || time_fmt_iso(time_round(t.v, dur_s())) from t;
```

```text
t   = 2011-11-18T15:56:35.666777888Z
12h = 2011-11-18T12:00:00Z
1h  = 2011-11-18T16:00:00Z
30m = 2011-11-18T16:00:00Z
1m  = 2011-11-18T15:57:00Z
30s = 2011-11-18T15:56:30Z
1s  = 2011-11-18T15:56:36Z
```

## Formatting

These are functions for formatting and parsing the time.

### time_fmt_iso

```text
time_fmt_iso(t[, offset_sec])
```

Returns an ISO 8601 time string for the given time value. Optionally converts the time value to the given timezone offset before formatting.

Chooses the most compact representation:

```text
2006-01-02T15:04:05.999999999+07:00
2006-01-02T15:04:05.999999999Z
2006-01-02T15:04:05+07:00
2006-01-02T15:04:05Z
```

```sql
select time_fmt_iso(time_date(2011, 11, 18, 15, 56, 35, 666777888), 3*3600);
-- 2011-11-18T18:56:35.666777888+03:00

select time_fmt_iso(time_date(2011, 11, 18, 15, 56, 35, 666777888));
-- 2011-11-18T15:56:35.666777888Z

select time_fmt_iso(time_date(2011, 11, 18, 15, 56, 35), 3*3600);
-- 2011-11-18T18:56:35+03:00

select time_fmt_iso(time_date(2011, 11, 18, 15, 56, 35));
-- 2011-11-18T15:56:35Z
```

### time_fmt_datetime

```text
time_fmt_datetime(t[, offset_sec])
```

Returns a datetime string (`2006-01-02 15:04:05`) for the given time value. Optionally converts the time value to the given timezone offset before formatting.

```sql
select time_fmt_datetime(time_date(2011, 11, 18, 15, 56, 35), 3*3600);
-- 2011-11-18 18:56:35

select time_fmt_datetime(time_date(2011, 11, 18, 15, 56, 35));
-- 2011-11-18 15:56:35

select time_fmt_datetime(time_date(2011, 11, 18));
-- 2011-11-18 00:00:00
```

### time_fmt_date

```text
time_fmt_date(t[, offset_sec])
```

Returns a date string (`2006-01-02`) for the given time value. Optionally converts the time value to the given timezone offset before formatting.

```sql
select time_fmt_date(time_date(2011, 11, 18, 15, 56, 35), 12*3600);
-- 2011-11-19

select time_fmt_date(time_date(2011, 11, 18, 15, 56, 35));
-- 2011-11-18

select time_fmt_date(time_date(2011, 11, 18));
-- 2011-11-18
```

### time_fmt_time

```text
time_fmt_time(t[, offset_sec])
```

Returns a time string (`15:04:05`) for the given time value. Optionally converts the time value to the given timezone offset before formatting.

```sql
select time_fmt_time(time_date(2011, 11, 18, 15, 56, 35), 3*3600);
-- 18:56:35

select time_fmt_time(time_date(2011, 11, 18, 15, 56, 35));
-- 15:56:35

select time_fmt_time(time_date(2011, 11, 18));
-- 00:00:00
```

### time_parse

```text
time_parse(s)
```

Parses a formatted string and returns the time value it represents.

Supports a limited set of layouts:

```text
2006-01-02T15:04:05.999999999+07:00     ISO 8601 with nanoseconds and timezone
2006-01-02T15:04:05.999999999Z          ISO 8601 with nanoseconds, UTC
2006-01-02T15:04:05+07:00               ISO 8601 with timezone
2006-01-02T15:04:05Z                    ISO 8601, UTC
2006-01-02 15:04:05                     Date and time, UTC
2006-01-02                              Date only, UTC
15:04:05                                Time only, UTC
```

```sql
select time_parse('2011-11-18T15:56:35.666777888Z')      = time_unix(1321631795, 666777888);
select time_parse('2011-11-18T19:26:35.666777888+03:30') = time_unix(1321631795, 666777888);
select time_parse('2011-11-18T12:26:35.666777888-03:30') = time_unix(1321631795, 666777888);
select time_parse('2011-11-18T15:56:35Z')                = time_unix(1321631795, 0);
select time_parse('2011-11-18T19:26:35+03:30')           = time_unix(1321631795, 0);
select time_parse('2011-11-18T12:26:35-03:30')           = time_unix(1321631795, 0);
select time_parse('2011-11-18 15:56:35')                 = time_unix(1321631795, 0);
select time_parse('2011-11-18')                          = time_date(2011, 11, 18);
select time_parse('15:56:35')                            = time_date(1, 1, 1, 15, 56, 35);
```

## Duration constants

These functions return durations in nanoseconds:

-   `dur_ns()` = 1 nanosecond;
-   `dur_us()` = 1 microsecond = 10³ ns;
-   `dur_ms()` = 1 millisecond = 10⁶ ns;
-   `dur_s()` = 1 second = 10⁹ ns;
-   `dur_m()` = 1 minute = 60\*10⁹ ns;
-   `dur_h()` = 1 hour = 3600\*10⁹ ns.

```sql
select dur_ns();
-- 1
select dur_us();
-- 1000
select dur_ms();
-- 1000000
select dur_s();
-- 1000000000
select dur_m();
-- 60000000000
select dur_h();
-- 3600000000000
```

## Acknowledgements

While this extension is implemented in C, its design and implementation is largely based on Go's stdlib [time](https://github.com/golang/go/tree/master/src/time) package (BSD 3-Clause License), which I think is awesome (except for the formatting part).

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./time
sqlite> select time_now();
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
