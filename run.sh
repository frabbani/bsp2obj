#!/bin/bash

clear
echo "cleaning..."
rm -rf export/textures/*.png
# rm -rf *.obj
rm -rf *.bin
rm -rf *.gltf
rm -rf export/*.obj
echo "done! copying..."
cp ~/git_uploads/grid_trace/master/build/*.dll .
echo "done! Running..."
./bsp2obj 2fort5r.bsp
echo "done!"
