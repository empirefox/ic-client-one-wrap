

follow [FFmpeg wiki](http://ffmpeg.org/trac/ffmpeg/wiki/Create%20a%20video%20slideshow%20from%20images) to make mp4:
```
ffmpeg -framerate 1/5 -start_number 126 -i reference.pnm -c:v libx264 -r 30 -pix_fmt yuv420p 5s-256x256x30fps.mp4
```
