#ifndef GSTK_S_GSDI_SIG_WAIT_C
#define GSTK_S_GSDI_SIG_WAIT_C


/*===========================================================================


            G S T K   G S D I   S I G N A L   W A I T



GENERAL DESCRIPTION
  This source file contains function to handle event for GSTK_S_GSDI_SIG_WAIT
  state.


FUNCTIONS
  gstk_state_gsdi_sig_wait
    This function handles events/commands/signal for GSTK_GSDI_SIG_WAIT_S


INITIALIZATION AND SEQUENCING REQUIREMENTS

===========================================================================*/

/*===========================================================================
                        COPYRIGHT INFORMATION

Copyright (c) 2003-2006,2008 QUALCOMM, Incorporated and its licensors.  All Rights
Reserved.  QUALCOMM Proprietary.  Export of this technology or software
is regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.



$Header: //source/qcom/qct/modem/uim/gstk/rel/07H1_2/src/gstk_s_gsdi_sig_wait.c#1 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
09/08/08   sk      Fixed indentation
09/02/08   sk      Removed Featurization FEATURE_GSTK_SINGLE_CLIENT_RAW_FMT
08/21/08   sk      Added changes to support enhanced GSTK registration
08/08/08   xz      Add support for raw format registration
11/07/07   sk      Get card type from mmgsdi on receing ok for tp dl
08/25/06   sk      Featurized uim recovery
07/25/06   sk      Added support to handle envelope cmds in non idle state
07/12/06   sk      During UIM recovery, if TP dl was not done prior to 
                   receiving this cmd, return TP dl done to GSDI 
06/13/06   sk      UIM recovery support
06/09/06   sk      Lint fixes
04/04/06   sk      Added support for 1X related to location information
11/09/05   sk      Fixed Lint Errors
09/20/05   tml     RRC callback registration and hanlding to allow proper
                   cell id caching and location status event when cell ID 
                   changed during normal service
07/29/05   sk      Fixed compilation errors.
01/07/05   jk      Fixed Compiler warnings
08/30/04   tml     Go to No SIM state if received SIM_INIT_COMPLETED before
                   other TP related events
05/06/04   tml     notify GSDI upon receiving Not OK for TP DL in single slot
                   solution
03/02/04   tml     Decouple Slot 1 and Slot 2 toolkit support
09/04/03   tml     Dual slot support
05/26/03   tml     linted
02/19/03   jar     Changed MSG_HIGH GSDI_SIG_WAIT in gstk_state_gsdi_sig_wait
                   to GSTK_GSDI_SIG_WAIT.
02/13/03   tml     Added/Changed DM messages
02/07/03   tml     Initial Version


===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "gstk_state.h"
#include "gstk_term_profile.h"
#include "gstk_envelope_cmd.h"

#include "gsdi_exp.h"
#include "err.h"
#ifdef FEATURE_GSTK_DUAL_SLOTS 
#include "gstkutil.h"
#endif /* FEATURE_GSTK_DUAL_SLOTS */

#ifdef FEATURE_GSTK_DUAL_SLOTS
/*===========================================================================
FUNCTION gstk_process_gsdi_evt_in_gsdi_sig_wait_state

DESCRIPTION

  This function checks the toolkit capability of the card, set the gstk_state
  appropriately and determine if a TP is ready to be sent to the card

PARAMETERS
  None

DEPENDENCIES
  None

RETURN VALUE
  gstk_status_enum_type

COMMENTS
  None

SIDE EFFECTS
  None

SEE ALSO
  None
===========================================================================*/
void gstk_process_gsdi_evt_in_gsdi_sig_wait_state(void) 
{
  gstk_toolkit_cap_enum_type    toolkit_cap = GSTK_UNKNOWN_TK_CAPABLE;

  gstk_state_determine_action_after_gsdi_evt(&toolkit_cap);

  switch (toolkit_cap) {
  case GSTK_UNKNOWN_TK_CAPABLE:
    /* remain in GSDI_SIG_WAIT */
    break;
  case GSTK_NO_TK_CAPABLE:
    gstk_state = GSTK_NO_SIM_S;
    break;
  case GSTK_BOTH_TK_CAPABLE:
  case GSTK_ONE_TK_CAPABLE:
    gstk_start_toolkit_session();
    break;
  }
} /* gstk_process_gsdi_evt_in_gsdi_sig_wait_state */
#endif /*FEATURE_GSTK_DUAL_SLOTS */

