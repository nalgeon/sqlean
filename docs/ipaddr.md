# ipaddr: IP address manipulation

Functions to manipulate IPs and subnets.

⚠️ This extension is not available on Windows.

### `ipfamily(ip)`

Returns the family of a specified IP address.

```
sqlite> select ipfamily('192.168.1.1');
4
```

### `iphost(ip)`

Returns the host part of an IP address.

```
sqlite> select iphost('2001:db8::123/64');
2001:db8::123
```

### `ipmasklen(ip)`

Returns the prefix length of an IP address.

```
sqlite> select ipmasklen('192.168.16.12/24');
24
```

### `ipnetwork(ip)`

Returns the network part of an IP address.

```
sqlite> select ipnetwork('192.168.16.12/24');
192.168.16.0/24
```

### `ipcontains(subnet, ip)`

Returns `1` if `subnet` contains `ip` (which can be another subnet).
`0` otherwise.

```
sqlite> select ipcontains('192.168.16.0/24', '192.168.16.3');
1
```

## Usage

```
sqlite> .load ./ipaddr
sqlite> select ipfamily('2001:db8::1');
6
```

[Download](https://github.com/nalgeon/sqlean/releases/latest)