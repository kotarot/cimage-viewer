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
    //showbmpdata(pix, ih.width, ih.height);
#endif

    // ファイルクローズ
    fclose(fp);
#ifdef DEBUG
    printf("[FILECLOSE: OK]\n");
#endif

    // コンソールサイズ取得
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
#ifdef DEBUG
    printf("[CONSOLESIZE: OK] col=%u,row=%u\n", win.ws_col, win.ws_row);
#endif

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
#ifdef DEBUG
    printf("[BMP/LETTER: OK] bpl_c=%u,bpl_r=%u,letter=%u,line=%u\n", cbmp.bpl_c, cbmp.bpl_r, cbmp.letter, cbmp.line);
#endif

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
#ifdef DEBUG
    printf("[MEMORYFREE: OK]\n");
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
    printf("--file header--\n");
    printf("type: %c%c\n", (char)fh->type, (char)(fh->type >> 8));
    printf("size: %u\n", fh->size);
    printf("reserved1: %u\n", fh->reserved1);
    printf("reserved1: %u\n", fh->reserved2);
    printf("offbits: %u\n", fh->offbits);
    //printf("----\n");
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
    //printf("----\n");
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

// 拡張パレット - RBGの変換テーブル
pixel_t pal2rgb[256] = {
{0,0,0},      {192,0,0},    {0,192,0},    {192,192,0},  
{64,64,192},  {192,0,192},  {0,192,192},  {192,192,192},
{64,64,64},   {255,0,0},    {0,255,0},    {255,255,0},  
{128,128,255},{255,0,255},  {0,255,255},  {255,255,255},
{0,0,0},      {0,0,95},     {0,0,135},    {0,0,175},    
{0,0,215},    {0,0,255},    {0,95,0},     {0,95,95},    
{0,95,135},   {0,95,175},   {0,95,215},   {0,95,255},   
{0,135,0},    {0,135,95},   {0,135,135},  {0,135,175},  
{0,135,215},  {0,135,255},  {0,175,0},    {0,175,95},   
{0,175,135},  {0,175,175},  {0,175,215},  {0,175,255},  
{0,215,0},    {0,215,95},   {0,215,135},  {0,215,175},  
{0,215,215},  {0,215,255},  {0,255,0},    {0,255,95},   
{0,255,135},  {0,255,175},  {0,255,215},  {0,255,255},  
{95,0,0},     {95,0,95},    {95,0,135},   {95,0,175},   
{95,0,215},   {95,0,255},   {95,95,0},    {95,95,95},   
{95,95,135},  {95,95,175},  {95,95,215},  {95,95,255},  
{95,135,0},   {95,135,95},  {95,135,135}, {95,135,175}, 
{95,135,215}, {95,135,255}, {95,175,0},   {95,175,95},  
{95,175,135}, {95,175,175}, {95,175,215}, {95,175,255}, 
{95,215,0},   {95,215,95},  {95,215,135}, {95,215,175}, 
{95,215,215}, {95,215,255}, {95,255,0},   {95,255,95},  
{95,255,135}, {95,255,175}, {95,255,215}, {95,255,255}, 
{135,0,0},    {135,0,95},   {135,0,135},  {135,0,175},  
{135,0,215},  {135,0,255},  {135,95,0},   {135,95,95},  
{135,95,135}, {135,95,175}, {135,95,215}, {135,95,255}, 
{135,135,0},  {135,135,95}, {135,135,135},{135,135,175},
{135,135,215},{135,135,255},{135,175,0},  {135,175,95}, 
{135,175,135},{135,175,175},{135,175,215},{135,175,255},
{135,215,0},  {135,215,95}, {135,215,135},{135,215,175},
{135,215,215},{135,215,255},{135,255,0},  {135,255,95}, 
{135,255,135},{135,255,175},{135,255,215},{135,255,255},
{175,0,0},    {175,0,95},   {175,0,135},  {175,0,175},  
{175,0,215},  {175,0,255},  {175,95,0},   {175,95,95},  
{175,95,135}, {175,95,175}, {175,95,215}, {175,95,255}, 
{175,135,0},  {175,135,95}, {175,135,135},{175,135,175},
{175,135,215},{175,135,255},{175,175,0},  {175,175,95}, 
{175,175,135},{175,175,175},{175,175,215},{175,175,255},
{175,215,0},  {175,215,95}, {175,215,135},{175,215,175},
{175,215,215},{175,215,255},{175,255,0},  {175,255,95}, 
{175,255,135},{175,255,175},{175,255,215},{175,255,255},
{215,0,0},    {215,0,95},   {215,0,135},  {215,0,175},  
{215,0,215},  {215,0,255},  {215,95,0},   {215,95,95},  
{215,95,135}, {215,95,175}, {215,95,215}, {215,95,255}, 
{215,135,0},  {215,135,95}, {215,135,135},{215,135,175},
{215,135,215},{215,135,255},{215,175,0},  {215,175,95}, 
{215,175,135},{215,175,175},{215,175,215},{215,175,255},
{215,215,0},  {215,215,95}, {215,215,135},{215,215,175},
{215,215,215},{215,215,255},{215,255,0},  {215,255,95}, 
{215,255,135},{215,255,175},{215,255,215},{215,255,255},
{255,0,0},    {255,0,95},   {255,0,135},  {255,0,175},  
{255,0,215},  {255,0,255},  {255,95,0},   {255,95,95},  
{255,95,135}, {255,95,175}, {255,95,215}, {255,95,255}, 
{255,135,0},  {255,135,95}, {255,135,135},{255,135,175},
{255,135,215},{255,135,255},{255,175,0},  {255,175,95}, 
{255,175,135},{255,175,175},{255,175,215},{255,175,255},
{255,215,0},  {255,215,95}, {255,215,135},{255,215,175},
{255,215,215},{255,215,255},{255,255,0},  {255,255,95}, 
{255,255,135},{255,255,175},{255,255,215},{255,255,255},
{8,8,8},      {18,18,18},   {28,28,28},   {38,38,38},   
{48,48,48},   {58,58,58},   {68,68,68},   {78,78,78},   
{88,88,88},   {98,98,98},   {108,108,108},{118,118,118},
{128,128,128},{138,138,138},{148,148,148},{158,158,158},
{168,168,168},{178,178,178},{188,188,188},{198,198,198},
{208,208,208},{218,218,218},{228,228,228},{238,238,238},
};


