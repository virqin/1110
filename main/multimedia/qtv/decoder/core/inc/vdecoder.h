#ifndef VDECODER_H
#define VDECODER_H
/* =======================================================================
                               vdecoder.h
DESCRIPTION
  This header file contains the function definitions for the
  QTV Video Decoder API.

Copyrighted by QUALCOMM Incorporated;
Copyright 2003 QUALCOMM Incorporated, All Rights Reserved
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia/qtv/decoder/core/rel/2.0/inc/vdecoder.h#1 $
$DateTime: 2008/11/03 04:38:22 $
$Change: 775446 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include "vdecoder_types.h"

/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
#ifdef __cplusplus
extern "C"
{
#endif

/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */
#define QTV_VDEC_REV_2

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Constant Data Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Data Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                          Macro Definitions
** ======================================================================= */

/* =======================================================================
MACRO MYOBJ

ARGS
  xx_obj - this is the xx argument

DESCRIPTION:
  Complete description of what this macro does
========================================================================== */

/* =======================================================================
**                        Function Declarations
** ======================================================================= */

/* ======================================================================
FUNCTION
  vdec_create

DESCRIPTION
  Creates a new decoder.

PARAMETERS
 const char * const      pFourcc
   The fourcc parameter specifies the FOURCC code for the media type. Without 
   the correct FOURCC code, this function will be unable to create the video 
   decoder correctly

 VDEC_STREAM_ID *const   pStreamID
   After the decoder has been successfully created vdec_create will return
   a handle to the created decoder via pStreamID. If an error occurs, pStreamID
   will be NULL and more information will be available via return error code.   

RETURN VALUE
   This function returns a VDEC_ERROR that describes the outcome of the 
   function call

========================================================================== */
VDEC_ERROR vdec_create
(
  const char * const      pFourcc,
  VDEC_STREAM_ID *const   pStreamID
);

/* ======================================================================
FUNCTION
  vdec_initialize

DESCRIPTION
  Initializes an already created decoder.

PARAMETERS
  VDEC_STREAM_ID            stream
    The stream id of the decoder which has to be initialized. This id 
    is obtained by a previous successful call to vdec_create.

  VDEC_BLOB *               pConfig
    Optional pointer to configuration information for the video stream 
    obtained from a previous successful call to vdec_dump; if the configuration 
    information is not being restored, this parameter must be set to NULL 
  
  uint16                    nLayers
    The number of layers in the video stream.  For simple streams,
    this will be 1.

  VDEC_MALLOC_FN            pfnFrameBufferMalloc
  VDEC_FREE_FN              pfnFrameBufferFree
    Pointers to malloc and free functions to be used by the decoder
    to manage frame buffers.  If the application wants control over
    where frame buffers are allocated, it can control that through
    these function pointers.

  VDEC_EVENT_CB_FN          pfnEventCb
    Callback function to be called by the decoder when an event is generated, 
    such as when a decoded frame is available; when frame buffers are passed 
    to the application, they remain property of the decoder, and it is up to 
    the application to return them to the decoder via vdec_reuse_frame_buffer.

  void *             pEventCbData
    Client data passed to the event callback; unused by the decoder provided 
    as a convenience for the application

  VDEC_DIMENSIONS           frameSize
    The source format of video frames in this stream; the decoder will use 
    this value if the source format cannot be obtained from the bitstream

  VDEC_CONCURRENCY_CONFIG  concurrencyConfig
    The concurrency requirements associated with this video stream. 

RETURN VALUE
   This function returns a VDEC_ERROR that describes the outcome of the 
   function call

========================================================================== */
VDEC_ERROR vdec_initialize
(
  const VDEC_STREAM_ID      stream,
  VDEC_BLOB *               pConfig,
  uint16                    nLayers,
  VDEC_MALLOC_FN            pfnFrameBufferMalloc,
  VDEC_FREE_FN              pfnFrameBufferFree,
  VDEC_EVENT_CB_FN          pfnEventCb,
  void *                    pEventCbData,
  VDEC_DIMENSIONS           frameSize,
  VDEC_CONCURRENCY_CONFIG   concurrencyConfig
);

