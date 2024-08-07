# ipaddr: IP address manipulation in SQLite

The `sqlean-ipaddr` extension provides functions to manipulate IPs and subnets.

⚠️ This extension is not available on Windows.

[Reference](#reference) •
[Acknowledgements](#acknowledgements) •
[Installation and usage](#installation-and-usage)

## Reference

[contains](#ipcontains) •
[family](#ipfamily) •
[host](#iphost) •
[masklen](#ipmasklen) •
[network](#ipnetwork)

### ipcontains

```text
ipcontains(subnet, ip)
```

Reports whether `subnet` contains `ip` (which may be another subnet).

```sql
select ipcontains('192.168.16.0/24', '192.168.16.3');
-- 1
```

### ipfamily

```text
ipfamily(ip)
```

Returns the family of a specified IP address.

```sql
select ipfamily('192.168.1.1');
-- 4
```

### iphost

```text
iphost(ip)
```

Returns the host part of an IP address.

```sql
select iphost('2001:db8::123/64');
-- 2001:db8::123
```

### ipmasklen

```text
ipmasklen(ip)
```

Returns the prefix length of an IP address.

```sql
select ipmasklen('192.168.16.12/24');
-- 24
```

### ipnetwork

```text
ipnetwork(ip)
```

Returns the network part of an IP address.

```sql
select ipnetwork('192.168.16.12/24');
-- 192.168.16.0/24
```

## Acknowledgements

Contributed by [Vincent Bernat](https://github.com/vincentbernat).

## Installation and usage

SQLite command-line interface:

```
sqlite> .load ./ipaddr
sqlite> select ipfamily('2001:db8::1');
```

See [How to install an extension](install.md) for usage with IDE, Python, etc.

↓ [Download](https://github.com/nalgeon/sqlean/releases/latest) the extension.

⛱ [Explore](https://github.com/nalgeon/sqlean) other extensions.

★ [Subscribe](https://antonz.org/subscribe/) to stay on top of new features.
