/**
 * \file
 * Playback module.
 */

#ifndef DVR_PLAYBACK_H_
#define DVR_PLAYBACK_H_
#include "list.h"
#include "dvr_types.h"
#include "segment.h"
#include "playback_device.h"
#include "dvr_types.h"
#include "dvr_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/**\brief dvr play segment flag */
typedef enum
{
  DVR_PLAYBACK_SEGMENT_ENCRYPTED   = (1 << 0), /**< encrypted stream */
  DVR_PLAYBACK_SEGMENT_DISPLAYABLE = (1 << 1), /**< displayable stream */
  DVR_PLAYBACK_SEGMENT_CONTINUOUS  = (1 << 2)  /**< continuous stream with pre one */
} DVR_PlaybackSegmentFlag_t;

/**\brief dvr pid type*/
typedef enum
{
  DVR_PLAYBACK_SYNC,    /**< sync play mode */
  DVR_PLAYBACK_ASYNC    /**< out of sync play mode */
} DVR_PlaybackSyncMode_t;


/**\brief dvr play pids */
typedef struct
{
  DVR_StreamInfo_t video;    /**< Video.*/
  DVR_StreamInfo_t audio;    /**< Audio.*/
  DVR_StreamInfo_t ad;       /**< AD.*/
  DVR_StreamInfo_t subtitle; /**< Subtitle.*/
  DVR_StreamInfo_t pcr;      /**< PCR.*/
} DVR_PlaybackPids_t;

/**\brief dvr segments info */
typedef struct
{
  struct  list_head head;         /**< Segment node.*/
  int     segment_id;             /**< Segment's index.*/
  uint8_t location[DVR_MAX_LOCATION_SIZE];  /**< chunk location */
  DVR_PlaybackPids_t  pids;       /**< Streams' PIDs.*/
  DVR_PlaybackSegmentFlag_t flags; /**< Segment's flag */
  int key_data_id;                /**< ??? */
} DVR_PlaybackSegmentInfo_t;

/**\brief play flag, if set this flag, player need pause when decode first frame */
typedef enum
{
  DVR_PLAYBACK_STARTED_PAUSEDLIVE = (1 << 0) /**< dvr play stat,need change to pause state if set */
} DVR_PlaybackFlag_t;

/**\brief playback speed mode*/
typedef enum
{
  DVR_PLAYBACK_FAST_FORWARD = 0,        /**< fast forward */
  DVR_PLAYBACK_FAST_BACKWARD = 1,       /**< fast backward */
} DVR_PlaybackSpeedMode_t;


/**\brief playback play speed*/
typedef struct
{
  Playback_DeviceSpeeds_t speed;  /**< playback speed */
  DVR_PlaybackSpeedMode_t mode;   /**< playback 0: fast forword or 1: fast backword*/
} DVR_PlaybackSpeed_t;

/**Maximum supported speed modes.*/
#define DVR_MAX_SUPPORTED_SPEEDS  32

/**\brief playback capability*/
typedef struct
{
  int nb_supported_speeds;       /**< support playback speed count*/
  int supported_speeds[DVR_MAX_SUPPORTED_SPEEDS]; /**< support playback speed*/
} DVR_PlaybackCapability_t;

/**Playback handle.*/
typedef void* DVR_PlaybackHandle_t;

/**\brief playback play state*/
typedef enum
{
  DVR_PLAYBACK_STATE_UNINIT,      /**< uninit state */
  DVR_PLAYBACK_STATE_INIT,        /**< init state, open device */
  DVR_PLAYBACK_STATE_START,       /**< start play */
  DVR_PLAYBACK_STATE_STOP,        /**< stop */
  DVR_PLAYBACK_STATE_PAUSE,       /**< pause */
  DVR_PLAYBACK_STATE_FF,          /**< fast forward */
  DVR_PLAYBACK_STATE_FB           /**< fast backword */
} DVR_PlaybackPlayState_t;

