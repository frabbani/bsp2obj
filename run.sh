#!/bin/bash

echo "Cleaning..."
rm -rf export/textures/*.png
rm -rf *.obj
rm -rf *.bin
rm -rf *.gltf
rm -rf export/*.obj
echo "Done! Running..."
./bsp2obj 2fort5r.bsp
echo "Done!"