/*===========================================================================
FUNCTION gstk_state_gsdi_sig_wait

DESCRIPTION

  This function handles events/commands/signal for GSTK_GSDI_SIG_WAIT_S
  GSTK_GSDI_SIG_WAIT_S may receive any of the following events:
  - SIM_INSERTED from GSDI -> do nothing
  - sim errors from GSDI -> transition to GSTK_NO_SIM_S
  - GSDI_OK_TP_DL from GSDI -> transition to GSTK_TERM_PROFILE_RSP_WAIT_S
  - GSDI_NOT_OK_TP_DL from GSDI -> transition to GSTK_NO_SIM_S
  - task stop sig from TMC -> transition to GSTK_POWER_DOWN_S

PARAMETERS
  cmd_ptr: [Input] Pointer to the command to be processed in this state

DEPENDENCIES
  None

RETURN VALUE
  gstk_status_enum_type

COMMENTS
  None

SIDE EFFECTS
  None

SEE ALSO
  None
===========================================================================*/
gstk_status_enum_type gstk_state_gsdi_sig_wait (
  const gstk_cmd_type            * cmd_ptr )
{
  gstk_status_enum_type     gstk_status   = GSTK_SUCCESS;

  MSG_HIGH("GSTK_GSDI_SIG_WAIT", 0, 0, 0);

  if(cmd_ptr == NULL)
  {
    MSG_ERROR("cmd_ptr ptr:NULL",0,0,0);
    return GSTK_BAD_PARAM;
  }

  switch( cmd_ptr->general_cmd.message_header.command_group ) {
#ifdef FEATURE_GSTK_ENH_MULTI_CLIENT_SUPPORT
#error code not present
#endif /* FEATURE_GSTK_ENH_MULTI_CLIENT_SUPPORT */
  case GSTK_GSDI_CMD:
    MSG_HIGH("Received GSDI, %x", cmd_ptr->gsdi_evt.command_id, 0, 0);
    switch( cmd_ptr->gsdi_evt.command_id ) {
    case GSDI_SIM_REMOVED:
    case GSDI_NO_SIM:
    case GSDI_DRIVER_ERROR:
    case GSDI_CARD_ERROR:
#ifndef FEATURE_GSTK_DUAL_SLOTS
    case GSDI_SIM_INIT_COMPLETED:
    case GSDI_SIM_INIT_COMPLETED_NO_PROV:
      gstk_state = GSTK_NO_SIM_S;
#else
      /* update the mask */
      gstk_toolkit_info |= GSTK_SLOT1_CARD_ERROR_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT1_INITIALIZED_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT1_TK_CAPABLE_MASK;

      gstk_process_gsdi_evt_in_gsdi_sig_wait_state();

#endif /* FEATURE_GSTK_DUAL_SLOTS */
      break; /* end - case GSDI_CARD_ERROR, GSDI_DRIVER_ERROR, GSDI_NO_SIM,
                ** GSDI_SIM_REMOVED */

#ifdef FEATURE_GSTK_DUAL_SLOTS
    case GSDI_CARD_ERROR_2:
      /* update the mask */
      gstk_toolkit_info |= GSTK_SLOT2_CARD_ERROR_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT2_INITIALIZED_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT2_TK_CAPABLE_MASK;

      gstk_process_gsdi_evt_in_gsdi_sig_wait_state();

      break; /* end - case GSDI_CARD_ERROR_2 */
#endif /* FEATURE_GSTK_DUAL_SLOTS */


    case GSDI_NOT_OK_FOR_TERMINAL_PROFILE_DL: /* no TP is allowed/required */
#ifndef FEATURE_GSTK_DUAL_SLOTS
      gstk_state = GSTK_NO_SIM_S;
#else
      /* update the mask */
      gstk_toolkit_info |= GSTK_SLOT1_INITIALIZED_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT1_TK_CAPABLE_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT1_CARD_ERROR_MASK;
      gstk_process_gsdi_evt_in_gsdi_sig_wait_state();
#endif /* FEATURE_GSTK_DUAL_SLOTS */

      /* notify GSDI received TP DL */
      gsdi_util_gstk_tp_dl_complete ();
      break; /* end - case GSDI_NOT_OK_FOR_TERMINAL_PROFILE_DL */


#ifdef FEATURE_GSTK_DUAL_SLOTS
    case GSDI_NOT_OK_FOR_TERMINAL_PROFILE_DL_2: /* no TP is allowed/required */
      /* update the mask */
      gstk_toolkit_info |= GSTK_SLOT2_INITIALIZED_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT2_TK_CAPABLE_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT2_CARD_ERROR_MASK;

      gstk_process_gsdi_evt_in_gsdi_sig_wait_state();
      /* notify GSDI received TP DL */
      gsdi_util_gstk_tp_dl_complete ();

      break; /* end - case GSDI_NOT_OK_FOR_TERMINAL_PROFILE_DL_2 */
#endif /* FEATURE_GSTK_DUAL_SLOTS */

    case GSDI_OK_FOR_TERMINAL_PROFILE_DL:
#ifndef FEATURE_GSTK_DUAL_SLOTS
      gstk_get_and_store_curr_app_type();
      /* use the gstk_terminal_profile_cache info and send it to uim */
      gstk_status = gstk_pack_tp_cmd_to_uim();
      if (gstk_status == GSTK_SUCCESS) 
      {
        /* go to wait for TP response state */
        gstk_state = GSTK_TERM_PROFILE_RSP_WAIT_S;
      }
      else 
      {
        ERR("Send TP error", 0, 0, 0);
      }
#else
      /* update the mask */
      gstk_toolkit_info |= GSTK_SLOT1_INITIALIZED_MASK;
      gstk_toolkit_info |= GSTK_SLOT1_TK_CAPABLE_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT1_CARD_ERROR_MASK;

      gstk_process_gsdi_evt_in_gsdi_sig_wait_state();

#endif /* FEATURE_GSTK_DUAL_SLOTS */
      break;   /* end - case GSDI_OK_FOR_TERMINAL_PROFILE_DL */


#ifdef FEATURE_GSTK_DUAL_SLOTS
    case GSDI_OK_FOR_TERMINAL_PROFILE_DL_2:
      /* update the mask */
      gstk_toolkit_info |= GSTK_SLOT2_INITIALIZED_MASK;
      gstk_toolkit_info |= GSTK_SLOT2_TK_CAPABLE_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT2_CARD_ERROR_MASK;

      gstk_process_gsdi_evt_in_gsdi_sig_wait_state();

      break; /* end - case GSDI_OK_FOR_TERMINAL_PROFILE_DL_2 */

    /* SIM inserted event in GSDI_SIG_WAIT state will only happen in
     * dual slot solution */
    case GSDI_SIM_INSERTED: 
      /* update the mask */
      gstk_toolkit_info &= ~GSTK_SLOT1_INITIALIZED_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT1_CARD_ERROR_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT1_TK_CAPABLE_MASK;
      break; /* end - case GSDI_SIM_INSERTED */

    case GSDI_SIM_INSERTED_2: /* for refresh/reset command etc */
      /* update the mask */
      gstk_toolkit_info &= ~GSTK_SLOT2_INITIALIZED_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT2_CARD_ERROR_MASK;
      gstk_toolkit_info &= ~GSTK_SLOT2_TK_CAPABLE_MASK;
      break; /* end - case GSDI_SIM_INSERTED_2 */

#endif /* FEATURE_GSTK_DUAL_SLOTS */
#ifndef FEATURE_NO_UIM_RECOVERY_SUPPORT
    case GSDI_INTERNAL_SIM_RESET:
      /* Notify GSDI that TP dl is complete */
         gsdi_util_gstk_tp_dl_complete ();
      break;
#endif /* FEATURE_NO_UIM_RECOVERY_SUPPORT */
    default:
      break; /* ignore all other gsdi_sim_event_T */
    }
    break;

  case GSTK_STOP_CMD:
    MSG_HIGH("Received STOP Sig", 0, 0, 0);
    /* flush GSTK queue, clean up and ack tmc */
    gstk_status = gstk_power_down();
    if (gstk_status == GSTK_SUCCESS) 
    {
      gstk_state = GSTK_POWER_DOWN_S;
    }
    else 
    {
      MSG_MED("Power Down err", 0, 0, 0);
    }
    break;

  case GSTK_MM_IDLE_CMD:
    MSG_MED("Ignore MM IDLE CMD", 0, 0, 0);
    /* gstk_mm_state_is_idle flag is used  as an indication to start queuing RRC messages */ 
    gstk_mm_state_is_idle = TRUE;
    break;

  case GSTK_MODE_CHANGE_IND:

#ifdef FEATURE_GSTK_DUAL_SLOTS
    gstk_util_handle_mode_change(cmd_ptr->mode_change_ind.cmd.new_mode);
#endif /* FEATURE_GSTK_DUAL_SLOTS */       

#ifdef FEATURE_CAT_REL6
#ifdef FEATURE_CCAT
    if(cmd_ptr->mode_change_ind.cmd.access_technology == GSTK_ACCESS_TECH_CDMA)
    {
      gstk_cache_1x_location_info(cmd_ptr->mode_change_ind.cmd.cdma_loc_info);
    }
#else  /* #ifdef FEATURE_CCAT */
    MSG_HIGH("GSTK_MODE_CHANGE_IND : Feature CCAT not supported",0,0,0);
#endif /*#ifdef FEATURE_CCAT */
#else  /* #ifdef FEATURE_CAT_REL6 */
    MSG_HIGH("GSTK_MODE_CHANGE_IND : FEATURE_CAT_REL6 not supported",0,0,0);
#endif /* #ifdef FEATURE_CAT_REL6 */
    break;   

  case GSTK_ENVELOPE_CMD:
    MSG_HIGH("Received Envelope cmd", 0, 0, 0);
    gstk_status = gstk_process_non_idle_state_envelope_cmd(cmd_ptr);
    if (gstk_status != GSTK_SUCCESS) 
    {
      MSG_ERROR("Envelope Cmd ERR, %x", gstk_status, 0, 0);
    }
    break;

  default:
    MSG_MED("Not handled in GSTK_GSDI_SIG_WAIT_S", 0, 0, 0);
    break;
  }
  return gstk_status;
} /*gstk_state_gsdi_sig_wait*/

#endif  /* GSTK_S_GSDI_SIG_WAIT_C */
/*lint -e750 local macro gstk_c not being used */

