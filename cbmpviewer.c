/**
 * CBmpViewer
 * コンソール上でBMP画像を表示するプログラム
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cbmpviewer.h"

// メイン関数
int main(int argc, char *argv[]) {
    // 引数チェック
    if (argc != 2) {
        usage();
        return EXIT_SUCCESS;
    }
    // argv[1]: 画像ファイル名

    // Viewプロシージャへ
    viewproc(argv[1]);

    return EXIT_SUCCESS;
}

// Usage
void usage(void) {
    printf("** CBmpViewer **\n");
    printf("Usage: `cbmpviewer [input.bmp]`\n");
}

// Viewプロシージャ
void viewproc(char *filename) {
    FILE *fp;
    bmpfileheader_t fh;
    bmpinfoheader_t ih;
    pixel_t **pix;
    int i;

    // 画像ファイルオープン
    if ((fp = fopen(filename, "rb")) == NULL) {
        printf("Error: file open\n");
        exit(EXIT_FAILURE);
    }
#ifdef DEBUG
    printf("[FILEOPEN: OK]\n");
#endif

    // 画像ヘッダ取得
    getbmpheader(fp, &fh, &ih);
#ifdef DEBUG
    showbmpheader(&fh, &ih);
#endif
    // 画像ヘッダの(このプログラムで対応する)フォーマットチェック
    checkbmpheader(&fh, &ih);
#ifdef DEBUG
    printf("[FORMAT: OK]\n");
#endif

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
#ifdef DEBUG
    printf("[MEMORYALLOC: OK]\n");
#endif

    // 画像データをメモリに読み込む
    readbmpdata(fp, pix, ih.width, ih.height);
#ifdef DEBUG
    printf("[READDATA: OK]\n");
#endif
#ifdef DEBUG
    showbmpdata(pix, ih.width, ih.height);
#endif

    

    // メモリ解放
    for (i = 0; i < ih.height; i++) {
        free(pix[i]);
    }
    free(pix);
#ifdef DEBUG
    printf("[MEMORYFREE: OK]\n");
#endif

    // ファイルクローズ
    fclose(fp);
#ifdef DEBUG
    printf("[FILECLOSE: OK]\n");
#endif
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
    printf("--file header--\n");
    printf("type: %c%c\n", (char)fh->type, (char)(fh->type >> 8));
    printf("size: %u\n", fh->size);
    printf("reserved1: %u\n", fh->reserved1);
    printf("reserved1: %u\n", fh->reserved2);
    printf("offbits: %u\n", fh->offbits);
    printf("----");
    printf("--info header--\n");
    printf("size: %u\n", ih->size);
    printf("width: %d\n", ih->width);
    printf("height: %d\n", ih->height);
    printf("planes: %u\n", ih->planes);
    printf("bitcount: %u\n", ih->bitcount);
    printf("compression: %u\n", ih->compression);
    printf("sizeimage: %u\n", ih->sizeimage);
    printf("xpixpermeter: %d\n", ih->xpixpermeter);
    printf("ypixpermeter: %d\n", ih->ypixpermeter);
    printf("clrused: %u\n", ih->clrused);
    printf("clrimporant: %u\n", ih->clrimporant);
    printf("----\n");
}

// 画像データをメモリに読み込む
void readbmpdata(FILE *fp, pixel_t **pix, int32_t w, int32_t h) {
    int i, j;

    for (i = 0; i < h; i++) {
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
