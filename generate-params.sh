
for n in $(seq 1 $[1 + 126])
do
  ./db -S /usr/share/sounds/sf2/FluidR3_GM.sf2 -r 48000 -b 1024 -f float -g 5 -N 6 0 $n 127 1000000 0 $n 0 1000000 0 $n 127 1000000 0 $n 0 1000000 0 $n 127 1000000 0 $n 0
done
