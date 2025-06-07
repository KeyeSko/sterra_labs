#include <stdio.h>
#include <stdint.h>

#define ZIP_END_SIG 0x06054B50
#define CD_FILE_HEADER_SIG 0x02014B50

typedef struct {
    // Обязательная сигнатура, равна 0x06054b50
    uint32_t signature;
    // Номер диска
    uint16_t diskNumber;
    // Номер диска, где находится начало Central Directory
    uint16_t startDiskNumber;
    // Количество записей в Central Directory в текущем диске
    uint16_t numberCentralDirectoryRecord;
    // Всего записей в Central Directory
    uint16_t totalCentralDirectoryRecord;
    // Размер Central Directory
    uint32_t sizeOfCentralDirectory;
    // Смещение Central Directory
    uint32_t centralDirectoryOffset;
    // Длина комментария
    uint16_t commentLength;
    // Комментарий (длиной commentLength)
    //uint8_t *comment;
} __attribute__((__packed__)) EOCD;

typedef struct {
    // Обязательная сигнатура, равна 0x02014b50
    uint32_t signature;
    // Версия для создания
    uint16_t versionMadeBy;
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
    // Длина комментариев к файлу
    uint16_t fileCommentLength;
    // Номер диска
    uint16_t diskNumber;
    // Внутренние аттрибуты файла
    uint16_t internalFileAttributes;
    // Внешние аттрибуты файла
    uint32_t externalFileAttributes;
    // Смещение до структуры LocalFileHeader
    uint32_t localFileHeaderOffset;
    // Имя файла (длиной filenameLength)
    //uint8_t *filename;
    // Дополнительные данные (длиной extraFieldLength)
    //uint8_t *extraField;
    // Комментарий к файла (длиной fileCommentLength)
    //uint8_t *fileComment;
} __attribute__((__packed__)) CentralDirectoryFileHeader;

int find_zip_end(FILE *file, long *end_offset) {
    fseek(file, -22, SEEK_END);
    *end_offset = ftell(file);

    uint32_t signature;
    while (*end_offset > 0) {
        fread(&signature, sizeof(signature), 1, file);
        if (signature == ZIP_END_SIG) {
            fseek(file, -4, SEEK_CUR);
            return 1;
        }
        fseek(file, -5, SEEK_CUR);
        (*end_offset)--;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    long zip_offset;
    if (find_zip_end(file, &zip_offset)) {
        printf("Zip archive found!\nFiles in archive:\n");
        fseek(file, zip_offset, SEEK_SET);
        EOCD zip_end;
        if (fread(&zip_end, sizeof(zip_end), 1, file) != 1)
            return 1;

        if (zip_end.signature != ZIP_END_SIG) {
            perror("Invalid EOCD signature");
            fclose(file);
            return 1;
        }
        if (zip_end.diskNumber != 0 || zip_end.startDiskNumber != 0 || zip_end.numberCentralDirectoryRecord != zip_end.totalCentralDirectoryRecord) {
            perror("Invalid EOCD values");
            fclose(file);
            return 1;
        }

        long cd_absolute_offset = zip_offset - zip_end.sizeOfCentralDirectory;
        if (cd_absolute_offset < 0) {
            fprintf(stderr, "Invalid Central Directory offset.\n");
            fclose(file);
            return 1;
        }

        fseek(file, cd_absolute_offset, SEEK_SET);
        for (int i = 0; i < zip_end.totalCentralDirectoryRecord; i++) {
            char filename[256];
            CentralDirectoryFileHeader cd_header;

            if (fread(&cd_header, sizeof(cd_header), 1, file) != 1)
                return 1;
            if (cd_header.signature != CD_FILE_HEADER_SIG) {
                perror("Invalid Central Directory File Header signature");
                fclose(file);
                return 1;
            }
            if (cd_header.filenameLength > 255) {
                perror("Filename too long");
                fclose(file);
                return 1;
            }

            fread(filename, 1, cd_header.filenameLength, file);
            filename[cd_header.filenameLength] = '\0';
            printf("%s\n", filename);

            fseek(file, cd_header.extraFieldLength + cd_header.fileCommentLength, SEEK_CUR);
        }
    } else {
        printf("No zip archive found.\n");
    }

    fclose(file);
    return 0;
}