// 三次空間から距離を得る
uint32_t distance(uint32_t r0, uint32_t g0, uint32_t b0,
                  uint32_t r1, uint32_t g1, uint32_t b1) {
    uint32_t r, g, b;
    r = abs(r0 - r1) ;
    g = abs(g0 - g1) ;
    b = abs(b0 - b1) ;
    return sqrt(r * r + g * g + b * b);
}

// RGB から 拡張カラーへの近似色を探す
uint8_t near(uint32_t r0, uint32_t g0, uint32_t b0) {
	uint32_t dmin = (uint32_t)1000 ;// > 441.672
	uint32_t d[256] = {0};
	uint32_t i;
    for (i = 0; i < 256; i++) {
		d[i] = distance( r0,g0,b0,
		pal2rgb[i].red,
		pal2rgb[i].green,
		pal2rgb[i].blue);
		if(dmin > d[i]) dmin = d[i];
	}
	for (i = 0; i < 256; i++) {
		if(d[i] == dmin) return i;
	}
	return 255;
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
#ifdef DEBUG
            printf("\x1b[0;48;5;%um%02x", clr, clr);
#else
            printf("\x1b[0;48;5;%um ", clr);
#endif
        }
        printf("\x1b[39m\x1b[49m"); // デフォルトに戻す
        printf("\n");
    }
}

