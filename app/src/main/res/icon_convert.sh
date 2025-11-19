#!/bin/bash
convert icon.png -resize 48x48 mipmap-mdpi/ic_launcher.webp
convert icon.png -resize 72x72 mipmap-hdpi/ic_launcher.webp
convert icon.png -resize 96x96 mipmap-xhdpi/ic_launcher.webp
convert icon.png -resize 144x144 mipmap-xxhdpi/ic_launcher.webp
convert icon.png -resize 192x192 mipmap-xxxhdpi/ic_launcher.webp

convert icon_round.png -resize 48x48 mipmap-mdpi/ic_launcher_round.webp
convert icon_round.png -resize 72x72 mipmap-hdpi/ic_launcher_round.webp
convert icon_round.png -resize 96x96 mipmap-xhdpi/ic_launcher_round.webp
convert icon_round.png -resize 144x144 mipmap-xxhdpi/ic_launcher_round.webp
convert icon_round.png -resize 192x192 mipmap-xxxhdpi/ic_launcher_round.webp

convert icon_foreground.png -resize 108x108 mipmap-mdpi/ic_launcher_foreground.webp
convert icon_foreground.png -resize 162x162 mipmap-hdpi/ic_launcher_foreground.webp
convert icon_foreground.png -resize 216x216 mipmap-xhdpi/ic_launcher_foreground.webp
convert icon_foreground.png -resize 324x324 mipmap-xxhdpi/ic_launcher_foreground.webp
convert icon_foreground.png -resize 432x432 mipmap-xxxhdpi/ic_launcher_foreground.webp
