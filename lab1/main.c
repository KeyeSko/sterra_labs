#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#pragma pack(push, 1)
struct LocalFileHeader
{
    // Обязательная сигнатура, равна 0x04034b50
    uint32_t signature;
    // Минимальная версия для распаковки
    uint16_t versionToExtract;
    // Битовый флаг
    uint16_t generalPurposeBitFlag;
    // Метод сжатия (0 - без сжатия, 8 - deflate)
    uint16_t compressionMethod;
    // Время модификации файла
    uint16_t modificationTime;
    // Дата модификации файла
    uint16_t modificationDate;
    // Контрольная сумма
    uint32_t crc32;
    // Сжатый размер
    uint32_t compressedSize;
    // Несжатый размер
    uint32_t uncompressedSize;
    // Длина название файла
    uint16_t filenameLength;
    // Длина поля с дополнительными данными
    uint16_t extraFieldLength;
    // Название файла (размером filenameLength)
    //uint8_t *filename;
    // Дополнительные данные (размером extraFieldLength)
    //uint8_t *extraField;
};
#pragma pack(pop)

int main(int argc, char *argv[]){
    
    int signature[4] = { 0x50, 0x4b, 0x03, 0x04};
    int ch = 0;
    char fileName[255] = {0};
    int flag = 0;
    int i = 0;

    if(argc == 1){
    	printf("Input filename\n");
    }

    FILE *fp = fopen(argv[1], "rb");
    if(fp == NULL){
        perror("main");
        return 0;
    }
    
    struct LocalFileHeader readFile;

    while ((ch = fgetc(fp)) != EOF){
        if (ch == signature[i]){
            i++;
            if (i == 4){
                flag = 1;
                i = 0;
                fseek(fp, -4, SEEK_CUR);
                if (1 != fread(&readFile, sizeof(readFile), 1, fp))
                    return 0;
                if (1 != fread(fileName, readFile.filenameLength, 1, fp))
                    return 0;
                fileName[readFile.filenameLength] = 0;
                printf("fileName: %s\n", fileName);
            }
        } else {
            i = (ch == signature[0])? 1 : 0;
        }
    }

    if(!flag)
        printf("File doesn't have zip\n");

    fclose(fp);
    return 0;
}
