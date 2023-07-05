#include <stdio.h>
#include <string.h>
#include "ts_indexer.h"

#define TS_PKT_SIZE (188)

//#define TS_INDEXER_DEBUG
#ifdef TS_INDEXER_DEBUG
#define INF(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#else
#define INF(fmt, ...)
#endif

#define ERR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

#define NAL_TYPE_IDR 5 // IDR NALU 类型
#define NAL_TYPE_NON_IDR 1 // 非 IDR NALU 类型

/**
 * Initialize the TS indexer.
 * \param ts_indexer The TS indexer to be initialized.
 * \retval 0 On success.
 * \retval -1 On error.
 */
int
ts_indexer_init (TS_Indexer_t *ts_indexer)
{
  TSParser init_parser;

  if (ts_indexer == NULL) {
    return -1;
  }

  memset(&init_parser, 0, sizeof(TSParser));
  init_parser.pid = 0x1fff;
  init_parser.format = -1;
  init_parser.PES.pts = -1;
  init_parser.PES.offset = 0;
  init_parser.PES.len = 0;
  init_parser.PES.state = TS_INDEXER_STATE_INIT;

  memcpy(&ts_indexer->video_parser, &init_parser, sizeof(TSParser));
  memcpy(&ts_indexer->audio_parser, &init_parser, sizeof(TSParser));
  ts_indexer->callback     = NULL;
  ts_indexer->offset       = 0;

  return 0;
}

/**
 * Release the TS indexer.
 * \param ts_indexer The TS indexer to be released.
 */
void
ts_indexer_destroy (TS_Indexer_t *ts_indexer)
{
  return;
}

/**
 * Set the video format.
 * \param ts_indexer The TS indexer.
 * \param format The stream format.
 * \retval 0 On success.
 * \retval -1 On error.
 */
int
ts_indexer_set_video_format (TS_Indexer_t *ts_indexer, TS_Indexer_StreamFormat_t format)
{
  if (ts_indexer == NULL)
    return -1;

  ts_indexer->video_parser.format = format;

  return 0;
}

/**
 * Set the video PID.
 * \param ts_indexer The TS indexer.
 * \param pid The video PID.
 * \retval 0 On success.
 * \retval -1 On error.
 */
int
ts_indexer_set_video_pid (TS_Indexer_t *ts_indexer, int pid)
{
  if (ts_indexer == NULL)
    return -1;


  TSParser *parser = &ts_indexer->video_parser;
  parser->pid = pid;
  parser->offset = 0;
  parser->PES.pts = -1;
  parser->PES.offset = 0;
  parser->PES.len = 0;
  parser->PES.state = TS_INDEXER_STATE_INIT;

  return 0;
}

/**
 * Set the audio PID.
 * \param ts_indexer The TS indexer.
 * \param pid The audio PID.
 * \retval 0 On success.
 * \retval -1 On error.
 */
int
ts_indexer_set_audio_pid (TS_Indexer_t *ts_indexer, int pid)
{
  if (ts_indexer == NULL)
    return -1;

  TSParser *parser = &ts_indexer->audio_parser;
  parser->pid = pid;
  parser->offset = 0;
  parser->PES.pts = -1;
  parser->PES.offset = 0;
  parser->PES.len = 0;
  parser->PES.state = TS_INDEXER_STATE_INIT;

  return 0;
}

/**
 * Set the event callback function.
 * \param ts_indexer The TS indexer.
 * \param callback The event callback function.
 * \retval 0 On success.
 * \retval -1 On error.
 */
int
ts_indexer_set_event_callback (TS_Indexer_t *ts_indexer, TS_Indexer_EventCallback_t callback)
{
  ts_indexer->callback = callback;

  return 0;
}

