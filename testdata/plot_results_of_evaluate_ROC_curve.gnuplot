
set term png size 1024, 1024
set output "figure.png"

set style line 11 lc rgb '#808080' lt 1
set border 3 back ls 11
set tics nomirror
# define grid
set style line 12 lc rgb '#808080' lt 0 lw 1
set grid back ls 12
set key box
set key font ",10"

set size 4, 4;
set multiplot layout 2, 2;
# Plot the TPR FPR IRR for exact search.
set key top left
set title "TPR, FPR, and IRR for exact search"
set xlabel "Maximum hamming distance to consider"
set ylabel "Percent"
set xrange [0:80]
plot "./roc_untrained.txt" using 1:2 with lines ls 1 title "TPR exact", \
  "./roc_untrained.txt" using 1:3 with lines ls 2 title "FPR exact", \
  "./roc_untrained.txt" using 1:6 with lines ls 3 title "IRR exact";

# Plot the TPR FPR IRR for approximate search.
set title "TPR, FPR, and IRR for approximate search"
set xlabel "Maximum hamming distance to consider"
set ylabel "Percent"
set xrange [0:80]
plot "./roc_untrained.txt" using 1:4 with lines ls 1 title "TPR approx", \
  "./roc_untrained.txt" using 1:5 with lines ls 2 title "FPR approx", \
  "./roc_untrained.txt" using 1:7 with lines ls 3 title "IRR approx";

# Plot precision vs Recall curve.
set title "Precision vs. Recall"
set xlabel "Recall"
set ylabel "Precision"
set size square
set xrange [0:1]
set yrange [0:1]
precision(x) = 1.0 - x
# Plot the precision vs. recall curve.
plot "./roc_untrained.txt" using 2:(precision($6)) with lines ls 2 \
  title "Exact", "./roc_untrained.txt" using 4:(precision($7)) \
  with lines ls 1 title "Approx";

# Plot FPR vs TPR (ROC).
set key bottom right
set title "FPR vs TPR (ROC)"
set size square
set xlabel "FPR"
set ylabel "TPR"
plot "./roc_untrained.txt" using 3:2 with lines ls 2 title "FPR vs TPR exact",\
  "./roc_untrained.txt" using 5:4 with lines ls 1 title "FPR vs TPR approx", \
  x with lines

