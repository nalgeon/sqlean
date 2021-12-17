-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/xmltojson

select '01', xml_to_json('<answer>42</answer>') = '{"answer":"42"}';

select '02', xml_to_json('<answer>42</answer>', 2) = '{
  "answer": "42"
}
';

select '03', xml_to_json('
<val>1</val>
<val>2</val>
<val>3</val>') = '{"val":["1","2","3"]}';

select '04', xml_to_json('
<ans>
  <v>1</v>
  <v>2</v>
  <v>3</v>
</ans>') = '{"ans":{"v":["1","2","3"]}}';

select '05', xml_to_json('
<people>
  <person id="11" city="London">Alice</person>
  <person id="12" city="Berlin">Bob</person>
</people>', 2) = '{
  "people": {
    "person": [
      {
        "@id": "11",
        "@city": "London",
        "#text": "Alice"
      },
      {
        "@id": "12",
        "@city": "Berlin",
        "#text": "Bob"
      }
    ]
  }
}
';