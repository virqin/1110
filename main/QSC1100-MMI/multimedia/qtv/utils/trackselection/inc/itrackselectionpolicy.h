/* =======================================================================
                      ITrackSelectionPolicy.h
DESCRIPTION
  This module contains the declaration of the ITrackSelectionPolicy 
  interface. It is an interface for a strategy for selecting tracks of a 
  ITrackList that can be provided by the client of QTV. Since it can be 
  provided by the client, it is reference counted.
  
EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright 2006-2006 QUALCOMM Incorporated, All Rights Reserved
========================================================================== */
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia/qtv/utils/trackselection/main/latest/inc/itrackselectionpolicy.h#7 $
$DateTime: 2008/05/12 23:03:34 $
$Change: 658428 $

========================================================================== */
#ifndef I_TRACK_SELECTION_POLICY_H
#define I_TRACK_SELECTION_POLICY_H

/* Includes custmp4.h. The following 2 includes must be the first includes in this file! */
#include "customer.h"
#include "qtvInternalDefs.h"
#include "IReferenceCountable.h"

class ITrackList;

class ITrackSelectionPolicy : public IReferenceCountable 
{

public:
  ITrackSelectionPolicy() { }
  virtual ~ITrackSelectionPolicy() { }

  // Call SelectTrack() on the track list appropriately.
  //
  // If reselect is true, the previous track selection is saved in the track
  // list and must be cleared if necessary. The meaning of reselect == true
  // is that some change has occurred (such as Notify() returning true) that
  // requires the policy to reselect the tracks while the current clip is still
  // playing.
  //
  // Otherwise, all tracks are initially not selected.
  virtual void SelectTracks(ITrackList *trackList /* in */, 
                            bool reselect) = 0;

  // Notify of an event. Return true if track selection should be redone.
  virtual bool Notify(int eventType, void *value) = 0;

  // Associated value is a MediaSource. 
  static const int EVENT_TYPE_MEDIA_SOURCE_TYPE = 0;

  // Associated value is a sys_sys_mode_e_type.
  static const int EVENT_TYPE_SYS_MODE = 1;

  // Associated value is an int32 indicating the measured bitrate in bps. 
  static const int EVENT_TYPE_MEASURED_BITRATE = 2;

  // Associated value is an int32 indicating the granted bandwidth in bps. 
  static const int EVENT_TYPE_GRANTED_QOS_BANDWIDTH = 3;
};
#endif /* I_TRACK_SELECTION_POLICY_H */

