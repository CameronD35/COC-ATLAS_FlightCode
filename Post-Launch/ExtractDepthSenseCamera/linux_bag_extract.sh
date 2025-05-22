if [ $# -ne 1 ]; then
  echo "Please provide name of bag file! EG. $0 yourfile.bag"
  exit 1
fi

BAGFILE="$1"
BASENAME=$(basename "$BAGFILE" .bag)
OUTDIR="RSXOutput_${BASENAME}"

echo "Making folders and subfolders"
mkdir -p "$OUTDIR/depth_pngs"
mkdir -p "$OUTDIR/color_pngs"
mkdir -p "$OUTDIR/infrared_pngs"
mkdir -p "$OUTDIR/metadata"

echo "Extracting files from bagfile; wait a moment and excuse the mess!"
rs-convert -i "$BAGFILE" -p rsx

echo "Organizing the mess!"
mv rsx_Depth*.png "$OUTDIR/depth_pngs/" 2>/dev/null
mv rsx_Color*.png "$OUTDIR/color_pngs/" 2>/dev/null
mv rsx_Infrared*.png "$OUTDIR/infrared_pngs/" 2>/dev/null
mv rsx_*.txt "$OUTDIR/metadata/" 2>/dev/null

echo "Generating video for Depth"
cd "$OUTDIR/depth_pngs" || exit 1
if ls rsx_Depth*.png 1> /dev/null 2>&1; then
  ffmpeg -framerate 30 -pattern_type glob -i 'rsx_Depth_*.png' -c:v libx264 -pix_fmt yuv420p ../RSXDepthOutput.mp4
fi

echo "Generating videos for Colour"
cd ../color_pngs || exit 1
if ls rsx_Color*.png 1> /dev/null 2>&1; then
  ffmpeg -framerate 30 -pattern_type glob -i 'rsx_Color_*.png' -c:v libx264 -pix_fmt yuv420p ../RSXColorOutput.mp4
fi

echo "Generating videos for Infrared (if there are any)"
cd ../infrared_pngs || exit 1
if ls rsx_Infrared*.png 1> /dev/null 2>&1; then
  ffmpeg -framerate 30 -pattern_type glob -i 'rsx_Infrared_*.png' -c:v libx264 -pix_fmt yuv420p ../RSXInfraredOutput.mp4
fi

echo "Finished, sorted, and ready for your use in: $OUTDIR!"