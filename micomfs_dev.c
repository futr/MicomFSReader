#include "micomfs_dev.h"
#include <sys/stat.h>

char micomfs_dev_get_info( MicomFS *fs, uint16_t *sector_size, uint32_t *sector_count )
{
    /* ファイルシステムに必要な情報を返す */
    struct stat buf;

    /* ファイル情報取得 */
    stat( fs->dev_name, &buf );

    *sector_size  = 512;
    *sector_count = buf.st_size / *sector_size;

    return 1;
}

char micomfs_dev_start_write( MicomFS *fs, uint32_t sector )
{
    /* セクターライト開始 */
    uint32_t address;
    FILE *fp = *(FILE **)fs->device;

    /* アドレス作成 */
    address = sector * fs->dev_sector_size;

    fs->dev_current_sector = address;
    fs->dev_current_spos   = 0;

    /* 移動 */
    fseek( fp, address, SEEK_SET );

    return 1;
}

char micomfs_dev_write( MicomFS *fs, const void *src, uint16_t count )
{
    /* 1バイト書き込み */
    FILE *fp = *(FILE **)fs->device;

    fwrite( src, 1, count, fp );

    return 1;
}

char micomfs_dev_stop_write( MicomFS *fs )
{
    /* セクターライト終了 */
    return 1;
}

char micomfs_dev_start_read( MicomFS *fs, uint32_t sector )
{
    /* セクターリード開始 */
    FILE *fp = *(FILE **)fs->device;
    uint32_t address;

    /* アドレス作成 */
    address = sector * fs->dev_sector_size;

    fs->dev_current_sector = address;
    fs->dev_current_spos   = 0;

    /* 移動 */
    fseek( fp, address, SEEK_SET );

    return 1;
}

char micomfs_dev_read( MicomFS *fs, void *dest, uint16_t count )
{
    /* 1バイト読み込み */
    FILE *fp = *(FILE **)fs->device;

    fread( dest, 1, count, fp );

    return 1;
}

char micomfs_dev_stop_read( MicomFS *fs )
{
    /* セクターリード終了 */
    return 1;
}