static void find_mpeg(uint8_t *data, int len, TS_Indexer_t *indexer, TSParser *stream)
{
  int i;
  uint32_t needle = 0;
  uint8_t *haystack = data;
  int haystack_len = len;
  int left = len;
  // start code of picture header
  uint8_t arr[4] = {0x00, 0x00, 0x01, 0x00};
  TS_Indexer_Event_t event;

  /* mpeg header needs at least 4 bytes */
  if (left < 4) {
    memcpy(&stream->PES.data[0], &haystack, left);
    stream->PES.len = left;
    return;
  }

  memset(&event, 0, sizeof(event));
  event.pid = stream->pid;
  event.offset = stream->offset;

  for (i = 0; i < 4; ++i) {
    needle += (arr[i] << (8 * i));
  }

  for (i = 0; i < haystack_len - sizeof(needle) + 1;) {
    if (*(uint32_t *)(haystack + i) == needle) {
      // picture header found
      if (left < 5) {
        INF("MPEG2 picture header across TS Packet\n");

        /* MEPG2 picture header across TS packet, should cache the left data */
        memcpy(&stream->PES.data[0], haystack + i, left);
        stream->PES.len = left;
        return;
      }

      int frame_type = (haystack[i + 5] >> 3) & 0x7;
      INF("frame_type: %d\n", frame_type);
      if (frame_type == 1) {
        INF("I-frame found, offset: %ld\n", event.offset);
        if (stream->format == TS_INDEXER_VIDEO_FORMAT_MPEG2) {
          event.pts = stream->PES.pts;
          event.type = TS_INDEXER_EVENT_TYPE_VIDEO_I_FRAME;
          if (indexer->callback) {
            indexer->callback(indexer, &event);
          }
        } else {
          ERR("%s not MPEG2 video I-frame??\n", __func__);
        }
      }

      i += 5;
      left -= 5;
    } else {
      i ++;
      left --;
    }
  }

  if (left > 0) {
    memcpy(&stream->PES.data[0], &haystack[i], left);
    stream->PES.len = left;
  } else {
    stream->PES.len = 0;
  }
}

static uint8_t *get_nalu(uint8_t *data, size_t len, size_t *nalu_len)
{
  size_t i;
  uint8_t *p = data;

  if (len == 0)
    return NULL;

  //INF("%s enter, len:%#lx\n", __func__, len);
  for (i = 0; i < len - 4; ++i) {
    if (p[i] == 0x00 && p[i+1] == 0x00 && p[i+2] == 0x01) {
      uint8_t *frame_data = data + i;
      size_t frame_data_len = 0;

      //INF("%s start code prefix\n", __func__);
      for (size_t j = i + 4; j < len - 4; ++j) {
        if (p[j] == 0x00 && p[j+1] == 0x00 && p[j+2] == 0x01) {
          frame_data_len = j - i;
          break;
        }
      }

      if (frame_data_len > 0) {
        *nalu_len = frame_data_len;
        return frame_data;
      } else {
        frame_data_len = len - i;
        *nalu_len = frame_data_len;
        return frame_data;
      }
    }
  }

  return NULL;
}

static void find_h264(uint8_t *data, size_t len, TS_Indexer_t *indexer, TSParser *stream)
{
  TS_Indexer_Event_t event;
  uint8_t *nalu = data;
  size_t pes_data_len = len;
  size_t nalu_len;

  memset(&event, 0, sizeof(event));
  event.pid = stream->pid;
  event.offset = stream->offset;

  while (1) {
    int left = pes_data_len - (nalu - data);
    if (left <= 4) {
      memcpy(&stream->PES.data[0], nalu, left);
      stream->PES.len = left;
      break;
    }

    nalu = get_nalu(nalu, left, &nalu_len);
    if (nalu == NULL)
      break;

    if (nalu[0] == 0x00 && nalu[1] == 0x00 && nalu[2] == 0x01) {
      int nal_type = nalu[3] & 0x1f;
      if (nal_type == NAL_TYPE_IDR || nal_type == NAL_TYPE_NON_IDR) {
        int nal_ref_idr = nalu[3] & 0x60;
        if (nal_ref_idr == 0x60) { //I-frame
          INF("H264 I-frame found\n");
          event.pts = stream->PES.pts;
          event.type = TS_INDEXER_EVENT_TYPE_VIDEO_I_FRAME;
          if (indexer->callback) {
            indexer->callback(indexer, &event);
          }
        }
      }
    }

    nalu += nalu_len;
  }

  stream->PES.len = 0;
}

