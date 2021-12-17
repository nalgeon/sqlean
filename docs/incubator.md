## cbrt

Cube root function.

Created by [Anton Zhiyanov](https://github.com/nalgeon/sqlean/blob/incubator/src/cbrt.c), MIT License.

```sql
sqlite> .load dist/cbrt
sqlite> select cbrt(27);
3.0
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/cbrt.dylib)

## eval

Dinamically runs arbitrary SQL statements.

Created by [D. Richard Hipp](https://sqlite.org/src/file/ext/misc/eval.c), Public Domain.

```sql
sqlite> .load dist/eval
sqlite> select eval('select 42');
42
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/eval.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/eval.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/eval.dylib)

## xmltojson

Converts an XML string to the corresponding JSON string.

Created by [jakethaw](https://github.com/jakethaw/xml_to_json), MIT License.

```sql
sqlite> .load dist/xmltojson
sqlite> select xml_to_json('<answer>42</answer>');
{"answer":"42"}
```

Download: [linux](https://github.com/nalgeon/sqlean/releases/download/incubator/xmltojson.so) | [windows](https://github.com/nalgeon/sqlean/releases/download/incubator/xmltojson.dll) | [macos](https://github.com/nalgeon/sqlean/releases/download/incubator/xmltojson.dylib)