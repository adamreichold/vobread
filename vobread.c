/*

Copyright 2021 Adam Reichold

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

*/

#include <dvdread/ifo_read.h>
#include <dvdread/ifo_print.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

static void stderr_logger(void* /* priv */, dvd_logger_level_t /* level */, const char* fmt, va_list list) {
    fprintf(stderr, "libdvdread: ");
    vfprintf(stderr, fmt, list);
    fprintf(stderr, "\n");
}

static const dvd_logger_cb logger = {
    .pf_log = stderr_logger,
};

int main(int argc, char** argv) {
    int status = EXIT_FAILURE;

    int opt;
    const char* device = "/dev/dvd";
    int buffer_size = 64 * 1024;
    int title = 0;
    bool info = false;

    dvd_reader_t* dvd = NULL;
    ifo_handle_t* ifo = NULL;
    dvd_file_t* dvd_file = NULL;

    int offset = 0;
    unsigned char* buffer = NULL;
    ssize_t blocks_total;
    ssize_t blocks_read;
    ssize_t bytes_written;

    while ((opt = getopt(argc, argv, "d:b:t:ih")) != -1) {
        switch (opt) {
        case 'd':
            device = optarg;
            break;
        case 'b':
            buffer_size = atoi(optarg);
            if (buffer_size < DVD_VIDEO_LB_LEN) {
                fprintf(stderr, "Invalid buffer size `%s`\n", optarg);
                goto out;
            }
            break;
        case 't':
            title = atoi(optarg);
            if (title < 0) {
                fprintf(stderr, "Invalid title `%s`\n", optarg);
                goto out;
            }
            break;
        case 'i':
            info = true;
            break;
        default:
        case 'h':
            fprintf(stderr, "Usage: %s [-d device] [-b buffer_size] [-t title] [-h]\n\n", argv[0]);
            fprintf(stderr, "Reads all blocks of the given title from the given device and writes them to standard output.\n");
            goto out;
        }
    }

    dvd = DVDOpen2(NULL, &logger, device);
    if (!dvd) {
        fprintf(stderr, "Failed to open DVD `%s`\n", device);
        goto out;
    }

    if (info) {
        ifo_print(dvd, 0);
        goto out;
    }

    ifo = ifoOpen(dvd, 0);
    if (!ifo) {
        fprintf(stderr, "Failed to open IFO\n");
        goto out;
    }

    if (title >= ifo->tt_srpt->nr_of_srpts) {
        fprintf(stderr, "Title %d is out of range\n", title);
        goto out;
    }

    dvd_file = DVDOpenFile(dvd, ifo->tt_srpt->title[title].title_set_nr, DVD_READ_TITLE_VOBS);
    if (!dvd_file) {
        fprintf(stderr, "Failed to open VOB of title %d\n", title);
        goto out;
    }

    buffer = malloc(buffer_size / DVD_VIDEO_LB_LEN);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate buffer of %d bytes\n", buffer_size);
        goto out;
    }

    blocks_total = DVDFileSize(dvd_file);
    if (blocks_total < 0) {
        fprintf(stderr, "Failed to determine number of blocks\n");
        goto out;
    }

    while (offset < blocks_total) {
        blocks_read = DVDReadBlocks(dvd_file, offset, buffer_size / DVD_VIDEO_LB_LEN, buffer);
        if (blocks_read < 0) {
            fprintf(stderr, "Failed to read blocks\n");
            goto out;
        }

        bytes_written = write(STDOUT_FILENO, buffer, blocks_read * DVD_VIDEO_LB_LEN);
        if (bytes_written < 0) {
            fprintf(stderr, "Failed to write bytes\n");
            goto out;
        }

        offset += blocks_read;
    }

    status = EXIT_SUCCESS;

out:
    if (buffer) {
        free(buffer);
    }
    if (dvd_file) {
        DVDCloseFile(dvd_file);
    }
    if (ifo) {
        ifoClose(ifo);
    }
    if (dvd) {
        DVDClose(dvd);
    }
    return status;
}
