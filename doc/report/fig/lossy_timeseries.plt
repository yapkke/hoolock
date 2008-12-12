#set title "Application latency"
set xlabel "Time (seconds)"
set ylabel "Packets lost"
set xrange [0:20]
set yrange [-1:50]

set key top left

set terminal postscript eps enhanced "Helvetica" 22
set output 'lossy_timeseries.eps'


plot "lossy_timeseries.dat" u 1:2 title 'No-handover' w linespoints ps 2 pt 6 , \
	"lossy_timeseries.dat" u 1:3 title 'Hoolock' w linespoints ps 2 pt 2 , \
	"lossy_timeseries.dat" u 1:4 title 'Hard' w linespoints ps 2 pt 4