/**\brief playback play status*/
typedef struct
{
  DVR_PlaybackPlayState_t state;  /**< playback play state */
  int segment_id;                 /**< playback ongoing segment index */
  int time_cur;                   /**< playback cur time,0 <--> time_end Ms*/
  int time_end;                   /**< playback ongoing segment dur,Ms */
  DVR_PlaybackPids_t pids;        /**< playback played pids */
  Playback_DeviceSpeeds_t speed;  /**< playback speed */
  DVR_PlaybackSegmentFlag_t flags;/**< playback played segment flag */
} DVR_PlaybackStatus_t;

/**\brief playback open params*/
typedef struct
{
  int                    dmx_dev_id;      /**< playback used dmx device index*/
  int                    block_size;      /**< playback inject block size*/
  DVR_Bool_t             is_timeshift;    /**< 0:playback mode, 1 : is timeshift mode*/
  Playback_DeviceHandle_t playback_handle; /**< Playback device handle.*/
  DVR_CryptoFunction_t   crypto_fn;       /**< Crypto function.*/
  void                  *crypto_data;     /**< Crypto function's user data.*/
} DVR_PlaybackOpenParams_t;

/**\brief playback play state*/
typedef enum
{
  DVR_PLAYBACK_CMD_START,               /**< start av */
  DVR_PLAYBACK_CMD_STOP,                /**< stop av */
  DVR_PLAYBACK_CMD_VSTART,              /**< v start */
  DVR_PLAYBACK_CMD_ASTART   ,           /**< a start */
  DVR_PLAYBACK_CMD_VSTOP  ,             /**< v stop */
  DVR_PLAYBACK_CMD_ASTOP,               /**< a stop */
  DVR_PLAYBACK_CMD_VRESTART,            /**< v restart */
  DVR_PLAYBACK_CMD_ARESTART,            /**< a restart */
  DVR_PLAYBACK_CMD_VSTOPASTART,         /**< v stop a start*/
  DVR_PLAYBACK_CMD_ASTOPVSTART,         /**< a stop vstart */
  DVR_PLAYBACK_CMD_VSTOPARESTART,       /**<v stop a restart*/
  DVR_PLAYBACK_CMD_ASTOPVRESTART,       /**<a stop v restart*/
  DVR_PLAYBACK_CMD_PAUSE,               /**< pause */
  DVR_PLAYBACK_CMD_RESUME,              /**< resume */
  DVR_PLAYBACK_CMD_SEEK,                /**< seek */
  DVR_PLAYBACK_CMD_FF,                  /**< fast forward */
  DVR_PLAYBACK_CMD_FB,                  /**< fast backword */
} DVR_PlaybackCmd_t;


/**\brief playback struct*/
typedef struct
{
  Playback_DeviceSpeeds_t   speed;     /**< play speed */
  DVR_PlaybackPlayState_t   state;     /**< plat state */
  DVR_PlaybackCmd_t         cur_cmd;    /**< cur send cmd */
  DVR_PlaybackCmd_t         last_cmd;   /**< last cmd */
  int                       pos;        /**< seek pos at cur segment*/
} DVR_PlaybackCmdInfo_t;


/**\brief playback struct*/
typedef struct
{
  Playback_DeviceHandle_t    handle;             /**< device handle */
  int                        cur_segment_id;        /**< Current segment id*/
  DVR_PlaybackSegmentInfo_t  cur_segment;          /**< Current playing segment*/
  struct list_head           segment_list;         /**< segment list head*/
  pthread_t                  playback_thread;    /**< playback thread*/
  pthread_mutex_t            lock;               /**< playback lock*/
  pthread_cond_t             cond;               /**< playback cond*/
  void                       *user_data;         /**< playback userdata, used to send event*/
  Playback_DeviceSpeeds_t    speed;           /**< playback speed*/
  DVR_PlaybackPlayState_t    state;           /**< playback state*/
  DVR_PlaybackFlag_t         play_flag;           /**< playback play flag*/
  int                        is_running;           /**< playback htread is runing*/
  DVR_PlaybackCmdInfo_t      cmd;           /**< playback cmd*/
  int                        offset;         /**< segment read offset*/
  Segment_Handle_t           r_handle;           /**< playback current segment handle*/
  DVR_PlaybackOpenParams_t   openParams;           /**< playback openParams*/
  DVR_Bool_t                 has_video;    /**< has video playing*/
  DVR_Bool_t                 has_audio;    /**< has audio playing*/
} DVR_Playback_t;

