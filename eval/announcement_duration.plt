set terminal png truecolor font small size 1000,1000
set output 'announcement_duration.png'
set datafile separator ";"
plot 'announcement_duration.csv' every::1::500 using 3:4 with lines
set terminal png truecolor font small size 1000,1000
set output 'announcement_duration2.png'
plot 'announcement_duration.csv' every::501::1001 using 3:4 with lines
