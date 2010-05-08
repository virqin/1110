#ifndef _BT_QSOC_NVM_BTS_H
#define _BT_QSOC_NVM_BTS_H

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

       B L U E T O O T H    B T S    D R I V E R    H E A D E R    F I L E

GENERAL DESCRIPTION
  This module contains extern declaration of NVM strings. 
  
EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2008 QUALCOMM Incorporated. 
All Rights Reserved. 
Qualcomm Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

 $Header: 
 $DateTime: 
*   #2         17 Dec 2008           NS
*   Added FEATURE_BT_QSOC_BTS_* defines/
*   #1         11 Nov  2008          SA
*   Created new file for NV automation support.

===========================================================================*/

/*===========================================================================

                         INCLUDE FILES FOR MODULE

===========================================================================*/
#include "customer.h"

#ifdef FEATURE_BT_QSOC_ALL
       #define FEATURE_BT_QSOC_BTS4020_R3
       #define FEATURE_BT_QSOC_BTS4021_B1
       #define FEATURE_BT_QSOC_BTS4020_BDB0
       #define FEATURE_BT_QSOC_BTS4020_BDB1
       #define FEATURE_BT_QSOC_BTS4025_B0
       #define FEATURE_BT_QSOC_BTS4025_B1
#endif /* FEATURE_BT_QSOC_ALL */

extern const void * bt_qsoc_nvm_BTS4025_B1_32Mhz;
extern const void * bt_qsoc_nvm_BTS4020_BDB0_19P2Mhz;
extern const void * bt_qsoc_nvm_BTS4020_R3_19P2Mhz;
extern const void * bt_qsoc_nvm_BTS4020_R3_32Mhz;
extern const void * bt_qsoc_nvm_BTS4020_BDB0_19P2Mhz;
extern const void * bt_qsoc_nvm_BTS4020_BDB0_32Mhz;
extern const void * bt_qsoc_nvm_BTS4020_BDB1_19P2Mhz;
extern const void * bt_qsoc_nvm_BTS4020_BDB1_32Mhz;
extern const void * bt_qsoc_nvm_BTS4021_B1_19P2Mhz;
extern const void * bt_qsoc_nvm_BTS4021_B1_32Mhz;
extern const void * bt_qsoc_nvm_BTS4025_B0_19P2Mhz;
extern const void * bt_qsoc_nvm_BTS4025_B0_32Mhz;
extern const void * bt_qsoc_nvm_BTS4025_B1_19P2Mhz;
extern const void * bt_qsoc_nvm_BTS4025_B1_32Mhz;

#endif /* _BTQSOCNVM_PF_DEF_H */
