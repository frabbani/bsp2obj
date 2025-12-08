#!/bin/bash

echo "Cleaning..."
rm -rf *.obj
echo "Done! Running..."
./bsp2obj e1m1.bsp
echo "Done!"