/* ======================================================================
FUNCTION
  vdec_destroy

DESCRIPTION
  Destroys an existing decoder.

PARAMETERS
  VDEC_STREAM_ID stream
    The ID of the of the decoder to be destroyed.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

SIDE EFFECTS
  The stream ID provided to this function will be invalid after
  this function is called.

========================================================================== */
VDEC_ERROR vdec_destroy( const VDEC_STREAM_ID stream );

/* ======================================================================
FUNCTION
  vdec_dump

DESCRIPTION
  Serializes a running decoder into a buffer and returns the buffer to
  the application.

PARAMETERS
  const VDEC_STREAM_ID stream
    VDEC_STREAM_ID of the decoder; must be a stream identifier obtained 
    from a previous successful call to vdec_create.

  VDEC_BLOB * const       pBlob
    Handle through which the buffer is returned

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

SIDE EFFECTS
  The buffer provided to the application becomes the property of the app.
  It's up to the caller to free the buffer when it's no longer needed by
  calling vdec_free_blob

========================================================================== */
VDEC_ERROR vdec_dump( const VDEC_STREAM_ID    stream,
                      VDEC_BLOB * const       pBlob);

/* ======================================================================
FUNCTION
  vdec_reuse_frame_buffer

DESCRIPTION
  Returns a frame buffer to the decoder for reuse in output.

PARAMETERS
  const VDEC_STREAM_ID stream
    The stream ID of the decoder.

  VDEC_FRAME * const   pFrame
    The frame being returned to the decoder.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

SIDE EFFECTS
  None.
========================================================================== */
VDEC_ERROR vdec_reuse_frame_buffer( const VDEC_STREAM_ID stream,
                                    VDEC_FRAME * const   pFrame );

/* ======================================================================
FUNCTION
  vdec_queue

DESCRIPTION
  Queues input bitstreams for consumption by the decoder.  Note that no
  actual decoding of queued data will occur until vdec_decode is called.
  Exactly one natural unit of data must be submitted with queue, with 
  the exception of H.264 where it can contain a multiple number of natural 
  units.  The definition of a natural unit depends on the decoder type; 
  for MPEG-4, JPEG, Windows Media Video, and RealVideo based decoders, 
  a natural unit consists of all the data associated for one frame; 
  for H.264, it is one NAL.

PARAMETERS
  const VDEC_STREAM_ID      stream
    The stream ID of the decoder.

  VDEC_INPUT_BUFFER * const pInput
    Data from the bitstream(s).

  VDEC_QUEUE_CB_FN          pfnQueueCb
    A callback function to be called by the decoder when this input buffer
    has been processed.  This is how the decoder returns 'used' buffers
    to the application.

  void *  const             pQueueCbData
    Client data passed to the callback.  Unused by the decoder - provided
    as a convenience for the application.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

========================================================================== */
VDEC_ERROR vdec_queue( const VDEC_STREAM_ID      stream,
                       VDEC_INPUT_BUFFER * const pInput,
                       VDEC_QUEUE_CB_FN          pfnQueueCb,
                       void *  const             pQueueCbData );

/* ======================================================================
FUNCTION
  vdec_queue_eos

DESCRIPTION
  Queues a EOS buffer if the decoder is able to support it

PARAMETERS
  const VDEC_STREAM_ID stream
    The stream ID of the decoder.
  nLayers - number of layers

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked, and why we returned
  ( used up all the input data?  or timed out? )

NOTES

========================================================================== */
VDEC_ERROR vdec_queue_eos( const VDEC_STREAM_ID      stream,
                           uint32 nLayers);

/* ======================================================================
FUNCTION
  vdec_decode

DESCRIPTION
  Decodes video frames from queued input buffers.  Frames, if any become
  available are provided to the application via the provided callback.

PARAMETERS
  const VDEC_STREAM_ID stream
    The stream ID of the decoder.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked, and why we returned
  ( used up all the input data?  or timed out? )

NOTES
  Decode is not guaranteed to produce a completed video frame.  Each call
  to decode will do 'some' work.  It's up to the application to keep
  calling Decode until the video frame it wants is returned.

========================================================================== */
VDEC_ERROR vdec_decode( const VDEC_STREAM_ID stream );

