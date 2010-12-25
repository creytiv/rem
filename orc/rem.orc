

# Convert from YUYV to YUV420P
.function convert_yuyv_to_yuv420p
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

.temp 2 w1
.temp 2 w0

#
#  Ya U  Yb V
#  ----  ----
#  1st   0th
#


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
