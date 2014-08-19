#include "micomfs_dev.h"

/* 以下デバイス依存コード */
#define _FILE_OFFSET_BITS 64
#include <sys/stat.h>
#include <stdio.h>
#ifdef __MINGW32__
#include <windows.h>
#endif

/*
 * Windowsのセクターアクセスも境界でセクタサイズしか許されていない
 */

static char sbuf[512];
static uint16_t spos;

char micomfs_dev_get_info( MicomFS *fs, uint16_t *sector_size, uint32_t *sector_count )
{
    /* ファイルシステムに必要な情報を返す */
    switch ( fs->dev_type ) {
    case MicomFSDeviceFile: {
        struct stat buf;

        /* ファイル情報取得 */
        stat( fs->dev_name, &buf );

        *sector_size  = 512;
        *sector_count = buf.st_size / *sector_size;

        break;
    }
    case MicomFSDeviceWinDrive: {
#ifdef __MINGW32__
        HANDLE handle;
        DISK_GEOMETRY_EX dgex;
        DWORD dw;

        handle = *( (HANDLE *)fs->device );

        /* デバイスの情報取得 */
        DeviceIoControl( handle,
                         IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                         NULL,
                         0,
                         (LPVOID) &dgex,
                         (DWORD) sizeof( dgex ),
                         (LPDWORD) &dw,
                         NULL );

        /* セクター数とセクターサイズ取得 */
        *sector_size  = dgex.Geometry.BytesPerSector;
        *sector_count = dgex.DiskSize.QuadPart / *sector_size;
#endif
        break;
    }
    default:
        break;
    }

    return 1;
}

char micomfs_dev_open( MicomFS *fs, const char *dev_name, MicomFSDeviceType dev_type, MicomFSDeviceMode dev_mode )
{
    /* デバイスを開く */
    FILE **fpp;
    const char *mode;

    /* init pointers */
    fs->dev_name = NULL;
    fs->device   = NULL;

    /* for PC ファイルを開く */
    fs->dev_type = dev_type;
    fs->dev_name = malloc( sizeof( char ) * 1024 );

    /* Open the device */
    switch ( dev_type ) {
    case MicomFSDeviceFile:
        /* Normal ( or device ) file */
        fs->device = malloc( sizeof( FILE * ) );
        strcpy( fs->dev_name, dev_name );

        fpp = (FILE **)fs->device;

        /* Set access mode */
        switch ( dev_mode ) {
        case MicomFSDeviceModeRead:
            mode = "r";
            break;

        case MicomFSDeviceModeReadWrite:
            mode = "rw";
            break;

        case MicomFSDeviceModeWrite:
            mode = "w";
            break;

        default:
            mode = "r";
            break;
        }

        if ( ( *fpp = fopen( dev_name, mode ) ) == NULL ) {
            /* Failed */
            free( fs->dev_name );
            free( fs->device );

            return 0;
        }

        break;

    case MicomFSDeviceWinDrive: {
        /* Window's logical drive letter */
#ifdef __MINGW32__
        HANDLE *handle;
        DWORD mode;

        fs->device = malloc( sizeof( HANDLE ) );
        handle = (HANDLE *)fs->device;

        wcscpy( (wchar_t *)fs->dev_name, (wchar_t*)dev_name );

        /* Set access mode */
        switch ( dev_mode ) {
        case MicomFSDeviceModeRead:
            mode = GENERIC_READ;
            break;

        case MicomFSDeviceModeReadWrite:
            mode = GENERIC_READ | GENERIC_WRITE;
            break;

        case MicomFSDeviceModeWrite:
            mode = GENERIC_WRITE;
            break;

        default:
            mode = GENERIC_READ;
            break;
        }

        /* Create file */
        *handle = CreateFileW( (LPCWSTR)dev_name,
                               mode,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL );

        /* Check error */
        if ( *handle == INVALID_HANDLE_VALUE ) {
            free( fs->dev_name );
            free( fs->device );

            fs->dev_name = NULL;
            fs->device   = NULL;

            return 0;
        }

        break;
#else
        free( fs->dev_name );
        fs->dev_name = NULL;
        return 0;
        break;
#endif
    }

    default:
        free( fs->dev_name );
        fs->dev_name = NULL;
        return 0;
        break;
    }

    return 1;
}

char micomfs_dev_close( MicomFS *fs )
{
    /* デバイスを閉じる */
    if ( fs->device != NULL ) {
        switch ( fs->dev_type ) {
        case MicomFSDeviceFile:
            fclose( *(FILE **)fs->device );
            break;

        case MicomFSDeviceWinDrive:
#ifdef __MINGW32__
            CloseHandle( *( (HANDLE *)fs->device ) );
#endif
            break;

        default:
            break;
        }
    }

    free( fs->dev_name );
    free( fs->device );

    return 1;
}