/* ======================================================================
FUNCTION
  vdec_flush

DESCRIPTION
  Flushes input and output queues, and resets the decoder in preparation
  for new input.

PARAMETERS
  const VDEC_STREAM_ID stream
    The stream ID of the decoder.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

========================================================================== */
VDEC_ERROR vdec_flush( const VDEC_STREAM_ID stream );

/* ======================================================================
FUNCTION
  vdec_free_blob

DESCRIPTION
  Frees the buffer stored inside a BLOB.  Applications should call this
  to free buffers inside a decoder-allocated BLOB.

PARAMETERS
  VDEC_BLOB *pBlob
    A pointer to the BLOB.

RETURN VALUE
  None.

========================================================================== */
VDEC_ERROR vdec_free_blob( VDEC_BLOB *pBlob );

/* ======================================================================
FUNCTION
  vdec_suspend

DESCRIPTION
  This function suspends decoding and brings the decoder to a state to 
  allow for another  application to be launched . It can be called to 
  serialize the decoder prior to a call to vdec_dump, and must be called 
  prior to vdec_destroy

PARAMETERS
  const VDEC_STREAM_ID stream
    The stream ID of the decoder.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

========================================================================== */
VDEC_ERROR vdec_suspend(const VDEC_STREAM_ID stream );

/* ======================================================================
FUNCTION
  vdec_resume

DESCRIPTION
  Resumes the decoder after release.

PARAMETERS
  const VDEC_STREAM_ID stream
    The stream ID of the decoder.
  VDEC_CONCURRENCY_CONFIG audioConfig
    The concurrency requirements. This value was originally specified to 
	the decoder as a parameter to vdec_initialize.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

========================================================================== */
VDEC_ERROR vdec_resume ( const VDEC_STREAM_ID    stream,
                         VDEC_CONCURRENCY_CONFIG concurrencyConfig);

/* ======================================================================
FUNCTION
  vdec_set_parameter

DESCRIPTION
  Sets codec-specific parameters

PARAMETERS
  VDEC_STREAM_ID         stream
    The stream ID of the decoder.
  VDEC_PARAMETER_ID      parameter
    The parameter ID to set.
  VDEC_PARAMETER_DATA *  const pParameterData
    The parameter data.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

========================================================================== */
VDEC_ERROR vdec_set_parameter
(
  const VDEC_STREAM_ID        stream,
  const VDEC_PARAMETER_ID     parameter,
  VDEC_PARAMETER_DATA * const pParameterData
);

/* ======================================================================
FUNCTION
  vdec_get_parameter

DESCRIPTION
  Sets codec-specific parameters

PARAMETERS
  VDEC_STREAM_ID        stream
    The stream ID of the decoder.
  VDEC_PARAMETER_ID     parameter
    The parameter ID to get.
  VDEC_PARAMETER_DATA * pParameterData
    The parameter data.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

========================================================================== */
VDEC_ERROR vdec_get_parameter
(
  const VDEC_STREAM_ID  stream,
  const VDEC_PARAMETER_ID  parameter,
  VDEC_PARAMETER_DATA * pParameterData
);

/* ======================================================================
FUNCTION
  vdec_scale

DESCRIPTION
  Start the xScalar post processing. Start scaling all the queued frames
  which are still pending decode.

PARAMETERS
  VDEC_STREAM_ID stream
    The stream ID of the decoder.
  VDEC_DIMENSIONS* outFrameSize
    output dimensions of the scaled (up/down) video frame.
  VDEC_SCALE_CB_FN scaleCbFn
    callback function to return the status of the
    vdec_scale function call. We need this since the decoder might not know
    the source frame input dimension until it decodes the first frame. The 
    client may call vdec_create -> vdec_scale -> vdec_queue -> vdec_decode
    Decode will use the callback to specify if vdec_scale was successful or
    not.
  void * const pCbData
    the client data the application passed in.

RETURN VALUE
  A VDEC_ERROR tells the world if everything worked.

========================================================================== */
VDEC_ERROR vdec_scale( VDEC_STREAM_ID stream, 
                       VDEC_DIMENSIONS outFrameSize,
                       VDEC_SCALE_CB_FN scaleCbFn,
                       void * const     pCbData);

#ifdef __cplusplus
}
#endif

#endif /* VDECODER_H */
