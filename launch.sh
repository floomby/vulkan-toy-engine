#!/bin/bash
# These are the environment varibles needed for the debian package launcher to run
export LD_LIBRARY_PATH=/opt/nvidia/nsight-graphics-for-linux/nsight-graphics-for-linux-2021.4.1/host/linux-desktop-nomad-x64/:LD_LIBRARY_PATH
export QT_QPA_PLATFORM=offscreen
export NV_AGORA_PATH=/opt/nvidia/nsight-graphics-for-linux/nsight-graphics-for-linux-2021.4.1/host/linux-desktop-nomad-x64/
echo /opt/nvidia/nsight-graphics-for-linux/nsight-graphics-for-linux-2021.4.1/host/linux-desktop-nomad-x64/ngfx.bin