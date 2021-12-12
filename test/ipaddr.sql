-- Copyright (c) 2021 Vincent Bernat, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/ipaddr
select '00', ipfamily('192.168.16.12') = 4;
select '01', ipfamily('192.168.16.12/24') = 4;
select '02', ipfamily('2001:db8::1') = 6;
select '03', ipfamily('2001:db8::1/64') = 6;
select '04', iphost('192.168.160.120') = '192.168.160.120';
select '05', iphost('192.168.16.12/24') = '192.168.16.12';
select '06', iphost('2001:db8::1/64') = '2001:db8::1';
select '07', iphost('2001:db8::1') = '2001:db8::1';
select '08', ipmasklen('192.168.16.12') = 32;
select '09', ipmasklen('192.168.16.12/24') = 24;
select '10', ipmasklen('2001:db8::1/64') = 64;
select '11', ipmasklen('2001:db8::1') = 128;
select '12', ipnetwork('192.168.160.120/24') = '192.168.160.0/24';
select '13', ipnetwork('192.168.160.128/26') = '192.168.160.128/26';
select '14', ipnetwork('192.168.160.120') = '192.168.160.120/32';
select '15', ipnetwork('2001:db8::1/64') = '2001:db8::/64';
select '16', ipnetwork('2001:db8::1') = '2001:db8::1/128';
select '17', ipnetwork('2001:db8:1::1/48') = '2001:db8:1::/48';
select '18', ipnetwork('2001:db8:1::1/47') = '2001:db8::/47';
select '19', ipcontains('192.168.16.0/24', '192.168.16.3') = 1;
select '20', ipcontains('192.168.15.0/24', '192.168.16.3') = 0;
select '21', ipcontains('2001:db8::/64', '2001:db8::17') = 1;
select '22', ipcontains('2001:db8:1::/64', '2001:db8::17') = 0;
select '23', ipcontains('192.168.16.0/24', '192.168.16.0/26') = 1;
select '24', ipcontains('192.168.16.0/27', '192.168.16.0/26') = 0;
select '25', ipcontains('192.168.16.0/25', '192.168.16.128/26') = 0;
select '26', ipcontains('2001:db8::/48', '2001:db8::/64') = 1;
select '27', ipcontains('2001:db8::/56', '2001:db8::/48') = 0;
select '28', ipcontains('2001:db8::/56', '2001:db8:1::/64') = 0;