static void find_h265(uint8_t *data, int len, TS_Indexer_t *indexer, TSParser *stream)
{
  TS_Indexer_Event_t event;
  uint8_t *nalu = data;
  size_t pes_data_len = len;
  size_t nalu_len;

  memset(&event, 0, sizeof(event));
  event.pid = stream->pid;
  event.offset = stream->offset;

  while (nalu != NULL) {
    int left = pes_data_len - (nalu - data);
    if (left <= 4) {
      memcpy(&stream->PES.data[0], nalu, left);
      stream->PES.len = left;
      break;
    }

    nalu = get_nalu(nalu, left, &nalu_len);
    if (nalu == NULL)
      break;

    if (nalu[0] == 0x00 && nalu[1] == 0x00 && nalu[2] == 0x01) {
      int nalu_type = (nalu[3] & 0x7E) >> 1;
      //INF("nalu[3]: %#x, nalu_type: %#x\n", nalu[3], nalu_type);
      if (nalu_type == 19) {
        event.pts = stream->PES.pts;
        event.type = TS_INDEXER_EVENT_TYPE_VIDEO_I_FRAME;
        INF("HEVC I-frame found\n");
        if (indexer->callback) {
          indexer->callback(indexer, &event);
        }
      } else {
        //INF("%s, not HEVC video I-frame. nalu_type: %#x\n", __func__, nalu_type);
      }
    }

    nalu += nalu_len;
  }
  stream->PES.len = 0;
}

/*Parse the PES packet*/
static void
pes_packet(TS_Indexer_t *ts_indexer, uint8_t *data, int len, TSParser *stream)
{
  uint8_t *p = data;
  TS_Indexer_t *pi = ts_indexer;
  int left = len;
  TS_Indexer_Event_t event;

  memset(&event, 0, sizeof(event));
  event.pid = stream->pid;
  event.offset = stream->offset;

  INF("stream: %p, state: %d\n", stream, stream->PES.state);
  if (stream->PES.state <= TS_INDEXER_STATE_INIT) {
    INF("%s, invalid state\n", __func__);
    stream->PES.len = 0;
    return;
  }

  /* needs splice two pieces of data together if have cache data */
  if (stream->PES.len > 0) {
    INF("%s have cache data %d bytes\n", __func__, stream->PES.len);
    memcpy(&stream->PES.data[stream->PES.len], data, len);
    p = &stream->PES.data[0];
    left = stream->PES.len + len;
    stream->PES.len = left;
  }

  if (stream->PES.state == TS_INDEXER_STATE_TS_START) {
    /* needs cache data if no enough data to parse PES header */
    if (left < 6) {
      if (stream->PES.len <= 0) {
        memcpy(&stream->PES.data[0], p, left);
        stream->PES.len = left;
      }
      INF("not enough ts payload len: %#x\n", left);
      return;
    }

    // chect the PES packet start code prefix
    if ((p[0] != 0) || (p[1] != 0) || (p[2] != 1)) {
      stream->PES.len = 0;
      stream->PES.state = TS_INDEXER_STATE_INIT;
      INF("%s, not the expected start code!\n", __func__);
      return;
    }

    p += 6;
    left -= 6;
    stream->PES.state = TS_INDEXER_STATE_PES_HEADER;
  }

  if (stream->PES.state == TS_INDEXER_STATE_PES_HEADER) {
    if (left < 8) {
      if (stream->PES.len <= 0) {
        memcpy(&stream->PES.data[0], p, left);
        stream->PES.len = left;
      }
      INF("not enough optional pes header len: %#x\n", left);
      return;
    }

    int header_length = p[2];
    if (p[1] & 0x80) {
      // parser pts
      p += 3;
      left -= 3;
      event.pts = stream->PES.pts = (((uint64_t)(p[0] & 0x0E) << 29) |
                                    ((uint64_t)p[1] << 22) |
                                    ((uint64_t)(p[2] & 0xFE) << 14) |
                                    ((uint64_t)p[3] << 7) |
                                    (((uint64_t)p[4] & 0xFE) >> 1));
      INF("pts: %lx, pos:%lx\n", event.pts, event.offset);

      if (stream == &pi->video_parser) {
        event.type = TS_INDEXER_EVENT_TYPE_VIDEO_PTS;
      } else {
        event.type = TS_INDEXER_EVENT_TYPE_AUDIO_PTS;
      }
      if (pi->callback) {
        pi->callback(pi, &event);
      }
    }
    stream->PES.state = TS_INDEXER_STATE_PES_PTS;

    p += header_length;
    left -= header_length;
  }

  stream->PES.len = left;
  if (left <= 0
    || stream->PES.state < TS_INDEXER_STATE_PES_PTS) {
    return;
  }

  INF("stream->format: %d, left: %d\n", stream->format, left);
  switch (stream->format) {
    case TS_INDEXER_VIDEO_FORMAT_MPEG2:
      find_mpeg(p, left, pi, &pi->video_parser);
      break;

    case TS_INDEXER_VIDEO_FORMAT_H264:
      find_h264(p, left, pi, &pi->video_parser);
      break;

    case TS_INDEXER_VIDEO_FORMAT_HEVC:
      find_h265(p, left, pi, &pi->video_parser);
      break;

    default:
      break;
  }
}