/**\brief Open an dvr palyback
 * \param[out] p_handle dvr playback addr
 * \param[in] params dvr playback open parameters
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_open(DVR_PlaybackHandle_t *p_handle, DVR_PlaybackOpenParams_t *params);

/**\brief Close an dvr palyback
 * \param[in] handle playback handle
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_close(DVR_PlaybackHandle_t handle);


/**\brief Start play audio and video, used start auido api and start video api
 * \param[in] handle playback handle
 * \param[in] params audio playback params,contains fmt and pid...
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_start(DVR_PlaybackHandle_t handle, DVR_PlaybackFlag_t flag);

/**\brief dvr play back add segment info to segment list
 * \param[in] handle playback handle
 * \param[in] info added segment info,con vpid fmt apid fmt.....
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_add_segment(DVR_PlaybackHandle_t handle, DVR_PlaybackSegmentInfo_t *info);

/**\brief dvr play back remove segment info by segmentkid
 * \param[in] handle playback handle
 * \param[in] segmentid need removed segment id
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_remove_segment(DVR_PlaybackHandle_t handle, int segmentkid);

/**\brief dvr play back add segment info
 * \param[in] handle playback handle
 * \param[in] info added segment info,con vpid fmt apid fmt.....
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_update_segment_flags(DVR_PlaybackHandle_t handle,
    int segment_id,
    DVR_PlaybackSegmentFlag_t flags);
/**\brief dvr play back up1date segment pids
 * if updated segment is ongoing segment, we need start new
 * add pid stream and stop remove pid stream.
 * \param[in] handle playback handle
 * \param[in] segment_id need updated pids segment id
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_update_segment_pids(DVR_PlaybackHandle_t handle, int segmentid,
DVR_PlaybackPids_t *p_pids);

/**\brief Stop play, will stop video and audio
 * \param[in] handle playback handle
 * \param[in] clear is clear last frame
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_stop(DVR_PlaybackHandle_t handle, DVR_Bool_t clear);

/**\brief Start play audio
 * \param[in] handle playback handle
 * \param[in] params audio playback params,contains fmt and pid...
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_audio_start(DVR_PlaybackHandle_t handle, Playback_DeviceAudioParams_t *param);

/**\brief Stop play audio
 * \param[in] handle playback handle
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_audio_stop(DVR_PlaybackHandle_t handle);

/**\brief Start play video
 * \param[in] handle playback handle
 * \param[in] params video playback params,contains fmt and pid...
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_video_start(DVR_PlaybackHandle_t handle, Playback_DeviceVideoParams_t *param);

/**\brief Stop play video
 * \param[in] handle playback handle
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_video_stop(DVR_PlaybackHandle_t handle);

/**\brief Pause play
 * \param[in] handle playback handle
 * \param[in] flush whether its internal buffers should be flushed
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_pause(DVR_PlaybackHandle_t handle, DVR_Bool_t flush);


/**\brief seek
 * \param[in] handle playback handle
 * \param[in] time_offset time offset base cur segment
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_seek(DVR_PlaybackHandle_t handle, int segment_id, int time_offset);

/**\brief Set play speed
 * \param[in] handle playback handle
 * \param[in] speed playback speed
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_set_speed(DVR_PlaybackHandle_t handle, Playback_DeviceSpeeds_t speed);

/**\brief Get playback status
 * \param[in] handle playback handle
 * \param[out] p_status playback status
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_get_status(DVR_PlaybackHandle_t handle, DVR_PlaybackStatus_t *p_status);

/**\brief Get playback capabilities
 * \param[out] p_capability playback capability
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_playback_get_capabilities(DVR_PlaybackCapability_t *p_capability);

/**\brief dump segmentinfo throw print log
 * \param[int] handle playback
 * \param[int] segment_id if segment_id > 0, only dump this log. else dump all segment info
 * \retval DVR_SUCCESS On success
 * \return Error code
 */
int dvr_dump_segmentinfo(DVR_PlaybackHandle_t handle, int segment_id);

#ifdef __cplusplus
}
#endif

#endif /*END DVR_PLAYBACK_H_*/