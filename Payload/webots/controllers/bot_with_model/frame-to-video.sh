#!/bin/bash

ffmpeg -framerate 10 -pattern_type glob -i "aicam/*.jpg"        -c:v libx264 -pix_fmt yuv420p aicam/video.mp4
