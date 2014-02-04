/**
 * CBmpViewer
 * コンソール上でBMP画像を表示するプログラム
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <math.h>
#include "cbmpviewer.h"

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#if DEBUG
#define debug(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define debug(...)
#endif /* DEBUG */
// メイン関数
int main(int argc, char *argv[]) {
    // 引数チェック
    if (argc < 2 || 5 < argc) {
        usage();
        return EXIT_SUCCESS;
    } else if (argc == 2) {
        // Viewプロシージャへ
        // しきい値はデフォルト値
        viewproc(argv[1], 128, 128, 128);
    } else {
        // しきい値のチェックしてないけど8bitに収まってればとりあえずおかしくはならないからこのままいく
        viewproc(argv[1], (uint8_t)atoi(argv[2]), (uint8_t)atoi(argv[3]), (uint8_t)atoi(argv[4]));
    }

    // 画面クリア
    //printf("\x1b[2J");

    return EXIT_SUCCESS;
}

// Usage
void usage(void) {
    printf("** CBmpViewer **\n");
    printf("Usage: `cbmpviewer <input.bmp> [threshold_r=128 threshold_g=128 threshold_b=128]`\n");
}

// Viewプロシージャ
void viewproc(char *filename, uint8_t threshold_r, uint8_t threshold_g, uint8_t threshold_b) {
    FILE *fp;
    bmpfileheader_t fh;
    bmpinfoheader_t ih;
    pixel_t **pix;
    uint32_t i;
    struct winsize win; // コンソールサイズ
    consolebmp_t cbmp;
    char *p;


    // 画像ファイルオープン
    if ((fp = fopen(filename, "rb")) == NULL) {
        printf("Error: file open\n");
        exit(EXIT_FAILURE);
    }
    debug("[FILEOPEN: OK]\n");

    // 画像ヘッダ取得
    getbmpheader(fp, &fh, &ih);
    showbmpheader(&fh, &ih);
    // 画像ヘッダの(このプログラムで対応する)フォーマットチェック
    checkbmpheader(&fh, &ih);
    debug("[FORMAT: OK]\n");

    // ピクセル分の配列を動的に確保
    if ((pix = (pixel_t **)malloc(sizeof(pixel_t *) * ih.height)) == NULL) {
        printf("Error: memory allocate\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < ih.height; i++) {
        if ((pix[i] = (pixel_t *)malloc(sizeof(pixel_t *) * ih.width)) == NULL) {
            printf("Error: memory allocate\n");
            exit(EXIT_FAILURE);
        }
    }
    debug("[MEMORYALLOC: OK]\n");

    // 画像データをメモリに読み込む
    readbmpdata(fp, pix, ih.width, ih.height);
    debug("[READDATA: OK]\n");
    //showbmpdata(pix, ih.width, ih.height);

    // ファイルクローズ
    fclose(fp);
    debug("[FILECLOSE: OK]\n");


   // コンソールサイズ取得
   // COLUMNS が指定されていなければ、コンソールサイズを取得
   // パイプの場合はデフォルト80またはCOLUMNS
    win.ws_col = 80;
    if ((p = getenv("COLUMNS")) != NULL && *p != '\0')
        win.ws_col = atoi(p);
    else {
        if ( isatty(STDOUT_FILENO) ) {
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
            debug("[CONSOLESIZE: OK] col=%u,row=%u\n", win.ws_col, win.ws_row);
        }
        else {
            debug("[PIPE: OK] col=%u,row=%u\n", win.ws_col, win.ws_row);
        }
    }

    // コンソール文字とピクセル比率の決定
    // ピクセル比率とはbmpでの何ピクセルがコンソールでの1文字になるか
    // 符号無し整数値で最小値は1
    // コンソールでの文字は縦横比が2:1になることも注意
    // 参照: pixel_letter.example
    cbmp.bpl_c = MAX(ih.width / win.ws_col, 1);
    cbmp.bpl_r = MAX(ih.height / win.ws_row, 1);
    if (cbmp.bpl_c < cbmp.bpl_r) cbmp.bpl_c = cbmp.bpl_r / 2;
    else cbmp.bpl_r = cbmp.bpl_c * 2;
    cbmp.letter = ih.width / cbmp.bpl_c;
    cbmp.line = ih.height / cbmp.bpl_r;
    debug("[BMP/LETTER: OK] bpl_c=%u,bpl_r=%u,letter=%u,line=%u\n", cbmp.bpl_c, cbmp.bpl_r, cbmp.letter, cbmp.line);

    // 色変換と出力
    cbmp.threshold_r = threshold_r;
    cbmp.threshold_g = threshold_g;
    cbmp.threshold_b = threshold_b;
    outputbmp(pix, &cbmp);

    // メモリ解放
    for (i = 0; i < ih.height; i++) {
        free(pix[i]);
    }
    free(pix);
    debug("[MEMORYFREE: OK]\n");
}

// 画像ヘッダ取得
void getbmpheader(FILE *fp, bmpfileheader_t *fh, bmpinfoheader_t *ih) {
    // 各値を取得
    freadwitherror(&fh->type, 2, fp);
    // タイプに関してはBM(リトルエンディアンで0x4d42)じゃなかったらそこで落とす
    if (fh->type != 0x4d42) {
        printf("Error: input file is not bitmap\n");
        exit(EXIT_FAILURE);
    }
    freadwitherror(&fh->size, 4, fp);
    freadwitherror(&fh->reserved1, 2, fp);
    freadwitherror(&fh->reserved2, 2, fp);
    freadwitherror(&fh->offbits, 4, fp);

    freadwitherror(&ih->size, 4, fp);
    freadwitherror(&ih->width, 4, fp);
    freadwitherror(&ih->height, 4, fp);
    freadwitherror(&ih->planes, 2, fp);
    freadwitherror(&ih->bitcount, 2, fp);
    freadwitherror(&ih->compression, 4, fp);
    freadwitherror(&ih->sizeimage, 4, fp);
    freadwitherror(&ih->xpixpermeter, 4, fp);
    freadwitherror(&ih->ypixpermeter, 4, fp);
    freadwitherror(&ih->clrused, 4, fp);
    freadwitherror(&ih->clrimporant, 4, fp);
}

// ファイルポインタから指定のバイト取得(エラー処理付き)
void freadwitherror(void *buf, int byte, FILE *fp) {
    if (fread(buf, byte, 1, fp) != 1) {
        printf("Error: file read\n");
        exit(EXIT_FAILURE);
    }
}

// 画像ヘッダの(このプログラムで対応する)フォーマットチェック
void checkbmpheader(bmpfileheader_t *fh, bmpinfoheader_t *ih) {
    // 高さが負数だったらはじく
    if (ih->height < 0) {
        printf("Error: this program currently does not work for top-down bmp\n");
        exit(EXIT_SUCCESS);
    }
    // ビットカウント24以外ははじく
    if (ih->bitcount != 24) {
        printf("Error: this program currently does not work for %u-bit bmp\n", ih->bitcount);
        exit(EXIT_SUCCESS);
    }
    // 無圧縮以外は弾く
    if (ih->compression != 0) {
        printf("Error: this program currently does not work for compressed bmp\n");
        exit(EXIT_SUCCESS);
    }
}

// 画像ヘッダの表示
void showbmpheader(bmpfileheader_t *fh, bmpinfoheader_t *ih) {
    debug("--file header--\n");
    debug("type: %c%c\n", (char)fh->type, (char)(fh->type >> 8));
    debug("size: %u\n", fh->size);
    debug("reserved1: %u\n", fh->reserved1);
    debug("reserved1: %u\n", fh->reserved2);
    debug("offbits: %u\n", fh->offbits);
    debug("--info header--\n");
    debug("size: %u\n", ih->size);
    debug("width: %d\n", ih->width);
    debug("height: %d\n", ih->height);
    debug("planes: %u\n", ih->planes);
    debug("bitcount: %u\n", ih->bitcount);
    debug("compression: %u\n", ih->compression);
    debug("sizeimage: %u\n", ih->sizeimage);
    debug("xpixpermeter: %d\n", ih->xpixpermeter);
    debug("ypixpermeter: %d\n", ih->ypixpermeter);
    debug("clrused: %u\n", ih->clrused);
    debug("clrimporant: %u\n", ih->clrimporant);
}

// 画像データをメモリに読み込む
// データはボトムアップに入っていることに注意する
void readbmpdata(FILE *fp, pixel_t **pix, int32_t w, int32_t h) {
    int i, j;

    for (i = h - 1; i >= 0; i--) {
        for (j = 0; j < w; j++) {
            // B G R の順
            freadwitherror(&pix[i][j].blue, 1, fp);
            freadwitherror(&pix[i][j].green, 1, fp);
            freadwitherror(&pix[i][j].red, 1, fp);
        }
        // 1行は4byteできり(つまり4の倍数)を合わせなきゃいけない
        // w:1 -> 3byte -> padding:1byte
        // w:2 -> 6byte -> padding:2byte
        // w:3 -> 9byte -> padding:3byte
        // w:4 -> 12byte -> padding:0byte
        // くりかえし
        fseek(fp, w % 4, SEEK_CUR);
    }
}

// 画像データの表示
void showbmpdata(pixel_t **pix, int32_t w, int32_t h) {
    int i, j;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            printf("R=%x G=%x B%x\n", pix[i][j].red, pix[i][j].green, pix[i][j].blue);
        }
        printf("\n");
    }
}

