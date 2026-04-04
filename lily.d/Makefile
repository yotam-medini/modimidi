# Author:  Yotam Medini  yotam.medini@gmail.com -- Created: 2026/April/04

icon.png: icon.ly
	lilypond -fpng -dresolution=600 -dcrop icon.ly
	convert icon.cropped.png -resize 512x512 \
            -fill "#aaa" -opaque "#ffffff" \
	    -background green -gravity center -extent 512x512 $@

clean:
	rm -f icon.png
