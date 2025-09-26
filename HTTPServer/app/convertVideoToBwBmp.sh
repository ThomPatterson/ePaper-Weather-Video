#!/bin/sh

#script will create a directory of dithered black and white frames from the video as bitmap

# Configurable variables
FRAMERATE=5  # Set the desired framerate out output frames (e.g. 5 ==  five frames per second)

# Input video file
input_video="$1"
directory=$(dirname "$input_video")
filename=$(basename "$input_video")
extension="${filename##*.}"
filename="${filename%.*}"

# Output directory for frames
temp_dir="/usr/data/videos/temp"
#temp_dir="/Users/thom/Development/PictureFrame/Server/data/temp"
colored_output_dir="$temp_dir/${filename}_color_frames"
dithered_output_dir="$temp_dir/${filename}_dithered_frames"
bitmap_output_dir="$temp_dir/${filename}_bitmap_frames"

# Create output directories if they don't exist
mkdir -p "$temp_dir"
mkdir -p "$colored_output_dir"
mkdir -p "$dithered_output_dir"
mkdir -p "$bitmap_output_dir"

# Extract frames using FFmpeg with dynamic scaling and cropping
# Scale the video to ensure the shortest side matches the crop size while maintaining aspect ratio
# Then crop to 800x480
ffmpeg -i "$input_video" -vf "scale='if(gt(a,800/480),-1,800)':'if(gt(a,800/480),480,-1)',crop=800:480" -r "$FRAMERATE" "$colored_output_dir/frame_%06d.png"

# Dither each frame using didder
for frame in "$colored_output_dir"/*.png; do
    base_name=$(basename "$frame" .png)
    didder --palette "black white" -i "$frame" -o "$dithered_output_dir/$base_name.png" --brightness 0.2 --contrast 0.05 edm FloydSteinberg

    # Convert dithered frame to 1-bit BMP
    convert "$dithered_output_dir/$base_name.png" -monochrome "$bitmap_output_dir/$base_name.bmp"
done

echo "Colored frames extracted and saved in $colored_output_dir"
echo "Dithered frames saved in $dithered_output_dir"
echo "Bitmap frames saved in $bitmap_output_dir"

# Move the bitmap_output_dir to the directory of the input file
mv "$bitmap_output_dir" "$directory"

# Delete the temporary directory
rm -rf "$temp_dir"
