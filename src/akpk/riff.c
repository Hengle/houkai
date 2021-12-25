#ifndef RIFF_H
#define RIFF_H

#define _DEFAULT_SOURCE
#include "riff.h"
#include "akpk.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define WAVE 0x45564157
#define FMT0 0x20746D66

/* http://soundfile.sapp.org/doc/WaveFormat */

enum chunk_type {
  /*RIFF_FMT  = 0x666d7420,*/
  RIFF_FMT = 0x20746d66,
  RIFF_WAVE = 0x45564157,
  RIFF_DATA = 0x61746164,
  RIFF_LIST = 0x5453494C,
  RIFF_CUE = 0x20657563,
  RIFF_JUNK = 0x4B4E554A,
  RIFF_AKD = 0x20646B61 /* some dummy padding. idk */
};

void save_wem(void *data, size_t size, uint64_t id, char *path) {
  char *fullpath = NULL;
  unsigned long len;
  int out;
  struct stat sib;

  len = strlen(path) + (sizeof(id) << 2) + 1 + 1;

  fullpath = (char *)malloc(len);

  if (fullpath == NULL) {
    fprintf(stderr, "Could not allocate memory for full file path: %s\n",
            strerror(errno));
    return;
  }

  snprintf(fullpath, len, "%s/%lX.wem", path, id);

  if (stat(fullpath, &sib) == 0) {
    if (sib.st_size == size) {
      printf("File %s already exists and have same size. Skip.\n", fullpath);
      return;
    } else {
      printf("File %s already exists but have different size. Override.\n",
             fullpath);
    }
  }

  out =
      open(fullpath, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  if (out) {
    ssize_t ret = write(out, data, size);

    if (ret != (ssize_t)size) {
      char *err_str = strerror(errno);
      printf("Failed to save %s: %s\n", fullpath, err_str);
    }
    close(out);
  }

  free(fullpath);
}

enum codec_t { CODEC_PCM, CODEC_WAVE };
typedef enum codec_t codec_t;

struct riff_chunk {
  uint32_t chunk_id;
  uint32_t size;
  uint32_t type;
};

struct fmt_chunk {
  uint32_t chunk_id;
  uint32_t size;
  uint16_t format_tag;
  uint16_t channels;
  uint32_t sample_rate;
  uint32_t avg_byte_rate;
  uint16_t block_size;
  uint16_t bits_per_sample;
};

struct fmt_extra_chunk {
  uint32_t extra_size;
  union {
    uint16_t valid_bits_per_sample;
    uint16_t samples_per_block;
    uint16_t _reserved;
  } channel_layout;
  uint32_t channel_mask;
  struct {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint32_t data4;
    uint32_t data5;
  } guid;
};

struct data_chunk {
  uint32_t chunk_id;
  uint32_t size;
};

struct cue_chunk {
  uint32_t chunk_id;
  uint32_t size;
  uint32_t points;
};

struct cue_point {
  uint32_t chunk_id;
  uint32_t position;
  uint32_t fcc_chunk;
  uint32_t chunk_start;
  uint32_t block_start;
  uint32_t offset;
};

struct list_chunk {
  uint32_t chunk_id;
  uint32_t size;
  uint32_t adtl;
};

struct labl {
  uint32_t chunk_id;
  uint32_t size;
  uint32_t cue_point_id;
};

struct junk_chunk {
  uint32_t chunk_id;
  uint32_t size;
  /* uint8_t data[size]*/
};

void wem_info(void *data) {
  uint32_t chunk;
  struct riff_chunk *riff;
  void *pos = data;

  riff = (struct riff_chunk *)pos;

  if (riff->chunk_id != RIFF) {
    printf("Error while reading WEM: RIFF expected.\n");
    return;
  }

  pos = (void *)((uintptr_t)pos + sizeof(*riff));

  while (pos < (void *)((uintptr_t)data + riff->size)) {
    chunk = *(uint32_t *)pos;

    switch (chunk) {
    case RIFF_FMT: {
      struct fmt_chunk *fmt = (struct fmt_chunk *)pos;
      pos = (void *)((uintptr_t)pos + fmt->size + 4 + 4);
    } break;
    case RIFF_DATA: {
      struct data_chunk *data = (struct data_chunk *)pos;
      pos = (void *)((uintptr_t)pos + 4 + 4 + data->size);
    } break;
    case RIFF_CUE: {
      struct cue_chunk *cue = (struct cue_chunk *)pos;
      pos = (void *)((uintptr_t)pos + 4 + 4 + cue->size);
      /* cue points wanna see? */
    } break;
    case RIFF_LIST: {
      struct list_chunk *list = (struct list_chunk *)pos;
      pos = (void *)((uintptr_t)pos + 4 + 4 + list->size);
    } break;
    case RIFF_AKD:
    case RIFF_JUNK: {
      struct junk_chunk *junk = (struct junk_chunk *)pos;
      pos = (void *)((uintptr_t)pos + 4 + 4 + junk->size);
    } break;
    default:
      printf("Unknown chunk: %X\n", chunk);
      pos = (void *)((uintptr_t)pos + 4);
      break;
    }
  }
}
#endif

/* vim: ts=2 sw=2 expandtab
 */
