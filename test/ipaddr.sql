-- Copyright (c) 2021 Vincent Bernat, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/ipaddr

-- ipfamily
select '01_01', ipfamily('192.168.16.12') = 4;
select '01_02', ipfamily('192.168.16.12/24') = 4;
select '01_03', ipfamily('2001:db8::1') = 6;
select '01_04', ipfamily('2001:db8::1/64') = 6;
select '01_05', ipfamily('0.0.0.0') = 4;
select '01_06', ipfamily('255.255.255.255') = 4;
select '01_07', ipfamily('::') = 6;
select '01_08', ipfamily('::1') = 6;
-- an IPv4-mapped IPv6 address is IPv6
select '01_09', ipfamily('::ffff:192.168.16.12') = 6;
select '01_10', typeof(ipfamily('192.168.16.12')) = 'integer';

-- iphost
-- the address without the prefix length, not the host bits
select '02_01', iphost('192.168.160.120') = '192.168.160.120';
select '02_02', iphost('192.168.16.12/24') = '192.168.16.12';
select '02_03', iphost('192.168.16.12/0') = '192.168.16.12';
select '02_04', iphost('2001:db8::1') = '2001:db8::1';
select '02_05', iphost('2001:db8::1/64') = '2001:db8::1';
select '02_06', iphost('0.0.0.0') = '0.0.0.0';
select '02_07', iphost('255.255.255.255') = '255.255.255.255';
select '02_08', iphost('::') = '::';
select '02_09', iphost('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff';
select '02_10', typeof(iphost('192.168.16.12')) = 'text';

-- ipmasklen
select '03_01', ipmasklen('192.168.16.12') = 32;
select '03_02', ipmasklen('192.168.16.12/24') = 24;
select '03_03', ipmasklen('192.168.16.12/0') = 0;
select '03_04', ipmasklen('192.168.16.12/32') = 32;
select '03_05', ipmasklen('2001:db8::1') = 128;
select '03_06', ipmasklen('2001:db8::1/64') = 64;
select '03_07', ipmasklen('2001:db8::1/0') = 0;
select '03_08', ipmasklen('2001:db8::1/128') = 128;
-- the prefix length is not checked against the host bits
select '03_09', ipmasklen('192.168.16.12/8') = 8;
select '03_10', typeof(ipmasklen('192.168.16.12')) = 'integer';

-- ipnetwork
select '11_01', ipnetwork('192.168.160.120/24') = '192.168.160.0/24';
select '11_02', ipnetwork('192.168.160.128/26') = '192.168.160.128/26';
select '11_03', ipnetwork('192.168.160.120') = '192.168.160.120/32';
select '11_04', ipnetwork('2001:db8::1/64') = '2001:db8::/64';
select '11_05', ipnetwork('2001:db8::1') = '2001:db8::1/128';
select '11_06', ipnetwork('2001:db8:1::1/48') = '2001:db8:1::/48';
select '11_07', ipnetwork('2001:db8:1::1/47') = '2001:db8::/47';
select '11_08', ipnetwork('::ffff:192.168.16.12/120') = '::ffff:192.168.16.0/120';
select '11_09', typeof(ipnetwork('192.168.16.12')) = 'text';

-- ipnetwork: IPv4 prefix lengths, from none to all bits
select '12_01', ipnetwork('255.255.255.255/0') = '0.0.0.0/0';
select '12_02', ipnetwork('255.255.255.255/1') = '128.0.0.0/1';
select '12_03', ipnetwork('255.255.255.255/7') = '254.0.0.0/7';
select '12_04', ipnetwork('255.255.255.255/8') = '255.0.0.0/8';
select '12_05', ipnetwork('255.255.255.255/9') = '255.128.0.0/9';
select '12_06', ipnetwork('255.255.255.255/16') = '255.255.0.0/16';
select '12_07', ipnetwork('255.255.255.255/23') = '255.255.254.0/23';
select '12_08', ipnetwork('255.255.255.255/24') = '255.255.255.0/24';
select '12_09', ipnetwork('255.255.255.255/25') = '255.255.255.128/25';
select '12_10', ipnetwork('255.255.255.255/31') = '255.255.255.254/31';
select '12_11', ipnetwork('255.255.255.255/32') = '255.255.255.255/32';

-- ipnetwork: IPv6 prefix lengths
-- clears all host bits, including the boundary byte for byte-aligned prefixes
select '13_01', ipnetwork('2001:db8:1111:2222:3333:4444:5555:6666/64') = '2001:db8:1111:2222::/64';
select '13_02', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/0') = '::/0';
select '13_03', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/1') = '8000::/1';
select '13_04', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/8') = 'ff00::/8';
select '13_05', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/120') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff00/120';
select '13_06', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/121') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff80/121';
select '13_07', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/122') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffc0/122';
select '13_08', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/123') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffe0/123';
select '13_09', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/124') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:fff0/124';
select '13_10', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/125') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:fff8/125';
select '13_11', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/126') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffc/126';
select '13_12', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/127') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe/127';
select '13_13', ipnetwork('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128';

-- ipcontains
select '21_01', ipcontains('192.168.16.0/24', '192.168.16.3') = 1;
select '21_02', ipcontains('192.168.15.0/24', '192.168.16.3') = 0;
select '21_03', ipcontains('2001:db8::/64', '2001:db8::17') = 1;
select '21_04', ipcontains('2001:db8:1::/64', '2001:db8::17') = 0;
select '21_05', ipcontains('192.168.16.0/24', '192.168.16.0/26') = 1;
select '21_06', ipcontains('192.168.16.0/27', '192.168.16.0/26') = 0;
select '21_07', ipcontains('192.168.16.0/25', '192.168.16.128/26') = 0;
select '21_08', ipcontains('2001:db8::/48', '2001:db8::/64') = 1;
select '21_09', ipcontains('2001:db8::/56', '2001:db8::/48') = 0;
select '21_10', ipcontains('2001:db8::/56', '2001:db8:1::/64') = 0;
-- a subnet contains itself, an address without a prefix is a /32 or /128
select '21_11', ipcontains('192.168.16.0/24', '192.168.16.0/24') = 1;
select '21_12', ipcontains('192.168.16.3', '192.168.16.3') = 1;
select '21_13', ipcontains('192.168.16.3', '192.168.16.4') = 0;
select '21_14', ipcontains('2001:db8::1', '2001:db8::1') = 1;
-- /0 contains every address of its family
select '21_15', ipcontains('0.0.0.0/0', '255.255.255.255') = 1;
select '21_16', ipcontains('::/0', 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff') = 1;
-- the host bits of the subnet are ignored
select '21_17', ipcontains('192.168.16.77/24', '192.168.16.3') = 1;
-- different families never contain each other, not even IPv4-mapped addresses
select '21_18', ipcontains('::/0', '192.168.16.3') = 0;
select '21_19', ipcontains('0.0.0.0/0', '::1') = 0;
select '21_20', ipcontains('::ffff:0:0/96', '192.168.16.3') = 0;
select '21_21', typeof(ipcontains('192.168.16.0/24', '192.168.16.3')) = 'integer';

-- ipcontains: the subnet's mask applies to both operands
select '22_01', ipcontains('2001:db8::/64', '2001:db8:0:0:ff00::1') = 1;
select '22_02', ipcontains('2001:db8::/127', '2001:db8::1') = 1;
select '22_03', ipcontains('2001:db8::/127', '2001:db8::2') = 0;
select '22_04', ipcontains('2001:db8::1/127', '2001:db8::') = 1;
select '22_05', ipcontains('2001:db8::1/128', '2001:db8::') = 0;
select '22_06', ipcontains('2001:db8::/64', '2001:db8:0:0:ff00::/72') = 1;
select '22_07', ipcontains('2001:db8::/72', '2001:db8::/64') = 0;
select '22_08', ipcontains('192.168.16.0/31', '192.168.16.1') = 1;
select '22_09', ipcontains('192.168.16.0/31', '192.168.16.2') = 0;
select '22_10', ipcontains('192.168.16.0/23', '192.168.17.255') = 1;
select '22_11', ipcontains('192.168.16.0/23', '192.168.18.0') = 0;

-- formatting: lowercase, with the longest run of zero groups compressed (RFC 5952)
select '31_01', iphost('2001:DB8::ABCD') = '2001:db8::abcd';
select '31_02', iphost('2001:0db8:0000:0000:0000:0000:0000:0001') = '2001:db8::1';
select '31_03', iphost('0:0:0:0:0:0:0:0') = '::';
select '31_04', iphost('0:0:0:0:0:0:0:1') = '::1';
select '31_05', iphost('2001:0:0:1:0:0:0:1') = '2001:0:0:1::1';
select '31_06', iphost('2001:db8:0:0:1:0:0:1') = '2001:db8::1:0:0:1';
-- a single zero group is not compressed
select '31_07', iphost('2001:db8:0:1:1:1:1:1') = '2001:db8:0:1:1:1:1:1';
-- IPv4-mapped addresses use the dotted form for the last 32 bits
select '31_08', iphost('::ffff:192.168.16.12') = '::ffff:192.168.16.12';
select '31_09', iphost('::ffff:c0a8:100c') = '::ffff:192.168.16.12';
-- so do the deprecated IPv4-compatible addresses (current behavior)
select '31_10', iphost('::192.168.16.12') = '::192.168.16.12';
select '31_11', iphost('::c0a8:100c') = '::192.168.16.12';

-- parsing: the prefix length
select '32_01', ipmasklen('192.168.16.12/33') is null;
select '32_02', ipmasklen('2001:db8::1/129') is null;
select '32_03', ipmasklen('192.168.16.12/99999999999999999999') is null;
select '32_04', ipmasklen('192.168.16.12/-1') is null;
select '32_05', ipmasklen('192.168.16.12/') is null;
select '32_06', ipmasklen('192.168.16.12/x') is null;
select '32_07', ipmasklen('192.168.16.12/24x') is null;
select '32_08', ipmasklen('192.168.16.12/24 ') is null;
select '32_09', ipmasklen('192.168.16.12/24/8') is null;
select '32_10', ipmasklen('192.168.16.12//24') is null;
select '32_11', ipmasklen('/24') is null;
-- a sign, leading whitespace and leading zeros are accepted (current behavior)
select '32_12', ipmasklen('192.168.16.12/+24') = 24;
select '32_13', ipmasklen('192.168.16.12/-0') = 0;
select '32_14', ipmasklen('192.168.16.12/ 24') = 24;
select '32_15', ipmasklen('192.168.16.12/024') = 24;

-- parsing: the address
select '33_01', ipfamily('192.168.16') is null;
select '33_02', ipfamily('192.168.16.12.1') is null;
select '33_03', ipfamily('192.168.16.256') is null;
select '33_04', ipfamily('192.168.16.-1') is null;
select '33_05', ipfamily('0x7f.0.0.1') is null;
select '33_06', ipfamily('192.168.16.12 ') is null;
select '33_07', ipfamily(' 192.168.16.12') is null;
select '33_08', ipfamily('1:2:3:4:5:6:7:8:9') is null;
select '33_09', ipfamily('1::2::3') is null;
select '33_10', ipfamily(':::1') is null;
select '33_11', ipfamily('2001:db8::g') is null;
select '33_12', ipfamily('2001:db8::12345') is null;
select '33_13', ipfamily('localhost') is null;
select '33_14', ipfamily('') is null;
select '33_15', ipfamily(printf('%.*c', 10000, '1')) is null;
-- an embedded NUL ends the address (current behavior)
select '33_16', ipfamily('192.168.16.12' || char(0) || 'x') = 4;

-- null propagation
select '41_01', ipfamily(null) is null;
select '41_02', iphost(null) is null;
select '41_03', ipmasklen(null) is null;
select '41_04', ipnetwork(null) is null;
select '41_05', ipcontains(null, '192.168.16.3') is null;
select '41_06', ipcontains('192.168.16.0/24', null) is null;
select '41_07', ipcontains(null, null) is null;
select '41_08', ipcontains(null, 'not an ip') is null;
select '41_09', ipcontains('not an ip', null) is null;

-- bad data is null
select '42_01', ipfamily('not an ip') is null;
select '42_02', iphost('not an ip') is null;
select '42_03', ipmasklen('not an ip') is null;
select '42_04', ipnetwork('not an ip') is null;
select '42_05', ipcontains('not an ip', '192.168.16.3') is null;
select '42_06', ipcontains('192.168.16.0/24', 'not an ip') is null;
-- numbers are converted to text, which is not an address
select '42_07', ipfamily(3232239628) is null;
select '42_08', iphost(1) is null;
select '42_09', ipmasklen(1.5) is null;
-- blobs are read as text (current behavior)
select '42_10', ipfamily(cast('192.168.16.12' as blob)) = 4;
select '42_11', iphost(cast('192.168.16.12/24' as blob)) = '192.168.16.12';

-- querying a table
create table hosts(addr text);
insert into hosts values
    ('192.168.16.3'), ('192.168.16.200'), ('192.168.17.1/24'),
    ('2001:db8::1'), ('2001:db8:1::1/64'), ('not an ip'), (null);
select '51_01', count(*) = 3 from hosts where ipcontains('192.168.16.0/23', addr);
select '51_02', count(*) = 2 from hosts where ipfamily(addr) = 6;
select '51_03', count(*) = 2 from hosts where ipfamily(addr) is null;
select '51_04', group_concat(net, ',') = '192.168.16.0/24,192.168.17.0/24' from (
    select distinct ipnetwork(iphost(addr) || '/24') as net from hosts
    where ipfamily(addr) = 4 order by net
);
create index hosts_family on hosts(ipfamily(addr));
select '51_05', count(*) = 3 from hosts where ipfamily(addr) = 4;
drop table hosts;
