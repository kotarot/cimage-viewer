# CBmpViewer
## とは
コンソール上でBMP画像を(無理矢理)表示するプログラム。  
とりあえず作ったものなのでごくわずかのフォーマットにしか対応していない。  
具体的には、Windows Bitmapの無圧縮24ビットビットマップのみ対応。  
コンソールでのエスケープシーケンスでの色表示するのは完全に機種依存だからうまく表示されるかは保証しない。  
もちろん使用色は8色になって画素密度は小さくなるので粗い画像(のようなもの)になる。

## コンパイル・インストール・使い方
    $ make  
    $ make install  
    $ cbmpviewer [input.bmp]  
make installはしてもしなくても別にいい。

## 参考サイト
* bmp ファイルフォーマット - Kondo, Masayoshi WebPage  
http://www.kk.iij4u.or.jp/~kondo/bmp/

* 第6章 C言語による実践プログラミング - Armadillo実践開発ガイド  
http://manual.atmark-techno.com/armadillo-guide/armadillo-guide-2_ja-1.0.0/ch06.html