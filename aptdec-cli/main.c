/*
 * aptdec - A lightweight FOSS (NOAA) APT decoder
 * Copyright (C) 2004-2009 Thierry Leconte (F4DWV) 2019-2023 Xerbo (xerbo@protonmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <libgen.h>
#else
#include <windows.h>
#endif
#include <math.h>
#include <sndfile.h>
#include <time.h>
#include <signal.h>
#include <float.h>
#include <aptdec.h>

#include "argparse/argparse.h"
#include "pngio.h"
#include "util.h"

// Maximum height of an APT image that can be decoded
#define APTDEC_MAX_HEIGHT 3000

#define STRING_SPLIT(src, dst, delim, n) \
char *dst[n]; \
size_t dst##_len = 0; \
{char *token = strtok(src, delim); \
while (token != NULL && dst##_len < n) { \
    dst[dst##_len++] = token; \
    token = strtok(NULL, delim); \
}}

typedef struct {
    char *type;      // Output image type
    char *effects;   // Effects on the image
    int satellite;   // The satellite number
    int realtime;    // Realtime decoding
    char *filename;  // Output filename
    char *lut;       // Filename of lut
} options_t;

typedef struct {
    SNDFILE *file;
    SF_INFO info;
} SNDFILE_t;

// Function declarations
static int process_file(const char *path, options_t *opts);
static int freq_from_filename(const char *filename);
static int satid_from_freq(int freq);
static size_t callback(float *samples, size_t count, void *context);
static void write_line(writer_t *png, float *row);

static volatile int sigint_stop = 0;
void sigint_handler(int signum) {
    (void)signum;
    sigint_stop = 1;
}

// Basename is unsupported by MSVC
// This implementation is GNU style
#ifdef _MSC_VER
char *basename(const char *filename) {
    char *p = strrchr(filename, '/');
    return p ? p + 1 : (char *)filename;
}
#endif

int main(int argc, const char **argv) {
    char version[128];
    get_version(version);
    printf("%s\n", version);
    // clang-format off
    options_t opts = {
        .type = "raw",
        .effects = "",
        .satellite = 0,
        .realtime = 0,
        .filename = "",
        .lut = "",
    };
    // clang-format on

    static const char *const usages[] = {
        "aptdec-cli [options] [[--] sources]",
        "aptdec-cli [sources]",
        NULL,
    };

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Image options"),
        OPT_STRING('i', "image", &opts.type, "set output image type (see the README for a list)", NULL, 0, 0),
        OPT_STRING('e', "effect", &opts.effects, "add an effect (see the README for a list)", NULL, 0, 0),
        OPT_GROUP("Satellite options"),
        OPT_INTEGER('s', "satellite", &opts.satellite, "satellite ID, must be either NORAD or 15/18/19", NULL, 0, 0),
        OPT_GROUP("Paths"),
        OPT_STRING('l', "lut", &opts.lut, "path to a LUT", NULL, 0, 0),
        OPT_STRING('o', "output", &opts.filename, "path of output image", NULL, 0, 0),
        OPT_GROUP("Misc"),
        OPT_BOOLEAN('r', "realtime", &opts.realtime, "decode in realtime", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse,
        "\nA lightweight FOSS NOAA APT satellite imagery decoder.",
        "\nSee `README.md` for a full description of command line arguments and `LICENSE` for licensing conditions."
    );
    argc = argparse_parse(&argparse, argc, argv);

    if (argc == 0) {
        argparse_usage(&argparse);
    }
    if (argc > 1 && opts.realtime) {
        error("Cannot use -r/--realtime with multiple input files");
    }

    // Actually decode the files
    for (int i = 0; i < argc; i++) {
        if (opts.satellite == 25338) opts.satellite = 15;
        if (opts.satellite == 28654) opts.satellite = 18;
        if (opts.satellite == 33591) opts.satellite = 19;

        if (opts.satellite == 0) {
            int freq = freq_from_filename(argv[i]);
            if (freq == 0) {
                opts.satellite = 19;
                warning("Satellite not specified, defaulting to NOAA-19");
            } else {
                opts.satellite = satid_from_freq(freq);
                printf("Satellite not specified, choosing to NOAA-%i based on filename\n", opts.satellite);
            }
        }

        if (opts.satellite != 15 && opts.satellite != 18 && opts.satellite != 19) {
            error("Invalid satellite ID");
        }

        process_file(argv[i], &opts);
    }

    return 0;
}

static int process_file(const char *path, options_t *opts) {
    const char *path_basename = basename((char *)path);
    const char *dot = strrchr(path_basename, '.');
    char name[256];
    if (dot == NULL) {
        strncpy(name, path_basename, 255);
    } else {
        strncpy(name, path_basename, clamp_int(dot - path_basename, 0, 255));
    }

    // Set filename to time when reading from stdin
    if (strcmp(name, "-") == 0) {
        time_t t = time(NULL);
        strcpy(name, ctime(&t));
    }

    writer_t *realtime_png;
    if (opts->realtime) {
        char filename[269];
        sprintf(filename, "%s-decoding.png", name);
        realtime_png = writer_init(filename, APT_REGION_FULL, APTDEC_MAX_HEIGHT, PNG_COLOR_TYPE_GRAY, "Unknown");

        // Capture Ctrl+C
        signal(SIGINT, sigint_handler);
    }

    // Create a libsndfile instance
    SNDFILE_t audioFile;
    audioFile.file = sf_open(path, SFM_READ, &audioFile.info);
    if (audioFile.file == NULL) {
        error_noexit("Could not open file");
        return 0;
    }

    printf("Input file: %s\n", path_basename);
    printf("Input sample rate: %d\n", audioFile.info.samplerate);

    // Create a libaptdec instances
    aptdec_t *aptdec = aptdec_init(audioFile.info.samplerate);
    if (aptdec == NULL) {
        sf_close(audioFile.file);
        error_noexit("Error initializing libaptdec, sample rate too high/low?");
        return 0;
    }

    // Decode image
    float *data = (float *)malloc(APT_IMG_WIDTH * (APTDEC_MAX_HEIGHT+1) * sizeof(float));
    size_t rows;
    for (rows = 0; rows < APTDEC_MAX_HEIGHT; rows++) {
        float *row = &data[rows * APT_IMG_WIDTH];

        // Break the loop when there are no more samples or the process has been sent SIGINT
        if (aptdec_getrow(aptdec, row, callback, &audioFile) == 0 || sigint_stop) {
            break;
        }

        if (opts->realtime) {
            write_line(realtime_png, row);
        }

        fprintf(stderr, "Row: %zu/%zu\r", rows+1, audioFile.info.frames/audioFile.info.samplerate * 2);
        fflush(stderr);
    }
    printf("\n");

    // Close stream
    sf_close(audioFile.file);
    aptdec_free(aptdec);

    if (opts->realtime) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
        writer_free(realtime_png);
#pragma GCC diagnostic pop

        char filename[269];
        sprintf(filename, "%s-decoding.png", name);
        remove(filename);
    }

    // Normalize
    int error;
    apt_image_t img = apt_normalize(data, rows, opts->satellite, &error);
    free(data);
    if (error) {
        error_noexit("Normalization failed");
        return 0;
    }

    // clang-format off
    const char *channel_name[] = { "?", "1", "2", "3A", "4", "5", "3B" };
    const char *channel_desc[] = {
        "unknown",
        "visible (0.58-0.68 um)",
        "near-infrared (0.725-1.0 um)",
        "near-infrared (1.58-1.64 um)",
        "thermal-infrared (10.3-11.3 um)",
        "thermal-infrared (11.5-12.5 um)",
        "mid-infrared (3.55-3.93 um)"
    };

    printf("Channel A: %s - %s\n", channel_name[img.ch[0]], channel_desc[img.ch[0]]);
    printf("Channel B: %s - %s\n", channel_name[img.ch[1]], channel_desc[img.ch[1]]);
    // clang-format on

    STRING_SPLIT(opts->type, images, ",", 10)
    STRING_SPLIT(opts->effects, effects, ",", 10)
    STRING_SPLIT(opts->filename, filenames, ",", 10)

    for (size_t i = 0; i < effects_len; i++) {
        if (strcmp(effects[i], "crop") == 0) {
            apt_crop(&img);
        } else if (strcmp(effects[i], "denoise") == 0) {
            apt_denoise(&img, APT_REGION_CHA);
            apt_denoise(&img, APT_REGION_CHB);
        } else if (strcmp(effects[i], "flip") == 0) {
            apt_flip(&img, APT_REGION_CHA);
            apt_flip(&img, APT_REGION_CHB);
        }
    }

    for (size_t i = 0; i < images_len; i++) {
        const char *base;
        if (i < filenames_len) {
            base = filenames[i];
        } else {
            base = name;
        }

        if (strcmp(images[i], "thermal") == 0) {
            if (img.ch[1] >= 4) {
                char filename[269];
                sprintf(filename, "%s-thermal.png", base);
                char description[128];
                sprintf(description, "Calibrated thermal image, channel %s - %s", channel_name[img.ch[1]], channel_desc[img.ch[1]]);

                // Perform visible calibration
                apt_image_t _img = apt_image_clone(img);
                apt_calibrate_thermal(&_img, APT_REGION_CHA);

                writer_t *writer = writer_init(filename, APT_REGION_CHB, img.rows, PNG_COLOR_TYPE_RGB, description);
                writer_write_image_gradient(writer, &_img, temperature_gradient);
                writer_free(writer);

                free(_img.data);
            } else {
                error_noexit("Could not generate thermal image, no infrared channel");
            }
        } else if (strcmp(images[i], "visible") == 0) {
            if (img.ch[0] <= 2) {
                char filename[269];
                sprintf(filename, "%s-visible.png", base);
                char description[128];
                sprintf(description, "Calibrated visible image, channel %s - %s", channel_name[img.ch[0]], channel_desc[img.ch[0]]);

                // Perform visible calibration
                apt_image_t _img = apt_image_clone(img);
                apt_calibrate_visible(&_img, APT_REGION_CHA);

                writer_t *writer = writer_init(filename, APT_REGION_CHA, img.rows, PNG_COLOR_TYPE_GRAY, description);
                writer_write_image(writer, &_img);
                writer_free(writer);

                free(_img.data);
            } else {
                error_noexit("Could not generate visible image, no visible channel");
            }
        }
    }

    for (size_t i = 0; i < effects_len; i++) {
        if (strcmp(effects[i], "stretch") == 0) {
            apt_stretch(&img, APT_REGION_CHA);
            apt_stretch(&img, APT_REGION_CHB);
        } else if (strcmp(effects[i], "equalize") == 0) {
            apt_equalize(&img, APT_REGION_CHA);
            apt_equalize(&img, APT_REGION_CHB);
        }
    }

    for (size_t i = 0; i < images_len; i++) {
        const char *base;
        if (i < filenames_len) {
            base = filenames[i];
        } else {
            base = name;
        }

        if (strcmp(images[i], "raw") == 0) {
            char filename[269];
            sprintf(filename, "%s-raw.png", base);
            char description[128];
            sprintf(description,
                "Raw image, channel %s - %s / %s - %s",
                channel_name[img.ch[0]],
                channel_desc[img.ch[0]],
                channel_name[img.ch[1]],
                channel_desc[img.ch[1]]
            );

            writer_t *writer = writer_init(filename, APT_REGION_FULL, img.rows, PNG_COLOR_TYPE_GRAY, description);
            writer_write_image(writer, &img);
            writer_free(writer);
        } else if (strcmp(images[i], "lut") == 0) {
            if (opts->lut != NULL && opts->lut[0] != '\0') {
                char filename[269];
                sprintf(filename, "%s-lut.png", base);
                char description[128];
                sprintf(description,
                    "LUT image, channel %s - %s / %s - %s",
                    channel_name[img.ch[0]],
                    channel_desc[img.ch[0]],
                    channel_name[img.ch[1]],
                    channel_desc[img.ch[1]]
                );

                png_colorp lut = (png_colorp)malloc(sizeof(png_color)*256*256);
                if (read_lut(opts->lut, lut)) {
                    writer_t *writer = writer_init(filename, APT_REGION_CHA, img.rows, PNG_COLOR_TYPE_RGB, description);
                    writer_write_image_lut(writer, &img, lut);
                    writer_free(writer);
                }
                free(lut);
            } else {
                warning("Cannot create LUT image, missing -l/--lut");
            }
        } else if (strcmp(images[i], "a") == 0) {
            char filename[269];
            sprintf(filename, "%s-a.png", base);
            char description[128];
            sprintf(description, "Channel A: %s - %s", channel_name[img.ch[0]], channel_desc[img.ch[0]]);

            writer_t *writer = writer_init(filename, APT_REGION_CHA_FULL, img.rows, PNG_COLOR_TYPE_GRAY, description);
            writer_write_image(writer, &img);
            writer_free(writer);
        } else if (strcmp(images[i], "b") == 0) {
            char filename[269];
            sprintf(filename, "%s-b.png", base);
            char description[128];
            sprintf(description, "Channel B: %s - %s", channel_name[img.ch[1]], channel_desc[img.ch[1]]);

            writer_t *writer = writer_init(filename, APT_REGION_CHB_FULL, img.rows, PNG_COLOR_TYPE_GRAY, description);
            writer_write_image(writer, &img);
            writer_free(writer);
        }
    }

    free(img.data);
    return 1;
}

static int freq_from_filename(const char *filename) {
    char frequency_text[10] = {'\0'};

    if (strlen(filename) >= 30+4 && strncmp(filename, "gqrx_", 5) == 0) {
        memcpy(frequency_text, &filename[21], 9);
        return atoi(frequency_text);
    } else if (strlen(filename) == 40+4 && strncmp(filename, "SDRSharp_", 9) == 0) {
        memcpy(frequency_text, &filename[26], 9);
        return atoi(frequency_text);
    } else if (strlen(filename) == 37+4 && strncmp(filename, "audio_", 6) == 0) {
        memcpy(frequency_text, &filename[6], 9);
        return atoi(frequency_text);
    }

    return 0;
}

static int satid_from_freq(int freq) {
    int differences[3] = {
        abs(freq - 137620000),  // NOAA-15
        abs(freq - 137912500),  // NOAA-18
        abs(freq - 137100000)   // NOAA-19
    };

    int best = 0;
    for (size_t i = 0; i < 3; i++) {
        if (differences[i] < differences[best]) {
            best = i;
        }
    }

    const int lut[3] = {15, 18, 19};
    return lut[best];
}

// Read samples from a SNDFILE_t instance (passed through context)
static size_t callback(float *samples, size_t count, void *context) {
    SNDFILE_t *file = (SNDFILE_t *)context;

    switch (file->info.channels) {
        case 1:
            return sf_read_float(file->file, samples, count);
        case 2: {
            float _samples[APTDEC_BUFFER_SIZE * 2];
            size_t read = sf_read_float(file->file, _samples, count * 2);

            for (size_t i = 0; i < count; i++) {
                // Average of left and right
                samples[i] = (_samples[i*2] + _samples[i*2 + 1]) / 2.0f;
            }
            return read / 2;
        }
        default:
            error_noexit("Only mono and stereo audio files are supported\n");
            return 0;
    }
}

// Write a line with very basic equalization
static void write_line(writer_t *png, float *row) {
    float min = FLT_MAX;
    float max = FLT_MIN;
    for (int i = 0; i < APT_IMG_WIDTH; i++) {
        if (row[i] < min) min = row[i];
        if (row[i] > max) max = row[i];
    }

    png_byte pixels[APT_IMG_WIDTH];
    for (int i = 0; i < APT_IMG_WIDTH; i++) {
        pixels[i] = clamp_int(roundf((row[i]-min) / (max-min) * 255.0f), 0, 255);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    png_write_row(png->png, pixels);
#pragma GCC diagnostic pop
}