// 色変換と出力
void outputbmp(pixel_t **pix, consolebmp_t *cbmp) {
    uint32_t i, j, m, n;

    // BUG: ここのforループはbmpのpixelがletterの倍数になっていることを前提としちゃってるから
    //      そうじゃないときにメモリのおかしなところ参照しちゃってセグフォル
    for (i = 0; i < cbmp->line; i++) {
        for (j = 0; j < cbmp->letter; j++) {
            uint32_t r, g, b, s;
            uint8_t clr;

            // 1文字で表される文のピクセルのRGB値の平均を求める
            // まずはsum
            r = g = b = 0;
            for (m = 0; m < cbmp->bpl_r; m++) {
                for (n = 0; n < cbmp->bpl_c; n++) {
                    r += pix[i * cbmp->bpl_r + m][j * cbmp->bpl_c + n].red;
                    g += pix[i * cbmp->bpl_r + m][j * cbmp->bpl_c + n].green;
                    b += pix[i * cbmp->bpl_r + m][j * cbmp->bpl_c + n].blue;
                }
            }
            // 平均
            s = cbmp->bpl_r * cbmp->bpl_c;
            r = r / s;
            g = g / s;
            b = b / s;
            clr = near(r,g,b);
#if    FULLCOLOR
            printf("\x1b[0;48;2;%2u:%2u:%2um ", r,g,b);
            (void)(clr);
#else  /* FULLCOLOR */
            printf("\x1b[0;48;5;%um ", clr);
#endif /* FULLCOLOR */
        }
        printf("\x1b[39m\x1b[49m"); // デフォルトに戻す
        printf("\n");
    }
}

