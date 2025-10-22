#!/bin/bash
convert icon.png -resize 48x48 mipmap-mdpi/icon.png
convert icon.png -resize 72x72 mipmap-hdpi/icon.png
convert icon.png -resize 96x96 mipmap-xhdpi/icon.png
convert icon.png -resize 144x144 mipmap-xxhdpi/icon.png
convert icon.png -resize 192x192 mipmap-xxxhdpi/icon.png

convert icon_round.png -resize 48x48 mipmap-mdpi/icon_round.png
convert icon_round.png -resize 72x72 mipmap-hdpi/icon_round.png
convert icon_round.png -resize 96x96 mipmap-xhdpi/icon_round.png
convert icon_round.png -resize 144x144 mipmap-xxhdpi/icon_round.png
convert icon_round.png -resize 192x192 mipmap-xxxhdpi/icon_round.png
