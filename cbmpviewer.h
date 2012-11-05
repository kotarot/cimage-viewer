/**
 * cbmpviewer.h
 */

#ifndef __CBMPVIEWER_H__
#define __CBMPVIEWER_H__

#include <stdint.h>

// 構造体定義
// ファイルヘッダ
typedef struct TAG_BITMAPFILEHEADER {
    uint16_t type;      // ファイルタイプ'BM'
    uint32_t size;      // ファイルサイズ(byte)
    uint16_t reserved1; // 予約(常に0)
    uint16_t reserved2; // 予約(常に0)
    uint32_t offbits;   // ファイル先頭から画像データまでのオフセット(byte)
} bmpfileheader_t;

// 情報ヘッダ(Windows bitmap)
typedef struct TAG_BITMAPINFOHEADER {
    uint32_t size;         // 情報ヘッダのサイズ(40byte)
    int32_t  width;        // 画像の幅
    int32_t  height;       // 画像の高さ
    uint16_t planes;       // プレーン数(常に1)
    uint16_t bitcount;     // 1pixelあたりのデータサイズ(bit)
    uint32_t compression;  // 圧縮形式
    uint32_t sizeimage;    // 画像データ部のサイズ(byte)
    int32_t  xpixpermeter; // X方向解像度
    int32_t  ypixpermeter; // Y方向解像度
    uint32_t clrused;      // パレット数
    uint32_t clrimporant;  // 重要なパレットのインデックス
} bmpinfoheader_t;

// Pixel構造体
typedef struct TAG_PIXEL {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} pixel_t;

// 関数プロトタイプ宣言
// Usage
void usage(void);
// Viewプロシージャ
void viewproc(char *filename);
// 画像ヘッダ取得
void getbmpheader(FILE *fp, bmpfileheader_t *fh, bmpinfoheader_t *ih);
// ファイルポインタから指定のバイト取得(エラー処理付き)
void freadwitherror(void *buf, int byte, FILE *fp);
// 画像ヘッダの(このプログラムで対応する)フォーマットチェック
void checkbmpheader(bmpfileheader_t *fh, bmpinfoheader_t *ih);
// 画像ヘッダの表示
void showbmpheader(bmpfileheader_t *fh, bmpinfoheader_t *ih);
// 画像データをメモリに読み込む
void readbmpdata(FILE *fp, pixel_t **pix, int32_t width, int32_t height);
// 画像データの表示
void showbmpdata(pixel_t **pix, int32_t width, int32_t height);

#endif
