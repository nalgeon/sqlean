-- Copyright (c) 2021 Anton Zhiyanov, MIT License
-- https://github.com/nalgeon/sqlean

.load dist/fcmp

-- Floating point numbers comparison
select '01', feq(0.1*3, 0.3) = 1;
select '02', feq(10.0/3, 3.34) = 0;
select '03', feq(10.0/3, 3.333333333333333) = 1;

select '11', fne(10.0/3, 3.33) = 1;
select '12', fne(10.0/3, 3.34) = 1;

select '21', flt(10.0/3, 3.34) = 1;
select '22', flt(10.0/3, 3.33) = 0;

select '31', fle(10.0/3, 3.34) = 1;
select '32', fle(10.0/3, 3.33) = 0;

select '41', fge(10.0/3, 3.33) = 1;
select '42', fge(10.0/3, 3.34) = 0;

select '51', fgt(10.0/3, 3.33) = 1;
select '52', fgt(10.0/3, 3.34) = 0;

-- Half to Odd
select '101', roundho(5) = 5.0;
select '102', roundho(4.6) = 5.0;
select '103', roundho(4.5) = 5.0;
select '104', roundho(4.4) = 4.0;
select '105', roundho(3.6) = 4.0;
select '106', roundho(3.5) = 3.0;
select '107', roundho(3.4) = 3.0;
select '108', roundho(-3.4) = -3.0;
select '109', roundho(-3.5) = -3.0;
select '110', roundho(-3.6) = -4.0;
select '111', roundho(-4.4) = -4.0;
select '112', roundho(-4.5) = -5.0;
select '113', roundho(-4.6) = -5.0;
select '114', roundho(-5) = -5.0;

-- Half to Even
select '121', roundhe(5) = 5.0;
select '122', roundhe(4.6) = 5.0;
select '123', roundhe(4.5) = 4.0;
select '124', roundhe(4.4) = 4.0;
select '125', roundhe(3.6) = 4.0;
select '126', roundhe(3.5) = 4.0;
select '127', roundhe(3.4) = 3.0;
select '128', roundhe(-3.4) = -3.0;
select '129', roundhe(-3.5) = -4.0;
select '130', roundhe(-3.6) = -4.0;
select '131', roundhe(-4.4) = -4.0;
select '132', roundhe(-4.5) = -4.0;
select '133', roundhe(-4.6) = -5.0;
select '134', roundhe(-5) = -5.0;

-- Half Away from 0
select '141', roundha(5) = 5.0;
select '142', roundha(4.6) = 5.0;
select '143', roundha(4.5) = 5.0;
select '144', roundha(4.4) = 4.0;
select '145', roundha(3.6) = 4.0;
select '146', roundha(3.5) = 4.0;
select '147', roundha(3.4) = 3.0;
select '148', roundha(-3.4) = -3.0;
select '149', roundha(-3.5) = -4.0;
select '150', roundha(-3.6) = -4.0;
select '151', roundha(-4.4) = -4.0;
select '152', roundha(-4.5) = -5.0;
select '153', roundha(-4.6) = -5.0;
select '154', roundha(-5) = -5.0;

-- Half Towards 0
select '161', roundht(5) = 5.0;
select '162', roundht(4.6) = 5.0;
select '163', roundht(4.5) = 4.0;
select '164', roundht(4.4) = 4.0;
select '165', roundht(3.6) = 4.0;
select '166', roundht(3.5) = 3.0;
select '167', roundht(3.4) = 3.0;
select '168', roundht(-3.4) = -3.0;
select '169', roundht(-3.5) = -3.0;
select '170', roundht(-3.6) = -4.0;
select '171', roundht(-4.4) = -4.0;
select '172', roundht(-4.5) = -4.0;
select '173', roundht(-4.6) = -5.0;
select '174', roundht(-5) = -5.0;

-- Directed Down
select '181', rounddd(5) = 5.0;
select '182', rounddd(4.6) = 4.0;
select '183', rounddd(4.5) = 4.0;
select '184', rounddd(4.4) = 4.0;
select '185', rounddd(3.6) = 3.0;
select '186', rounddd(3.5) = 3.0;
select '187', rounddd(3.4) = 3.0;
select '188', rounddd(-3.4) = -4.0;
select '189', rounddd(-3.5) = -4.0;
select '190', rounddd(-3.6) = -4.0;
select '191', rounddd(-4.4) = -5.0;
select '192', rounddd(-4.5) = -5.0;
select '193', rounddd(-4.6) = -5.0;
select '194', rounddd(-5) = -5.0;

