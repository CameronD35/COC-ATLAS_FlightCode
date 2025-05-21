# STEPS TO EXTRACT ON LINUX

(NOTE: Revise and update this later so it looks nicer and is less jank.)

### Install Intel Real Sense SDK (librealsense2)

```
sudo apt update
sudo apt-get install libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev
sudo apt install git libgtk-3-dev cmake build-essential libssl-dev libusb-1.0-0-dev pkg-config libgtk-3-dev
```

### Download Repository

```
git clone https://github.com/IntelRealSense/librealsense.git
cd librealsense
```

### Set UDEV rules

```
sudo ./scripts/setup_udev_rules.sh
```

### Build SDK

```
mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=true -DBUILD_GRAPHICAL_EXAMPLES=false
make -j$(nproc)
sudo make install
```

### Check to make sure rs-convert installed

```
which rs-convert
rs-convert --help
```


### Install ffmpeg (If you don't already have)

```
sudo apt install ffmpeg
```


### Run the linux_bag_extract script

This script uses the first parameter following the command as the name of the bagfile.

```
. linux_bag_extract.sh example.bag
```

Replacing `example` with the actual bag file name, of course!

**DON'T BE ALARMED** It's going to create a BUTT TON of files wherever you run it, but it will later sort those files in a folder called "RSX_Output" that has all the depth, color, and metadata sorted.

The reason for this is that `rs-convert` doesn't have a parameter to specify the path to save files (it doesn't have a `-o`, so I decided to compensate with bash).


## OUTPUT

Everything will be in a folder called RSX_Output and will be sorted into colour, depth, and infrared (if it's available; it was for the example bag file I was using, so I figured it'd be good to add it just in case!)