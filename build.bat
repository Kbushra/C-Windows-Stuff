gcc -g -mwindows -municode main.c headers/*.c headers/assets/*.c -lgdi32 -lmsimg32 -lz -o image_renderer.exe
::gdb image_renderer.exe
echo "Built"