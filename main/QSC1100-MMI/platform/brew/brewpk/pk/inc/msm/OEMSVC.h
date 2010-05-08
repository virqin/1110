#ifndef _OEMSVC_H_
#define _OEMSVC_H_

/*======================================================
FILE:  OEMSVC.h

SERVICES:  OEM supervisor mode functions

GENERAL DESCRIPTION:

This file contains prototypes and definitions for 
access to Supervisor mode services that OEMs need
to provide in order to support BREW..

PUBLIC CLASSES:

None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
None.

        Copyright ?1999-2004 QUALCOMM Incorporated.
               All Rights Reserved.
            QUALCOMM Proprietary/GTDR
=====================================================*/
#ifndef WIN32
#include "disp.h"
#include "voc.h"
#endif
#include "OEMConfig.h"
#include "OEMNotify.h"
#include "AEEShell.h"

/*==================================================================
Function: OEM_SVCGetConfig

Description: Retrieves configuration information from the OEM

Prototype:
   int  OEM_SVCGetConfig(AEEConfigItem i, void * pBuff, int nSize)

Parameter(s):

Return Value:  None
Comments:      None
Side Effects:  None
See Also:      None

==================================================================*/
int     OEM_SVCGetConfig(AEEConfigItem i, void * pBuff, int nSize);

/*==================================================================
Function: OEM_SVCSetConfig

Description: Sets configuration information from the OEM

Prototype:
   int  OEM_SVCSetConfig(AEEConfigItem i, void * pBuff, int nSize)

Parameter(s):

Return Value:  None
Comments:      None
Side Effects:  None
See Also:      None

==================================================================*/
int     OEM_SVCSetConfig(AEEConfigItem i, void * pBuff, int nSize);

/*==============================================================
Function: OEM_SVCIsVocCapable

Description: Is the given type supported by the vocoder

==============================================================*/
#ifndef WIN32
boolean OEM_SVCIsVocCapable(voc_capability_type vocCap);
#endif
/*==================================================================
Function: OEM_SimpleBeep

Description:
   This function plays a standard OEM beep tone and vibration given
the duration, and returns TRUE if successful.

Prototype:
   boolean OEM_SimpleBeep(BeepType nBeepType, boolean bLoud);

Parameters:
   nBeepType: beep type which can be one of the following:
      - BEEP_OFF: stop playback of the current beep or vibration
      - BEEP_ALERT: alert beep tone
      - BEEP_REMINDER: reminder beep tone
      - BEEP_MSG: message beep tone
      - BEEP_ERROR: error beep tone
      - BEEP_VIBRATE_ALERT: alert vibration
      - BEEP_VIBRATE_REMIND: reminder vibration
   bLoud: boolean flag that sets the playback volume high or low

Return Value:
   TRUE if successfuly played or stopped the tone or vibration;
   FALSE otherwise/unsupported.

Comments:
   None

Side Effects:
   None

See Also:
   None
==================================================================*/
boolean OEM_SVCSimpleBeep(BeepType nBeepType, boolean bLoud);

boolean OEM_SVCUiIsInCall(void);
void    OEM_SVCUiAddEvent(OEMNotifyEvent evt);
#ifndef WIN32
/*==============================================================
Function: OEM_SVCdisp_get_info

Description: Gets Display info. from DMSS

==============================================================*/
int     OEM_SVCdisp_get_info(disp_info_type *pInfo);
#endif
#ifdef FEATURE_SECONDARY_DISPLAY
/*==============================================================
Function: OEM_SVCdisp_get_info2

Description: Gets Display info. from DMSS for secondary display

==============================================================*/
int     OEM_SVCdisp_get_info2(disp_info_type *pInfo);
#endif

#include "OEMSVC.h"
#include "AEEConfig.h"
#include "OEMConfig.h"
#include "OEMCFGI.h"
#include "OEMNV.h"
#ifndef WIN32
#include "ps_ppp_defs.h"
#else
#define PPP_MAX_USER_ID_LEN 127
#define PPP_MAX_PASSWD_LEN  127
#endif
typedef struct
{
  char  user_id_info[PPP_MAX_USER_ID_LEN];
  char  passwd_info[PPP_MAX_PASSWD_LEN];
} PppAccounts;

typedef enum 
{
   DS_NONE_TYPE = 0,
   DS_BREW_TYPE,
   DS_WAP12_TYPE,
   DS_WAP20_TYPE,
   DS_MAIL_TYPE,
   DS_JAVA_TYPE,
   DS_INTERNET_TYPE
} DataSvcType;

int OEM_SetPppAccounts(PppAccounts *pAccount, DataSvcType dsType);
int OEM_GetPppAccounts(PppAccounts *pAccount, DataSvcType dsType);

void GetMobileInfo(AEEMobileInfo * pmi);
// 在测PEK时，因目前取 CFGI_SUBSCRIBERID 时我们以MIN表示(BREW 设置亦如此)，若用GetMobileInfo返回的
// szMobileID,长度为15，此时DPK中MIN设为15位的号码，GetConfigComplex.18 测不过；若设为10位的号码，
// GetConfigComplex.25 测不过。通过分析，oat 测试软件认为对以 MIN 表示 SUBSCRIBERID ，则是 10 位号
// 码的 MIN 且与用 CFGI_MOBILEINFO 取得的 szMobileID 一致。目前，仅部分 UI 用到 15 位的 szMobileID
// 。为满足双方需要，添加一个取 10 位的函数用于系统级，原来的函数仅供需要15位的UI应用使用。
void GetMobileInfoEx(AEEMobileInfo * pmi);
// 正在活动的设备类型
typedef enum
{
    DEVICE_TYPE_MP4,
    DEVICE_TYPE_CAMERA
}DeviceType;
// 设备当前的状态
typedef enum
{
    DEVICE_MP4_STATE_ON, 
    DEVICE_MP4_STATE_OFF,
    DEVICE_CAMERA_STATE_ON,
    DEVICE_CAMERA_STATE_OFF
}DeviceStateType;

void SetDeviceState(DeviceType dt,DeviceStateType dst);
int  GetDeviceState(DeviceType dt);



#endif //_OEMSVC_H_
