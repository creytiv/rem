

#
# Convert from YUYV422 to YUV420P
#

.function yuyv422_to_yuv420p
.flags 2d
.dest 2 y1
.dest 2 y2
.dest 1 u
.dest 1 v
.source 4 p1
.source 4 p2
.temp 4 x1
.temp 4 x2
.temp 2 hi
.temp 2 lo
.temp 1 ya
.temp 1 yb

swapl x1, p1
swapl x2, p2

splitlw hi, lo, x1
select1wb ya, hi
select0wb u, hi
select1wb yb, lo
select0wb v, lo
mergebw y1, ya, yb

splitlw hi, lo, x2
select1wb ya, hi
select1wb yb, lo
mergebw y2, ya, yb




#
# Convert from RGB32 to YUV420P
#

.function rgb32_to_yuv420p
.flags 2d
.dest 2 y1
.dest 2 y2
.dest 1 u
.dest 1 v
.source 4 p1
.source 4 p2

.temp 2 hi
.temp 2 lo
.temp 1 ya
.temp 1 yb

.temp 1 r0
.temp 1 g0
.temp 1 b0

.temp 2 r
.temp 2 g
.temp 2 b

.temp 4 ry0
.temp 4 gy0
.temp 4 by0

.temp 2 ry
.temp 2 gy
.temp 2 by

.temp 1 t1
.temp 2 t2

.const 1 c16 16
.const 1 c25 25
.const 1 c66 66
.const 1 c129 127
.const 1 c37n -37
.const 1 c73n -73
.const 1 c112 112
.const 1 c93n -93
.const 1 c17n -17
.const 1 c128 128

# Y1
splitlw hi, lo, p1
splitwb r0, g0, hi
splitwb b0, t1, lo

convubw r, r0
convubw g, g0
convubw b, b0

mulswl ry0, r, c66
mulswl gy0, g, c129
mulswl by0, b, c25

convssslw ry, ry0
convssslw gy, gy0
convssslw by, by0

addw t2, ry, gy
addw t2, t2, by
convhwb t1, t2
addusb ya, t1, c16

mergebw y1, ya, ya
mergebw y2, ya, ya


# U
mulswl ry, r, c37n
mullw gy, g, c73n
mullw by, b, c112

addw t2, ry, gy
addw t2, t2, by
shruw t2, t2, 8
addw t2, t2, c128
convsuswb u, t2


# V
mullw ry, r, c112
mullw gy, g, c93n
mullw by, b, c17n

addw t2, ry, gy
addw t2, t2, by
shruw t2, t2, 8
addw t2, t2, c128
convsuswb v, t2
