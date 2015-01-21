#include "micomfs_dev.h"

/* 以下デバイス依存コード */
#define _FILE_OFFSET_BITS 64
#include <sys/stat.h>
#include <stdio.h>
#ifdef __MINGW32__
#include <windows.h>
#include <math.h>
#endif

#define WIN_SECTOR_READ_ACCESS_SIZE ( 512 * 128 )
#define WIN_INVALID_SECTOR 0xFFFFFFFF
/*
 * 無効セクターをこのような方法で（セクター番号の最大値）指定しているので
 * Windowsのディスクの最終セクターにはアクセスできないという仕様が生じます．
 * 実用的にはそこまでおおきなファイルを扱うことはないと思うので問題ないと思います．
 *
 */

/*
 * Windowsのセクターアクセスも境界でセクタサイズしか許されていない
 */

/*
 * WindowsのAPIを使って512バイト毎にセクターアクセスを行うとあまりに遅いので
 * 読み込みの場合はWIN_SECTOR_READ_ACCESS_SIZE Bytes一度にバッファへ読み込み，続くアクセスが読み込んだ範囲内なら
 * 新たなディスクアクセスを実行しません．
 * これは読み込みの場合だけで，書き込みはバッファリングされません．
 *
 */


static char sbuf[512];
static char largeBuf[WIN_SECTOR_READ_ACCESS_SIZE];

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

    fs->dev_current_sector = 0;
    fs->dev_current_spos   = 0;

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
            mode = "r+";
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

        /* 現在のセクターに最大セクター番号を指定してアクセスしていないことを示す */
        fs->dev_current_sector = WIN_INVALID_SECTOR;
        fs->dev_current_spos   = 0;

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

        /* おかしなセクタ番号では失敗 */
        if ( sector >= fs->dev_sector_count || sector == WIN_INVALID_SECTOR ) {
            return 0;
        }

        /* 書き込みアドレス決定 */
        address = (uint64_t)sector * fs->dev_sector_size;

        /* Windowsでの書き込みはバッファリングしないので現在のセクターに書き込みが開始されたフラグとして無効セクター番号を指定 */
        fs->dev_current_sector = WIN_INVALID_SECTOR;
        fs->dev_current_spos   = 0;

        SetFilePointer( *( (HANDLE *)fs->device ), address, (PLONG)( ( (char *)&address ) + 4 ), FILE_BEGIN );
#endif
        break;
    }

    default:
        return 0;
        break;
    }

    /* セクター内位置を0に */
    fs->dev_current_spos = 0;

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
        memcpy( sbuf + fs->dev_current_spos, src, count );

        /* 進める */
        fs->dev_current_spos += count;
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
        DWORD readSize;

        /* おかしなセクタ番号では失敗 */
        if ( sector >= fs->dev_sector_count || sector == WIN_INVALID_SECTOR ) {
            return 0;
        }

        /* 前回読み込み成功していてかつ4k範囲内なら新たに読み込まない */
        if ( fs->dev_current_sector != WIN_INVALID_SECTOR &&
             ( ( (int64_t)sector - (int64_t)fs->dev_current_sector ) < ( WIN_SECTOR_READ_ACCESS_SIZE / 512 ) &&
               ( (int64_t)sector - (int64_t)fs->dev_current_sector ) >= 0 ) ) {
            /* 前回読み込み分に該当 */
            memcpy( sbuf, largeBuf + ( sector - fs->dev_current_sector ) * 512, 512 );
        } else {
            /* 新たなセクターアクセスが必要 */

            /* 指定ブロックでセクターを丸める */
            fs->dev_current_sector = sector - sector % ( WIN_SECTOR_READ_ACCESS_SIZE / 512 );

            /* 読み込むべきサイズを決定 */
            if ( ( fs->dev_sector_count - sector ) < ( WIN_SECTOR_READ_ACCESS_SIZE / 512 ) ) {
                /* 読み込むセクターが足りないのでサイズを減らす */
                readSize = ( fs->dev_sector_count - sector ) * 512;
            } else {
                readSize = WIN_SECTOR_READ_ACCESS_SIZE;
            }

            /* アドレス決定 */
            address = (uint64_t)fs->dev_current_sector * fs->dev_sector_size;

            /* FP移動 */
            SetFilePointer( *( (HANDLE *)fs->device ), address, (PLONG)( ( (char *)&address ) + 4 ), FILE_BEGIN );

            /* 読み込み */
            ReadFile( *( (HANDLE *)fs->device ), largeBuf, readSize, &dw, NULL );

            /* セクタ単位読み込みバッファにブロック単位バッファの最初のセクターをコピー */
            memcpy( sbuf, largeBuf + ( sector - fs->dev_current_sector ) * 512, 512 );

            /* エラーチェック */
            if ( dw != sizeof( largeBuf ) ) {
                fs->dev_current_sector = WIN_INVALID_SECTOR;

                return 0;
            }
        }
#endif
        break;
    }

    default:
        return 0;
        break;
    }

    /* セクター内位置を0に */
    fs->dev_current_spos = 0;

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
        memcpy( dest, sbuf + fs->dev_current_spos, count );

        /* 進める */
        fs->dev_current_spos += count;
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
