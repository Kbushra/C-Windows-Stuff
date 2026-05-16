gcc -mwindows -municode -g -o image_renderer.exe main.c headers/*.c headers/assets/*.c -lgdi32 -lmsimg32 -lz
echo "Built"