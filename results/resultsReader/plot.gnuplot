
set xlabel "Time"
set ylabel "Sample Value"

#set timefmt "%Y-%m-%d-%H:%M:%.6S" */
#set timefmt "%Y-%m-%d-%H:%M:%.6S" */

# plot "./out.BHN" using 2 pointtype 5 pointsize 0.1, "./out.BHE" using 2 pointtype 5 pointsize 0.1, "./out.BHZ" using 2 pointtype 5 pointsize 0.1
plot "./out.BHN" using 1:2 pointtype 5 pointsize 0.1

pause -1 "Hit any key to continue"
