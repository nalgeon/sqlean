-- Copyright (c) 2021 Vincent Bernat, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/ipaddr

-- ip_family
select '01_01', ip_family('192.168.16.12') = 4;
select '01_02', ip_family('192.168.16.12/24') = 4;
select '01_03', ip_family('2001:db8::1') = 6;
select '01_04', ip_family('2001:db8::1/64') = 6;
select '01_05', ip_family('0.0.0.0') = 4;
select '01_06', ip_family('255.255.255.255') = 4;
select '01_07', ip_family('::') = 6;
select '01_08', ip_family('::1') = 6;
-- an IPv4-mapped IPv6 address is IPv6
select '01_09', ip_family('::ffff:192.168.16.12') = 6;
select '01_10', typeof(ip_family('192.168.16.12')) = 'integer';

-- ip_host
-- the address without the prefix length, not the host bits
select '02_01', ip_host('192.168.160.120') = '192.168.160.120';
select '02_02', ip_host('192.168.16.12/24') = '192.168.16.12';
select '02_03', ip_host('192.168.16.12/0') = '192.168.16.12';
select '02_04', ip_host('2001:db8::1') = '2001:db8::1';
select '02_05', ip_host('2001:db8::1/64') = '2001:db8::1';
select '02_06', ip_host('0.0.0.0') = '0.0.0.0';
select '02_07', ip_host('255.255.255.255') = '255.255.255.255';
select '02_08', ip_host('::') = '::';
select '02_09', ip_host('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff';
select '02_10', typeof(ip_host('192.168.16.12')) = 'text';

-- ip_masklen
select '03_01', ip_masklen('192.168.16.12') = 32;
select '03_02', ip_masklen('192.168.16.12/24') = 24;
select '03_03', ip_masklen('192.168.16.12/0') = 0;
select '03_04', ip_masklen('192.168.16.12/32') = 32;
select '03_05', ip_masklen('2001:db8::1') = 128;
select '03_06', ip_masklen('2001:db8::1/64') = 64;
select '03_07', ip_masklen('2001:db8::1/0') = 0;
select '03_08', ip_masklen('2001:db8::1/128') = 128;
-- the prefix length is not checked against the host bits
select '03_09', ip_masklen('192.168.16.12/8') = 8;
select '03_10', typeof(ip_masklen('192.168.16.12')) = 'integer';

-- ip_network
select '11_01', ip_network('192.168.160.120/24') = '192.168.160.0/24';
select '11_02', ip_network('192.168.160.128/26') = '192.168.160.128/26';
select '11_03', ip_network('192.168.160.120') = '192.168.160.120/32';
select '11_04', ip_network('2001:db8::1/64') = '2001:db8::/64';
select '11_05', ip_network('2001:db8::1') = '2001:db8::1/128';
select '11_06', ip_network('2001:db8:1::1/48') = '2001:db8:1::/48';
select '11_07', ip_network('2001:db8:1::1/47') = '2001:db8::/47';
select '11_08', ip_network('::ffff:192.168.16.12/120') = '::ffff:192.168.16.0/120';
select '11_09', typeof(ip_network('192.168.16.12')) = 'text';

-- ip_network: IPv4 prefix lengths, from none to all bits
select '12_01', ip_network('255.255.255.255/0') = '0.0.0.0/0';
select '12_02', ip_network('255.255.255.255/1') = '128.0.0.0/1';
select '12_03', ip_network('255.255.255.255/7') = '254.0.0.0/7';
select '12_04', ip_network('255.255.255.255/8') = '255.0.0.0/8';
select '12_05', ip_network('255.255.255.255/9') = '255.128.0.0/9';
select '12_06', ip_network('255.255.255.255/16') = '255.255.0.0/16';
select '12_07', ip_network('255.255.255.255/23') = '255.255.254.0/23';
select '12_08', ip_network('255.255.255.255/24') = '255.255.255.0/24';
select '12_09', ip_network('255.255.255.255/25') = '255.255.255.128/25';
select '12_10', ip_network('255.255.255.255/31') = '255.255.255.254/31';
select '12_11', ip_network('255.255.255.255/32') = '255.255.255.255/32';

-- ip_network: IPv6 prefix lengths
-- clears all host bits, including the boundary byte for byte-aligned prefixes
select '13_01', ip_network('2001:db8:1111:2222:3333:4444:5555:6666/64') = '2001:db8:1111:2222::/64';
select '13_02', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/0') = '::/0';
select '13_03', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/1') = '8000::/1';
select '13_04', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/8') = 'ff00::/8';
select '13_05', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/120') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff00/120';
select '13_06', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/121') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff80/121';
select '13_07', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/122') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffc0/122';
select '13_08', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/123') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffe0/123';
select '13_09', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/124') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:fff0/124';
select '13_10', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/125') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:fff8/125';
select '13_11', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/126') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffc/126';
select '13_12', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/127') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe/127';
select '13_13', ip_network('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128') = 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128';

-- ip_contains
select '21_01', ip_contains('192.168.16.0/24', '192.168.16.3') = 1;
select '21_02', ip_contains('192.168.15.0/24', '192.168.16.3') = 0;
select '21_03', ip_contains('2001:db8::/64', '2001:db8::17') = 1;
select '21_04', ip_contains('2001:db8:1::/64', '2001:db8::17') = 0;
select '21_05', ip_contains('192.168.16.0/24', '192.168.16.0/26') = 1;
select '21_06', ip_contains('192.168.16.0/27', '192.168.16.0/26') = 0;
select '21_07', ip_contains('192.168.16.0/25', '192.168.16.128/26') = 0;
select '21_08', ip_contains('2001:db8::/48', '2001:db8::/64') = 1;
select '21_09', ip_contains('2001:db8::/56', '2001:db8::/48') = 0;
select '21_10', ip_contains('2001:db8::/56', '2001:db8:1::/64') = 0;
-- a subnet contains itself, an address without a prefix is a /32 or /128
select '21_11', ip_contains('192.168.16.0/24', '192.168.16.0/24') = 1;
select '21_12', ip_contains('192.168.16.3', '192.168.16.3') = 1;
select '21_13', ip_contains('192.168.16.3', '192.168.16.4') = 0;
select '21_14', ip_contains('2001:db8::1', '2001:db8::1') = 1;
-- /0 contains every address of its family
select '21_15', ip_contains('0.0.0.0/0', '255.255.255.255') = 1;
select '21_16', ip_contains('::/0', 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff') = 1;
-- the host bits of the subnet are ignored
select '21_17', ip_contains('192.168.16.77/24', '192.168.16.3') = 1;
-- different families never contain each other, not even IPv4-mapped addresses
select '21_18', ip_contains('::/0', '192.168.16.3') = 0;
select '21_19', ip_contains('0.0.0.0/0', '::1') = 0;
select '21_20', ip_contains('::ffff:0:0/96', '192.168.16.3') = 0;
select '21_21', typeof(ip_contains('192.168.16.0/24', '192.168.16.3')) = 'integer';

-- ip_contains: the subnet's mask applies to both operands
select '22_01', ip_contains('2001:db8::/64', '2001:db8:0:0:ff00::1') = 1;
select '22_02', ip_contains('2001:db8::/127', '2001:db8::1') = 1;
select '22_03', ip_contains('2001:db8::/127', '2001:db8::2') = 0;
select '22_04', ip_contains('2001:db8::1/127', '2001:db8::') = 1;
select '22_05', ip_contains('2001:db8::1/128', '2001:db8::') = 0;
select '22_06', ip_contains('2001:db8::/64', '2001:db8:0:0:ff00::/72') = 1;
select '22_07', ip_contains('2001:db8::/72', '2001:db8::/64') = 0;
select '22_08', ip_contains('192.168.16.0/31', '192.168.16.1') = 1;
select '22_09', ip_contains('192.168.16.0/31', '192.168.16.2') = 0;
select '22_10', ip_contains('192.168.16.0/23', '192.168.17.255') = 1;
select '22_11', ip_contains('192.168.16.0/23', '192.168.18.0') = 0;

-- formatting: lowercase, with the longest run of zero groups compressed (RFC 5952)
select '31_01', ip_host('2001:DB8::ABCD') = '2001:db8::abcd';
select '31_02', ip_host('2001:0db8:0000:0000:0000:0000:0000:0001') = '2001:db8::1';
select '31_03', ip_host('0:0:0:0:0:0:0:0') = '::';
select '31_04', ip_host('0:0:0:0:0:0:0:1') = '::1';
select '31_05', ip_host('2001:0:0:1:0:0:0:1') = '2001:0:0:1::1';
select '31_06', ip_host('2001:db8:0:0:1:0:0:1') = '2001:db8::1:0:0:1';
-- a single zero group is not compressed
select '31_07', ip_host('2001:db8:0:1:1:1:1:1') = '2001:db8:0:1:1:1:1:1';
-- IPv4-mapped addresses use the dotted form for the last 32 bits
select '31_08', ip_host('::ffff:192.168.16.12') = '::ffff:192.168.16.12';
select '31_09', ip_host('::ffff:c0a8:100c') = '::ffff:192.168.16.12';
-- other addresses with an IPv4 part, such as the deprecated IPv4-compatible ones, are in hex
select '31_10', ip_host('::192.168.16.12') = '::c0a8:100c';
select '31_11', ip_host('::c0a8:100c') = '::c0a8:100c';
select '31_12', ip_host('1:2::ffff:192.168.140.255') = '1:2::ffff:c0a8:8cff';
-- a zero group at the end is not compressed when it is the only one
select '31_13', ip_host('fd7a:115c:a1e0:ab12:4843:cd96:626b::') = 'fd7a:115c:a1e0:ab12:4843:cd96:626b:0';

-- parsing: the prefix length
select '32_01', ip_masklen('192.168.16.12/33') is null;
select '32_02', ip_masklen('2001:db8::1/129') is null;
select '32_03', ip_masklen('192.168.16.12/99999999999999999999') is null;
select '32_04', ip_masklen('192.168.16.12/-1') is null;
select '32_05', ip_masklen('192.168.16.12/') is null;
select '32_06', ip_masklen('192.168.16.12/x') is null;
select '32_07', ip_masklen('192.168.16.12/24x') is null;
select '32_08', ip_masklen('192.168.16.12/24 ') is null;
select '32_09', ip_masklen('192.168.16.12/24/8') is null;
select '32_10', ip_masklen('192.168.16.12//24') is null;
select '32_11', ip_masklen('/24') is null;
-- only canonical decimal: no sign, whitespace or leading zeros
select '32_12', ip_masklen('192.168.16.12/+24') is null;
select '32_13', ip_masklen('192.168.16.12/-0') is null;
select '32_14', ip_masklen('192.168.16.12/ 24') is null;
select '32_15', ip_masklen('192.168.16.12/024') is null;
select '32_16', ip_masklen('192.168.16.12/00') is null;
select '32_17', ip_masklen('2001:db8::/0128') is null;

-- parsing: the address
select '33_01', ip_family('192.168.16') is null;
select '33_02', ip_family('192.168.16.12.1') is null;
select '33_03', ip_family('192.168.16.256') is null;
select '33_04', ip_family('192.168.16.-1') is null;
select '33_05', ip_family('0x7f.0.0.1') is null;
select '33_06', ip_family('192.168.16.12 ') is null;
select '33_07', ip_family(' 192.168.16.12') is null;
select '33_08', ip_family('1:2:3:4:5:6:7:8:9') is null;
select '33_09', ip_family('1::2::3') is null;
select '33_10', ip_family(':::1') is null;
select '33_11', ip_family('2001:db8::g') is null;
select '33_12', ip_family('2001:db8::12345') is null;
select '33_13', ip_family('localhost') is null;
select '33_14', ip_family('') is null;
select '33_15', ip_family(printf('%.*c', 10000, '1')) is null;
-- an embedded NUL is not valid
select '33_16', ip_family('192.168.16.12' || char(0) || 'x') is null;
select '33_17', ip_masklen('192.168.16.12/24' || char(0)) is null;
-- octets with leading zeros are not valid (some parsers read them as octal)
select '33_18', ip_family('01.2.3.4') is null;
select '33_19', ip_family('1.2.3.04') is null;
select '33_20', ip_family('::ffff:1.2.03.4') is null;
select '33_21', ip_family('0.0.0.0') = 4;
-- IPv6 zones are not valid
select '33_22', ip_family('fe80::1%eth0') is null;
select '33_23', ip_family('fe80::1%1') is null;
select '33_24', ip_family('fe80::1%eth0/64') is null;

-- null propagation
select '41_01', ip_family(null) is null;
select '41_02', ip_host(null) is null;
select '41_03', ip_masklen(null) is null;
select '41_04', ip_network(null) is null;
select '41_05', ip_contains(null, '192.168.16.3') is null;
select '41_06', ip_contains('192.168.16.0/24', null) is null;
select '41_07', ip_contains(null, null) is null;
select '41_08', ip_contains(null, 'not an ip') is null;
select '41_09', ip_contains('not an ip', null) is null;

-- bad data is null
select '42_01', ip_family('not an ip') is null;
select '42_02', ip_host('not an ip') is null;
select '42_03', ip_masklen('not an ip') is null;
select '42_04', ip_network('not an ip') is null;
select '42_05', ip_contains('not an ip', '192.168.16.3') is null;
select '42_06', ip_contains('192.168.16.0/24', 'not an ip') is null;
-- numbers are converted to text, which is not an address
select '42_07', ip_family(3232239628) is null;
select '42_08', ip_host(1) is null;
select '42_09', ip_masklen(1.5) is null;
-- blobs are not text
select '42_10', ip_family(cast('192.168.16.12' as blob)) is null;
select '42_11', ip_host(cast('192.168.16.12/24' as blob)) is null;
select '42_12', ip_contains('192.168.16.0/24', cast('192.168.16.3' as blob)) is null;

-- querying a table
create table hosts(addr text);
insert into hosts values
    ('192.168.16.3'), ('192.168.16.200'), ('192.168.17.1/24'),
    ('2001:db8::1'), ('2001:db8:1::1/64'), ('not an ip'), (null);
select '51_01', count(*) = 3 from hosts where ip_contains('192.168.16.0/23', addr);
select '51_02', count(*) = 2 from hosts where ip_family(addr) = 6;
select '51_03', count(*) = 2 from hosts where ip_family(addr) is null;
select '51_04', group_concat(net, ',') = '192.168.16.0/24,192.168.17.0/24' from (
    select distinct ip_network(ip_host(addr) || '/24') as net from hosts
    where ip_family(addr) = 4 order by net
);
create index hosts_family on hosts(ip_family(addr));
select '51_05', count(*) = 3 from hosts where ip_family(addr) = 4;
drop table hosts;

-- aliases
select '61_01', ipfamily('192.168.16.12') = 4;
select '61_02', iphost('192.168.16.12/24') = '192.168.16.12';
select '61_03', ipmasklen('192.168.16.12/24') = 24;
select '61_04', ipnetwork('192.168.16.12/24') = '192.168.16.0/24';
select '61_05', ipcontains('192.168.16.0/24', '192.168.16.3') = 1;
