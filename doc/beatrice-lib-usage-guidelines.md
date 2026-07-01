# beatrice.lib の利用に関するガイドライン from Project Beatrice

- beatrice-androidは、モデルの設定が記載されたtomlに基づき、使用中または描画中のmodel、voice、portraitに対応したdescriptionを可能な限り画面に表示する。
- 話者モーフィングを行う際は、重みが0でない各voiceに対応したdescriptionと https://github.com/prj-beatrice/beatrice-vst/pull/8 に準じた注意書きを表示するよう努める。
- beatrice-androidの管理者は、第三者がbeatrice.libを許諾範囲外で利用することを防ぐため、以下の内容をREADMEあるいはそれに類するファイルに日本語又は英語で目立つように適切に記載する。
  - Project Beatriceの許諾を受けてbeatrice.libを利用していること
  - この許諾は、beatrice-androidの開発に要する合理的な範囲での利用及び(beatrice.libを利用してビルドした)beatrice-androidを改変せずに頒布・譲渡する行為のみを、その実施者に関わらず認めるものであること

- beatrice.libが依存する下記ライブラリのライセンスに従う。

  - PocketFFT
    - BSD-3-Clause License
    - gitlab.mpcdf.mpg.de/mtr/pocketfft/-/blob/cpp/LICENSE.md

  - fmath
    - BSD-3-Clause License
    - github.com/herumi/fmath#license

  - (Arm のみ) SIMD Everywhere
    - MIT License
    - github.com/simd-everywhere/simde/blob/master/COPYING