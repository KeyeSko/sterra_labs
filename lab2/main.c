#include <stdio.h>
#include <string.h>

enum {
  UNKNOWN_TYPE = 0,
  CP_1251,
  KOI8_R,
  ISO_8859_5,
};

int getEncodingType(char* encoding){
    if(strcmp(encoding, "CP-1251") == 0)
      return CP_1251;
    if(strcmp(encoding, "KOI8-R") == 0)
      return KOI8_R;
    if(strcmp(encoding, "ISO-8859-5") == 0)
      return ISO_8859_5;
    return UNKNOWN_TYPE;
    }

const int utf8_codes[][2] = {
    {0xD1, 0x8E}, // ю
    {0xD0, 0xB0}, // а
    {0xD0, 0xB1}, // б
    {0xD1, 0x86}, // ц
    {0xD0, 0xB4}, // д
    {0xD0, 0xB5}, // е
    {0xD1, 0x84}, // ф
    {0xD0, 0xB3}, // г
    {0xD1, 0x85}, // х
    {0xD0, 0xB8}, // и
    {0xD0, 0xB9}, // й
    {0xD0, 0xBA}, // к
    {0xD0, 0xBB}, // л
    {0xD0, 0xBC}, // м
    {0xD0, 0xBD}, // н
    {0xD0, 0xBE}, // о
    {0xD0, 0xBF}, // п
    {0xD1, 0x8F}, // я
    {0xD1, 0x80}, // р
    {0xD1, 0x81}, // с
    {0xD1, 0x82}, // т
    {0xD1, 0x83}, // у
    {0xD0, 0xB6}, // ж
    {0xD0, 0xB2}, // в
    {0xD1, 0x8C}, // ь
    {0xD1, 0x8B}, // ы
    {0xD0, 0xB7}, // з
    {0xD1, 0x88}, // ш
    {0xD1, 0x8D}, // э
    {0xD1, 0x89}, // щ
    {0xD1, 0x87}, // ч
    {0xD1, 0x8A}  // ъ
};


void koi8rToUtf8(FILE* infp, FILE* outfp) {
    int ch;
    while ((ch = fgetc(infp)) != EOF) {
        if (ch < 0x80) {
            fputc(ch, outfp);
        }
        else{
            if(191 < ch && ch < 224){
                fputc(utf8_codes[ch - 192][0], outfp);
                fputc(utf8_codes[ch - 192][1], outfp);
            }
            else{
                fputc(208, outfp);
                if(utf8_codes[ch-224][0] == 209)
                    fputc(utf8_codes[ch - 224][1] + 32, outfp);
                else
                    fputc(utf8_codes[ch - 224][1] - 32, outfp);
            }
        }
    }
}

void cp1251ToUtf8(FILE* infp, FILE* outfp){
    int ch;
    while((ch = fgetc(infp)) != EOF){
        if(ch < 160){
            fputc(ch, outfp);
        }
        if(191 < ch && ch < 240){
            fputc(208, outfp);
            fputc(ch - 48, outfp);
            continue;
        }
        if(239 < ch && ch < 256){
            fputc(209, outfp);
            fputc(ch - 112, outfp);
        }
        if(ch == 168){
            fputc(208, outfp);
            fputc(129, outfp);
            continue;
        }
        if(ch == 184){
            fputc(209, outfp);
            fputc(145, outfp);
            continue;
        }
    }
}

void iso88595ToUtf8(FILE* infp, FILE* outfp){
    int ch;
    while((ch = fgetc(infp)) != EOF){
	if(ch < 160){
            fputc(ch, outfp);
        }
        if(175 < ch && ch < 224){
            fputc(208, outfp);
            fputc(ch - 32, outfp);
            continue;
        }
        if(223 < ch && ch < 240){
            fputc(209, outfp);
            fputc(ch - 96, outfp);
        }
        if(ch == 161){
            fputc(208, outfp);
            fputc(129, outfp);
            continue;
        }
        if(ch == 241){
            fputc(209, outfp);
            fputc(145, outfp);
            continue;
        }
    }
}

int main(int argc, char *argv[]){
    int encType;

    if(argc != 4){
        printf("Usage: ./main InputFile Encoding OutputFile\n");
        printf("Encoding types: CP-1251, KOI8-R, ISO-8859-5\n");
        return 1;
    }

    if((encType = getEncodingType(argv[2])) == 0){
        printf("Unknown encoding. Please enter one of these: CP-1251, KOI8-R, ISO-8859-5\n");
        return 1;
    }


    FILE *infp = fopen(argv[1], "rb");
    if(infp == NULL){
        perror("main");
        return 1;
    }


    FILE *outfp = fopen(argv[3], "wb");
    if(outfp == NULL){
        perror("main");
        return 1;
    }

    switch (encType) {
        case CP_1251:
            cp1251ToUtf8(infp, outfp);
            break;
        case KOI8_R:
            koi8rToUtf8(infp, outfp);
            break;
        case ISO_8859_5:
            iso88595ToUtf8(infp, outfp);
            break;
    }

    fclose(infp);
    fclose(outfp);

    return 0;
}