/*Parse the TS packet.*/
static void
ts_packet(TS_Indexer_t *ts_indexer, uint8_t *data)
{
  uint16_t pid;
  uint8_t afc;
  uint8_t *p = data;
  TS_Indexer_t *pi = ts_indexer;
  int len;
  int is_start;

  is_start = p[1] & 0x40;
  pid = ((p[1] & 0x1f) << 8) | p[2];
  if (pid == 0x1fff)
    return;

  if ((pid != pi->video_parser.pid) &&
      (pid != pi->audio_parser.pid)) {
    return;
  }

  if (is_start) {
    if (pid == pi->video_parser.pid) {
      pi->video_parser.offset = pi->offset;
      pi->video_parser.PES.state = TS_INDEXER_STATE_TS_START;
    }
    else if (pid == pi->audio_parser.pid) {
      pi->audio_parser.offset = pi->offset;
      pi->audio_parser.PES.state = TS_INDEXER_STATE_TS_START;
    }
  }

  afc = (p[3] >> 4) & 0x03;

  p += 4;
  len = 184;

  if (afc & 2) {
    int adp_field_len = p[0];
    p++;
    len--;

    p += adp_field_len;
    len -= adp_field_len;

    if (len < 0) {
      ERR("illegal adaption field length!");
      return;
    }
  }

  // has payload
  if ((afc & 1) && (len > 0)) {
    // parser pes packet
    pes_packet(pi, p, len, (pid == pi->video_parser.pid) ? &pi->video_parser : &pi->audio_parser);
  }
}

/**
 * Parse the TS stream and generate the index data.
 * \param ts_indexer The TS indexer.
 * \param data The TS data.
 * \param len The length of the TS data in bytes.
 * \return The left TS data length of bytes.
 */
int
ts_indexer_parse (TS_Indexer_t *ts_indexer, uint8_t *data, int len)
{
  uint8_t *p = data;
  int left = len;

  if (ts_indexer == NULL || data == NULL  || len <= 0)
    return -1;

  while (left > 0) {
    // find the sync byte
    if (*p == 0x47) {
      if (left < TS_PKT_SIZE) {
        INF("%s data length may not be 188-byte aligned\n", __func__);
        return left;
      }

      // parse one ts packet
      ts_packet(ts_indexer, p);
      p += TS_PKT_SIZE;
      left -= TS_PKT_SIZE;
      ts_indexer->offset += TS_PKT_SIZE;
    } else {
      p++;
      left--;
      ts_indexer->offset++;
    }
  }

  return left;
}