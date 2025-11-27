# beatrice-android

<img src="./app/src/main/res/icon_round.png" width="50%">

[beatrice-vst](https://github.com/prj-beatrice/beatrice-vst) を Android へ移植したアプリです。

## How to Use

TBD...


## Project License

This repository is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.  

このリポジトリは MIT ライセンスの下で公開されています。詳細は [LICENSE](LICENSE) ファイルをご覧ください。

## Third-party Libraries and Licenses

This project uses the following third-party libraries as Git submodules  
このプロジェクトでは、以下のサードパーティライブラリを Git サブモジュールとして使用しています

- [Beatrice VST](https://github.com/prj-beatrice/beatrice-vst) : [MIT License](https://github.com/prj-beatrice/beatrice-vst/blob/main/LICENSE.txt)
- [Oboe](https://github.com/google/oboe) : [Apache License 2.0](https://github.com/google/oboe/blob/main/LICENSE)
- [Asio](https://github.com/chriskohlhoff/asio) : [Boost Software License 1.0](https://github.com/chriskohlhoff/asio/blob/master/LICENSE_1_0.txt)


And this application uses the inference library beatrice.lib under permission from [Project Beatrice](https://prj-beatrice.com/).  
また、このアプリは [Project Beatrice](https://prj-beatrice.com/) の許諾を受けて Beatrice の推論ライブラリ beatrice.lib を利用しています。

For running on Arm platforms, it uses a modified version of beatrice.lib adapted by [w-okada](https://github.com/w-okada) for the M1 Mac version of [VCClient](https://github.com/w-okada/voice-changer), with permission from w-okada.  
なお、Arm で動かすにあたり[w-okada](https://github.com/w-okada)氏が M1 Mac向け [VCClient](https://github.com/w-okada/voice-changer) のために改変した beatrice.lib を w-okada 氏の許諾を受けて利用しています。

These permission are limited to reasonable use for the development of beatrice-android, and to distribution or transfer of beatrice-android without modification.  
これらの許諾は、beatrice-android の開発に要する合理的な範囲での利用及び beatrice-android を改変せずに頒布・譲渡する行為のみを認めるものです。

Any use beyond this scope requires explicit permission from Project Beatrice and w-okada.  
この範囲を超えて利用する場合は Project Beatrice ならびに w-okada 氏に許諾を得る必要があります。

Libraries used by beatrice.lib  
beatrice.lib は以下のライブラリを使用しています。

- [PocketFFT](https://gitlab.mpcdf.mpg.de/mtr/pocketfft) : [BSD-3-Clause License](https://gitlab.mpcdf.mpg.de/mtr/pocketfft/-/blob/cpp/LICENSE.md)
- [fmath](https://github.com/herumi/fmath) : [BSD-3-Clause License](https://github.com/herumi/fmath#license)
