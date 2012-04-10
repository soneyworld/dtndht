set terminal png truecolor font "Helvetica" 10 size 1000,800
set output 'announcement_duration.png'
set datafile separator ";"
set ylabel 'duration until start [sec]'
set xlabel 'timeline [hours]'
set xtics 3600
set ytics 0,60
set yrange [0:840]
set format x ""
set title "Announcement Duration" font "Helvetica,12"
plot 'announcement_duration.csv' every::1::500 using 3:4 with lines title "2012-03-28"
set terminal png truecolor font "Helvetica" 10 size 1000,800
set output 'announcement_duration2.png'
set title "Announcement Duration" font "Helvetica,12"
plot 'announcement_duration.csv' every::501::1001 using 3:4 with lines title "2012-04-01"
