#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/
#include "stdio.h"
#include "dvr_playback.h"
#include "playback_device.h"

static void display_usage(void)
{
    fprintf(stderr, "==================\n");
    fprintf(stderr, "*play\n");
    fprintf(stderr, "*pause\n");
    fprintf(stderr, "*resume\n");
    fprintf(stderr, "*ff speed(1=1X,2=4X,3=6X)\n");
    fprintf(stderr, "*fb speed(1=1X,2=4X,3=6X)\n");
    fprintf(stderr, "*seek time_in_msecond\n");
    fprintf(stderr, "*quit\n");
    fprintf(stderr, "==================\n");
}

int start_playback_test(DVR_PlaybackHandle_t handle)
{
    DVR_Bool_t  go = DVR_TRUE;
    char buf[256];

    display_usage();

    while (go) {
        if (fgets(buf, sizeof(buf), stdin)) {

            if (!strncmp(buf, "quit", 4)) {
                go = DVR_FALSE;
                continue;
            }
            else if (!strncmp(buf, "play", 4)) {
              dvr_playback_start(handle, 1);
            }
            else if (!strncmp(buf, "pause", 5)) {
                dvr_playback_pause(handle, 1);
            }
            else if (!strncmp(buf, "resume", 6)) {
                dvr_playback_resume(handle);
            }
            else if (!strncmp(buf, "ff", 2)) {
               int speed;
                sscanf(buf + 2, "%d", &speed);
                printf("fast forward not suport  is %d, speed is %d  \n",speed);
                DVR_PlaybackSpeed_t speeds;
                speeds.mode = DVR_PLAYBACK_FAST_FORWARD;
                speeds.speed.speed = speed;
                dvr_playback_set_speed(handle, speeds);
            }
            else if (!strncmp(buf, "fb", 2)) {
                int speed;
                sscanf(buf + 2, "%d", &speed);
                printf("fast forward not suport  is %d, speed is %d  \n",speed);
                DVR_PlaybackSpeed_t speeds;
                speeds.mode = DVR_PLAYBACK_FAST_BACKWARD;
                speeds.speed.speed = speed;
                dvr_playback_set_speed(handle, speeds);
            }
            else if (!strncmp(buf, "seek", 4)) {
                int time;
                sscanf(buf + 4, "%d", &time);
                dvr_playback_seek(handle, 0,time);
            } else {
                fprintf(stderr, "Unkown command: %s\n", buf);
                display_usage();
            }
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
  Playback_DeviceHandle_t device_handle;
  Playback_DeviceOpenParams_t dev_params;

  int vpid = 1019, apid = 1018, vfmt = 0, afmt = 0;
  int bsize = 256 * 1024;
  int pause = 0;
  int segid = 1;
  int dmx = 0;
  int i;

  for (i = 1; i < argc; i++) {
      if (!strncmp(argv[i], "v", 1))
          sscanf(argv[i], "v=%i:%i", &vpid, &vfmt);
      else if (!strncmp(argv[i], "a", 1))
          sscanf(argv[i], "a=%i:%i", &apid, &afmt);
      else if (!strncmp(argv[i], "bsize", 4))
          sscanf(argv[i], "bsize=%i", &bsize);
      else if (!strncmp(argv[i], "dmx", 3))
          sscanf(argv[i], "dmx=%i", &dmx);
      else if (!strncmp(argv[i], "pause", 5))
          sscanf(argv[i], "pause=%i", &pause);
      else if (!strncmp(argv[i], "segid", 5))
          sscanf(argv[i], "segid=%i", &segid);
      else if (!strncmp(argv[i], "help", 4)) {
          printf("Usage: %s  [dmx=id] [segid=id] [v=pid:fmt] [a=pid:fmt] [bsize=size] [pause=n]\n", argv[0]);
          exit(0);
      }
  }

  printf("video:%d:%d(pid/fmt) audio:%d:%d(pid/fmt)\n", vpid, vfmt, apid, afmt);
  printf("segid:%d bsize:%d dmx:%d\n", segid, bsize, dmx);
  printf("pause:%d\n", pause);
  dev_params.dmx = dmx;

  int ret = playback_device_open(&device_handle, &dev_params);

  DVR_PlaybackHandle_t handle = 0;
  DVR_PlaybackOpenParams_t params;
  params.dmx_dev_id = dmx;
  params.block_size = bsize;
  params.playback_handle = device_handle;
  printf("open dvr playback device\r\n");
  dvr_playback_open(&handle, &params);
  //add chunk info
  DVR_PlaybackSegmentInfo_t info;
  info.segment_id = segid;
  info.flags = 1;
  memcpy(info.location, "/data/data/", 12);
  info.pids.video.pid = vpid;
  info.pids.video.format = vfmt;
  info.pids.audio.pid = apid;
  info.pids.audio.format = afmt;
  // info.pids.pcr.pid = 0x30;
  // info.pids.pcr.format = 0x30;
  // info.pids.ad.pid = 0x31;
  // info.pids.ad.format = 0x31;
  dvr_playback_add_segment(handle, &info);
  start_playback_test(handle);
  dvr_playback_stop(handle, 0);
  dvr_playback_close(handle);
  playback_device_close(device_handle);
  return ret;
}