-- Directed Up
select '201', rounddu(5) = 5.0;
select '202', rounddu(4.6) = 5.0;
select '203', rounddu(4.5) = 5.0;
select '204', rounddu(4.4) = 5.0;
select '205', rounddu(3.6) = 4.0;
select '206', rounddu(3.5) = 4.0;
select '207', rounddu(3.4) = 4.0;
select '208', rounddu(-3.4) = -3.0;
select '209', rounddu(-3.5) = -3.0;
select '210', rounddu(-3.6) = -3.0;
select '211', rounddu(-4.4) = -4.0;
select '212', rounddu(-4.5) = -4.0;
select '213', rounddu(-4.6) = -4.0;
select '214', rounddu(-5) = -5.0;

-- Directed Towards 0
select '221', rounddt(5) = 5.0;
select '222', rounddt(4.6) = 4.0;
select '223', rounddt(4.5) = 4.0;
select '224', rounddt(4.4) = 4.0;
select '225', rounddt(3.6) = 3.0;
select '226', rounddt(3.5) = 3.0;
select '227', rounddt(3.4) = 3.0;
select '228', rounddt(-3.4) = -3.0;
select '229', rounddt(-3.5) = -3.0;
select '230', rounddt(-3.6) = -3.0;
select '231', rounddt(-4.4) = -4.0;
select '232', rounddt(-4.5) = -4.0;
select '233', rounddt(-4.6) = -4.0;
select '234', rounddt(-5) = -5.0;

-- Directed Away from 0
select '241', roundda(5) = 5.0;
select '242', roundda(4.6) = 5.0;
select '243', roundda(4.5) = 5.0;
select '244', roundda(4.4) = 5.0;
select '245', roundda(3.6) = 4.0;
select '246', roundda(3.5) = 4.0;
select '247', roundda(3.4) = 4.0;
select '248', roundda(-3.4) = -4.0;
select '249', roundda(-3.5) = -4.0;
select '250', roundda(-3.6) = -4.0;
select '251', roundda(-4.4) = -5.0;
select '252', roundda(-4.5) = -5.0;
select '253', roundda(-4.6) = -5.0;
select '254', roundda(-5) = -5.0;

-- Half Down
select '261', roundhd(5) = 5.0;
select '262', roundhd(4.6) = 5.0;
select '263', roundhd(4.5) = 4.0;
select '264', roundhd(4.4) = 4.0;
select '265', roundhd(3.6) = 4.0;
select '266', roundhd(3.5) = 3.0;
select '267', roundhd(3.4) = 3.0;
select '268', roundhd(-3.4) = -3.0;
select '269', roundhd(-3.5) = -4.0;
select '270', roundhd(-3.6) = -4.0;
select '271', roundhd(-4.4) = -4.0;
select '272', roundhd(-4.5) = -5.0;
select '273', roundhd(-4.6) = -5.0;
select '274', roundhd(-5) = -5.0;

-- Half Up
select '281', roundhu(5) = 5.0;
select '282', roundhu(4.6) = 5.0;
select '283', roundhu(4.5) = 5.0;
select '284', roundhu(4.4) = 4.0;
select '285', roundhu(3.6) = 4.0;
select '286', roundhu(3.5) = 4.0;
select '287', roundhu(3.4) = 3.0;
select '288', roundhu(-3.4) = -3.0;
select '289', roundhu(-3.5) = -3.0;
select '290', roundhu(-3.6) = -4.0;
select '291', roundhu(-4.4) = -4.0;
select '292', roundhu(-4.5) = -4.0;
select '293', roundhu(-4.6) = -5.0;
select '294', roundhu(-5) = -5.0;

-- Money Special (Half to Even, 4 digits)
select '301', money(2) = 2.0;
select '302', money(1.44446) = 1.4445;
select '303', money(1.44445) = 1.4444;
select '304', money(1.44444) = 1.4444;
select '305', money(1.33336) = 1.3334;
select '306', money(1.33335) = 1.3334;
select '307', money(1.33334) = 1.3333;
select '308', money(-1.33334) = -1.3333;
select '309', money(-1.33335) = -1.3334;
select '310', money(-1.33336) = -1.3334;
select '311', money(-1.44444) = -1.4444;
select '312', money(-1.44445) = -1.4444;
select '313', money(-1.44446) = -1.4445;
select '314', money(-5) = -5.0;