char micomfs_dev_start_write( MicomFS *fs, uint32_t sector )
{
    /* セクターライト開始 */
    switch ( fs->dev_type ) {
    case MicomFSDeviceFile: {
        uint64_t address;
        FILE *fp = *(FILE **)fs->device;

        /* アドレス作成 */
        address = (uint64_t)sector * fs->dev_sector_size;

        fs->dev_current_sector = sector;
        fs->dev_current_spos   = 0;

        /* 移動 */
        fseeko( fp, address, SEEK_SET );

        break;
    }

    case MicomFSDeviceWinDrive: {
#ifdef __MINGW32__
        /* 移動 */
        uint64_t address;

        address = (uint64_t)sector * fs->dev_sector_size;

        SetFilePointer( *( (HANDLE *)fs->device ), address, (PLONG)( ( (char *)&address ) + 4 ), FILE_BEGIN );
#endif
        break;
    }

    default:
        return 0;
        break;
    }

    /* セクター内位置を0に */
    spos = 0;

    return 1;
}

char micomfs_dev_write( MicomFS *fs, const void *src, uint16_t count )
{
    /* 書き込み */
    switch ( fs->dev_type ) {
    case MicomFSDeviceFile: {
        FILE *fp = *(FILE **)fs->device;

        if ( fwrite( src, 1, count, fp ) != count ) {
            return 0;
        }

        break;
    }

    case MicomFSDeviceWinDrive: {
#ifdef __MINGW32__
        /* sbufへコピー */
        memcpy( sbuf + spos, src, count );

        /* 進める */
        spos += count;
#endif
        break;
    }

    default:
        break;
    }

    return 1;
}

char micomfs_dev_stop_write( MicomFS *fs )
{
    /* セクターライト終了 */


    switch ( fs->dev_type ) {
    case MicomFSDeviceFile: {
        break;
    }

    case MicomFSDeviceWinDrive: {
#ifdef __MINGW32__
        DWORD dw;

        /* 書き込み */
        WriteFile( *( (HANDLE *)fs->device ), sbuf, sizeof( sbuf ), &dw, NULL );

        /* エラーチェック */
        if ( dw != sizeof( sbuf ) ) {
            return 0;
        }
#endif
        break;
    }

    default:
        break;
    }

    return 1;
}

char micomfs_dev_start_read( MicomFS *fs, uint32_t sector )
{
    /* セクターリード開始 */
    switch ( fs->dev_type ) {
    case MicomFSDeviceFile: {
        uint64_t address;
        FILE *fp = *(FILE **)fs->device;

        /* アドレス作成 */
        address = (uint64_t)sector * fs->dev_sector_size;

        fs->dev_current_sector = sector;
        fs->dev_current_spos   = 0;

        /* 移動 */
        if ( fseeko( fp, address, SEEK_SET ) ) {
            return 0;
        }

        break;
    }

    case MicomFSDeviceWinDrive: {
#ifdef __MINGW32__
        /* 移動 */
        uint64_t address;
        DWORD dw;

        address = (uint64_t)sector * fs->dev_sector_size;

        SetFilePointer( *( (HANDLE *)fs->device ), address, (PLONG)( ( (char *)&address ) + 4 ), FILE_BEGIN );

        /* 読み込み */
        ReadFile( *( (HANDLE *)fs->device ), sbuf, sizeof( sbuf ), &dw, NULL );

        /* エラーチェック */
        if ( dw != sizeof( sbuf ) ) {
            return 0;
        }
#endif
        break;
    }

    default:
        return 0;
        break;
    }

    /* セクター内位置を0に */
    spos = 0;

    return 1;
}

char micomfs_dev_read( MicomFS *fs, void *dest, uint16_t count )
{
    /* 読み込み */
    switch ( fs->dev_type ) {
    case MicomFSDeviceFile: {
        FILE *fp = *(FILE **)fs->device;

        if ( fread( dest, 1, count, fp ) != count ) {
            return 0;
        }

        break;
    }

    case MicomFSDeviceWinDrive: {
#ifdef __MINGW32__
        /* sbufからコピー */
        memcpy( dest, sbuf + spos, count );

        /* 進める */
        spos += count;
#endif
        break;
    }

    default:
        break;
    }

    return 1;
}

char micomfs_dev_stop_read( MicomFS *fs )
{
    /* セクターリード終了 */
    return 1;
}
