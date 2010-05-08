/*=============================================================================

FILE: OEMIPL.c

SERVICES: OEM Header and Functions for Image Processing Interface.

GENERAL DESCRIPTION:

PUBLIC CLASSES AND STATIC FUNCTIONS:

INITIALIZATION AND SEQUENCING REQUIREMENTS:

        Copyright c 2003-2006 QUALCOMM Incorporated.
               All Rights Reserved.
            QUALCOMM Proprietary/GTDR
=============================================================================*/

/*=============================================================================


                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.
  
$Header: //depot/asic/msmshared/apps/StaticExtensions/OEM/Src/OEMIPL.c#36 $
$DateTime: 2006/06/14 09:37:19 $ $Author: ncai $

when         who     what, where, why
----------   ---     ---------------------------------------------------------
06/14/06     NC       Updated OEMIPL_Whiteboard per ipl API change (ipl.h#83).
06/13/06     NC       Change include file name case for L4 compiler
06/12/06     ES       Remove color conversion for IIPL_ROTATE.
06/08/06     ES       Added support to OEMIPL_CopyPaste
05/26/06     NC       Updated OEMIPL_Whiteboard per ipl API change(bring #28 back).
05/25/06     NC       Revert #28 to make later bug fix out of ipl dependency.
05/24/06     HV       Simplified OEMIPL_ConvertColor() function and removed 
                      IPL_LITE feature checking.Fixed lint errors also.
05/24/06     NC       Bug fix in function OEMIPL_ChangeSize().
05/22/06     NC       Updated OEMIPL_Whiteboard per ipl API change.
05/17/06     HV       Introduced OEMIPL_GetYCbCrIPL function and simplified 
                      OEMIPL_ChangeSize
05/16/06     HV       Modified OEMIPL_DIBToIPL and simplified OEMIPL_ChangeSize
05/15/06     HV       Added RGB888 support
05/08/06     HV       Bug fix in function OEMIPL_Filter
05/05/06     HV       Bug fix in function OEMIPL_RotateCropAddFrame
05/02/06     HV       Fixed lint errors and added the function headers to file 
04/23/06     HV       Removed redundancies with in the file.
04/13/06     HV       Replaced all interface release blocks with the macro RELEASEIF().
03/23/06     NC       Simplified OEMIPL_ChangeSize()
02/13/06     NC       Added FEATURE_IPL_EFX_COMPACT condition to red eye feature per ipl change.
02/10/06     JRR      Added featurization for FEATURE_IPL_LITE.
02/02/06     NC       Changed histogram color to improve the display quality.
12/22/05   c_gudipa   Added RGB histogram Support, New Function, for Qalbum 
11/22/05     JN       Added luma histogram support.
11/10/05     JN       Added whiteboard effect.
10/06/05     JN       Removed bytesPerPixel from OEMIPL_CreateIPL as input param.
09/16/05     JN       Coverity error clean up.
08/30/05     JN       Started to add IYCbCr support.
                      Bug fixes in ViewPerspective, GaussianBlur, UniformBlur,
                      MotionBlur.
12/01/04     ADM      Initial version 
=============================================================================*/

/*===========================================================================

                     INCLUDE FILES

===========================================================================*/
/* Lint global dont cares */
/*lint -save -e717*/
//lint -e740 suppress errors on pointer cast
//lint -e611 


#include "AEE_OEM.h"
#include "AEEStdLib.h"
#include "AEEShell.h"
#include "AEECamera.h"
#include "AEEText.h"
#include "IIPL.h"
#include "IYCbCr.h"

#include "IPL.BID"
#include "ipl.h"

/*===========================================================================*/
#ifdef _BUILD_AS_STATIC_EXTENSION_
// Define this if you are intending to build this as a static extension.
// Otherwise this implementation file can be statically built and linked
// in the target build.

#include "StdAfx.h"
#include "StaticExtensionInc.h"
#include "AEEModTable.h"

extern int OEMIPL_New(IShell *ps, AEECLSID cls, void **ppo);

// Create an AEEStaticClass array that lists the information about the interface 
const AEEStaticClass gOEMIPLExtensionClasses[] = 
{
   { AEECLSID_IPL, ASCF_UPGRADE, 0, 0, OEMIPL_New }, 
   { 0, 0, 0, 0, NULL},
};

/*===========================================================================

Function:  GetStaticClassEntries()

Description:
  This function is required to be exposed if this interface will be 
  exposed as a static extension DLL. BREW Simulator loads all the DLLs
  that expose this function as a Static Extension DLL.

Prototype:
     const AEEStaticClass* GetStaticClassEntries(void)

Parameters: None

Return Value:
  return an array of AEEStaticClass

Comments:  None

Side Effects: None

==============================================================================*/
BREWSTATICEXTENSION_API const AEEStaticClass *GetStaticClassEntries(void)
{
  return gOEMIPLExtensionClasses;
}
#endif // _BUILD_AS_STATIC_EXTENSION_

/************************************************************************
 *
 * Constant definitions
 *
 ***********************************************************************/
#define MAX_HUE_FACTOR (360)        // Maximum hue factor supported
#define MIN_HUE_FACTOR (-360)       // Minimum hue factor supported
#define MAX_SAT_FACTOR (255)        // Maximum saturation factor supported
#define MIN_SAT_FACTOR (-255)       // Minimum saturation factor supported
#define MAX_POS_FACTOR (255)        // Maximum posterization factor supported
#define MIN_POS_FACTOR (1)          // Minimum posterizationfactor supported
#define MAX_INT_FACTOR (255)        // Maximum intensity factor supported
#define MIN_INT_FACTOR (-255)       // Minimum intensity factor supported
#define MAX_SOL_FACTOR (255)        // Maximum solarization factor supported
#define MIN_SOL_FACTOR (0)          // Minimum solarization factor supported

#define RELEASEIF(p)  OEMIPL_ReleaseIF((IBase **)&(p))

/************************************************************************
 *
 * Common IIPL type definitions
 *
 ***********************************************************************/
typedef struct OEMIPL OEMIPL;

AEEINTERFACE(OEMIPL)
{
  INHERIT_IIpl(OEMIPL);
};

struct OEMIPL
{
  const AEEVTBL(OEMIPL)   *m_pvt;            // Vector table of IIPL methods
  IShell                  *m_pIShell;        // Pointer to the IShell object
  uint32                  m_uRefs;          // Reference count
};

/************************************************************************
 *
 *      Interface Function Prototypes
 *
 ***********************************************************************/

static uint32 OEMIPL_AddRef(OEMIPL *po);

static uint32 OEMIPL_Release(OEMIPL *po);

static int OEMIPL_QueryInterface(OEMIPL *po, AEECLSID clsid, void **ppNew);

static void OEMIPL_ReleaseIF(IBase **ppif);

static int OEMIPL_Transform(OEMIPL *po, IBitmap *i_image, IBitmap *o_image, 
                            IIPLTransformType xform, uint32 xFormValue);

static int OEMIPL_ConvertColor(OEMIPL *po, IBitmap *i_image, IBitmap *o_image);

static int  OEMIPL_Effect(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                          IIPLEffectType effect, int32 effectValue);

static int  OEMIPL_Compose(OEMIPL *po, IBitmap *i_image, IBitmap *i_image2,
                           IBitmap *o_image, IIPLComposeType p1, int32 boundary);

static int  OEMIPL_CopyPaste(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                             AEERect *rcCut, AEERect *rcPaste);

static int  OEMIPL_RotateFrame(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                               IIPLRotateType p1);

static int  OEMIPL_ArbitRotate(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                               int16 angle, uint16 fillerPixel);

static int  OEMIPL_ConvertFrame(OEMIPL *po, IBitmap *i_frame, IBitmap *o_frame);

static int  OEMIPL_Filter(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                          IIPLFilterType filter, uint32 blurWindow);

static int  OEMIPL_ComposeCanvas(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                                 IIPLIconInfo *icon_list, uint32 listSize);

static int  OEMIPL_RotateCropAddFrame(OEMIPL *po, IBitmap *i_image, IBitmap *i_frame,
                                      IBitmap *o_image, IIPLIconInfo *icon_list,
                                      uint32 listSize, AEERect *cropRect, 
                                      IIPLRotateType rotate);

static int  OEMIPL_ChangeSize(OEMIPL *po, IBitmap *i_image, IBitmap *o_image);

static int  OEMIPL_ViewPerspective (OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                                    IIPLLine *lineIn, uint16 fillerPixel,
                                    IIPLPerspectiveType orientation);

static int  OEMIPL_Magnify (OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                            IIPLCircle *circleIn);

static int  OEMIPL_MotionBlur(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                              uint32 angle, uint32 blurWindow);

static int  OEMIPL_RadialBlur(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                              uint32 level);

static int  OEMIPL_Overlap(OEMIPL *po, IBitmap *i_image, IBitmap *i_image2,
                           IBitmap *o_image, AEERect *rcOverlap);

static int  OEMIPL_Warp(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                        IIPLGrid *grid, boolean gridLoc);

static int  OEMIPL_CutPaste(OEMIPL *po, IBitmap *i_image, IBitmap *o_image,
                            AEERect *rcCut, AEERect *rcPaste, uint16 fillerPixel);

static int  OEMIPL_DrawHistogram(OEMIPL *pIIpl,IBitmap *inImage, IBitmap *outImage,
                                 AEERect *rect);

static int  OEMIPL_DrawRgbHistogram(OEMIPL *pIIpl,IBitmap *inImage,
                                    IBitmap *outImage, AEERect *rect);

static int  OEMIPL_RemoveRedEye(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                                AEERect rcCut, AEERect rcPaste);

/************************************************************************
 *
 *            Internal Functions 
 *
 ***********************************************************************/
static int OEMIPL_Grayscale(const ipl_image_type *iIpl, ipl_image_type *oIpl);

static int OEMIPL_Hue(const ipl_image_type *iIpl, const ipl_image_type *oIpl, int32 hue_factor);

static int OEMIPL_Negative(const ipl_image_type *iIpl, ipl_image_type *oIpl);

static int OEMIPL_Saturation(const ipl_image_type *iIpl, const ipl_image_type *oIpl,
                             int32 sat_factor);

static int OEMIPL_Sepia(const ipl_image_type *iIpl, ipl_image_type *oIpl);

static int OEMIPL_Posterize(const ipl_image_type *iIpl, ipl_image_type *oIpl,
                            int32 pos_factor);

static int OEMIPL_Intensity(const ipl_image_type *iIpl, const ipl_image_type *oIpl,
                            int32 int_factor);

static int OEMIPL_Flash(const ipl_image_type *iIpl, ipl_image_type *oIpl);

static int OEMIPL_Whiteboard(ipl_image_type *iIpl, ipl_image_type *oIpl,
                             int32 threshold);

/************************************************************************
 *
 *            Helper Functions
 *
 ***********************************************************************/
static int OEMIPL_GetIPLs(IBitmap *inImg, IBitmap *outImg, ipl_image_type **iIpl,
                          ipl_image_type **oIpl, boolean bCheckSize, boolean bDIB);

static int OEMIPL_GetIPL(IBitmap *inImg, ipl_image_type **iIpl);

static int OEMIPL_RGB2YCbCrIPL(const ipl_image_type *iIpl, ipl_image_type **oIpl);

static int OEMIPL_DIBToIPL(const IDIB *pDIB, ipl_image_type **inIpl);

static int OEMIPL_YCbCrToIPL(const IYCbCr *pYCbCr, ipl_image_type **inIpl);

static int OEMIPL_CreateIPL(ipl_image_type **outIpl, uint16 width, uint16 height, 
                            ipl_col_for_type  colorFormat);

static void OEMIPL_FreeIPLTypeIF(ipl_image_type *inIpl);

static void OEMIPL_FreeIPLDataIF(ipl_image_type *inIpl);

static int OEMIPL_GetYCbCrIPL(IBitmap *inImg, ipl_image_type **iIpl);

// gvtOEMIPLFuncs is the virtual table for the IIpl object
/*lint --e{754}*/
static const VTBL(OEMIPL) gvtOEMIPLFuncs = 
{
  OEMIPL_AddRef,
  OEMIPL_Release,
  OEMIPL_QueryInterface,
  OEMIPL_Transform,
  OEMIPL_ConvertColor,
  OEMIPL_Effect,
  OEMIPL_Compose,
  OEMIPL_CopyPaste,
  OEMIPL_RotateFrame,
  OEMIPL_ArbitRotate,
  OEMIPL_ConvertFrame,
  OEMIPL_Filter,
  OEMIPL_ComposeCanvas,
  OEMIPL_RotateCropAddFrame,
  OEMIPL_ChangeSize,
  OEMIPL_ViewPerspective,
  OEMIPL_Magnify,
  OEMIPL_MotionBlur,
  OEMIPL_RadialBlur,
  OEMIPL_Overlap,
  OEMIPL_Warp,
  OEMIPL_CutPaste,
  OEMIPL_DrawHistogram,
  OEMIPL_DrawRgbHistogram,
  OEMIPL_RemoveRedEye
};

/*===========================================================================

FUNCTION: OEMIPL_New

DESCRIPTION
  Creates an instance of the IIpl class.  Only one instance of the IIpl
  class can exist at any given time. So, if an instance already exists.
  then the create instance operation will simply return a pointer to the 
  existing object.

DEPENDENCIES
  None

RETURN VALUE
  Returns AEE return values

  AEE_SUCCESS:  Registration was successful
  EBADPARM:     One of the arguments was invalid (e.g. NULL pointer)
  ENOMEMORY:    There was not enough memory to create the object
  EFAILED:      An unspecified error occurred; IIpl object not created

SIDE EFFECTS
  None

===========================================================================*/
int OEMIPL_New(IShell *pIShell, AEECLSID cls, void **ppif)
{
  OEMIPL *pIIpl;
  if ((!pIShell) || (!ppif))
  {
    return EBADPARM;
  }

  *ppif = NULL;

  if (cls != AEECLSID_IPL) 
    return ECLASSNOTSUPPORT;
 
  pIIpl = (OEMIPL*)MALLOC(sizeof(OEMIPL));
  if (!pIIpl)
  {
    return ENOMEMORY;
  }

  pIIpl->m_pvt = &gvtOEMIPLFuncs;
  /* Set ref count to one, since we just created it.*/
  pIIpl->m_uRefs = 1;
  // Set shell pointer
  pIIpl->m_pIShell = pIShell;
  (void)ISHELL_AddRef(pIIpl->m_pIShell);
   
  *ppif = pIIpl;

  return AEE_SUCCESS;
}

/*===========================================================================

FUNCTION: OEMIPL_AddRef

DESCRIPTION
  Increments the reference count for the IIpl object.

DEPENDENCIES
  None

RETURN VALUE
  Returns the value of the incremented reference count

SIDE EFFECTS
  None

===========================================================================*/
static uint32 OEMIPL_AddRef(OEMIPL *pMe)
{
  return ++(pMe->m_uRefs);
}

/*===========================================================================

FUNCTION: OEMIPL_Release

DESCRIPTION
  Decrements the reference count for the IIpl object.  When the
  reference count drops to zero, it deletes the object.

DEPENDENCIES
  None

RETURN VALUE
  Returns the decremented reference count for the object

SIDE EFFECTS
  None

===========================================================================*/
static uint32 OEMIPL_Release(OEMIPL *pMe)
{
  // If the object doesn't exist, return 0
  if (pMe->m_uRefs == 0)
    return 0;
 
  // If there are still references after decrementing, return ref count
  if (--(pMe->m_uRefs) > 0)
    return (uint32)(pMe->m_uRefs);

  (void)ISHELL_Release(pMe->m_pIShell);
  FREE(pMe);

  return 0;
}

/*===========================================================================

FUNCTION: OEMIPL_QueryInterface

DESCRIPTION
  Allows applications to query if IIpl supports other interfaces and then
  returns a pointer to that new interface supported via IIpl.  Currently,
  we do not support any other interface.

DEPENDENCIES
  none

RETURN VALUE
  none

SIDE EFFECTS
===========================================================================*/

static int OEMIPL_QueryInterface(OEMIPL *pMe, AEECLSID id, void **ppNew)
{
  int ret_val = AEE_SUCCESS;

  if (!pMe || !ppNew) 
  {
    return EBADPARM;
  }

  switch (id) 
  {
  case AEECLSID_QUERYINTERFACE:
  case AEECLSID_IPL:
  case AEEIID_IPL:
    if (OEMIPL_AddRef(pMe))
    {
      *ppNew = pMe;
    }
    else 
    {
      *ppNew = NULL;
      ret_val = ECLASSNOTSUPPORT;
    }
    break;
  default:
    *ppNew = NULL;
    ret_val = ECLASSNOTSUPPORT;
    break;
  }

  return ret_val;
}

/*===========================================================================
FUNCTION OEMIPL_ReleaseIF

  DESCRIPTION
    This function releases the pointer and set it to NULL if it is not NULL.
  
  PARAMETERS:
    ppif - Pointer to (IBase *), replaced by any BREW interface pointer
  
  RETURN VALUE:
   None
===========================================================================*/
static void OEMIPL_ReleaseIF(IBase **ppif)
{
  if (ppif && *ppif)
  {
    (void)IBASE_Release(*ppif);
  }
  if (ppif)
    *ppif = NULL;
}



/*=============================================================================
  
                    SPECIAL EFFECTS

=============================================================================*/

/*=============================================================================

OEMIPL_Magnify()

Description:
    This function magnifies a circular area of the input image. This area will appear
    as being viewed under a magnifying glass.


Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EBADPARM, if any of the parameters are invalid , the circle center has negative co-prdinates
              or radius is less than 1.
    EUNSUPPORTED, if the current operation is not supported
    ENOMEMORY, if there isn't enough memory for the operation.
    EFAILED, in any other failure situation.

=============================================================================*/
static int OEMIPL_Magnify(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                          IIPLCircle *circleIn)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_circle_type *circleIpl = NULL;

  /*lint -save -e715 -e818*/
  // Check for validity of the input pointers
  if (!pIIpl || !inImage || !outImage || !circleIn)
    return EBADPARM;

  // Check to see if it is a valid circle
  if(circleIn->cX < 0 || circleIn->cY < 0 || !(circleIn->radius > 0))
    return EBADPARM;

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
    if (retVal)
      break;

    // Create the ipl circle type
    circleIpl = (ipl_circle_type *) MALLOC(sizeof(ipl_circle_type));
    if(circleIpl)
    {
      circleIpl->x = (uint16)circleIn->cX;
      circleIpl->y = (uint16)circleIn->cY;
      circleIpl->r = circleIn->radius;
    }
    else
    {
      retVal = ENOMEMORY;
      break;
    }

    //Call the magnify function
    if(ipl_magnify(iIpl, oIpl, circleIpl))
    {
      retVal = EFAILED;
      break;
    }
    retVal = AEE_SUCCESS;
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  FREEIF(circleIpl);
  circleIpl = NULL;
  return retVal;
}
  
/*lint -restore*/

/*=============================================================================

OEMIPL_ChangeSize()

Description:
    This function is used to change the size of the input image. 

    The scaling of the image can be arbitrary, i.e. we can increase the width and decrease the height or vice-versa

Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully

See Also:

=============================================================================*/
static int OEMIPL_ChangeSize (OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_image_type *tempIpl = NULL;   
  uint16 cx, cy;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage)
    return EBADPARM;

  do
  {
    if(OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, FALSE, TRUE) != AEE_SUCCESS)
    {
      if(OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, FALSE, FALSE) != AEE_SUCCESS)
      {
        retVal = EUNSUPPORTED;
        break;
      }
    }

    //If both images have the same dimensions, simply copy input to output
    if ((oIpl->dx == iIpl->dx) && (oIpl->dy == iIpl->dy))
    {
      switch (iIpl->cFormat)
      {
      case IPL_RGB565:
      case IPL_YCbCr:
      case IPL_YCbCr422_LINE_PK:
        (void)MEMCPY(oIpl->imgPtr, iIpl->imgPtr, (iIpl->dx * iIpl->dy * 2));
        break;
      case IPL_RGB888:
        (void)MEMCPY(oIpl->imgPtr, iIpl->imgPtr, (iIpl->dx * iIpl->dy * 3));
        break;
      case IPL_YCbCr420_LINE_PK:
        (void)MEMCPY(oIpl->imgPtr, iIpl->imgPtr, (iIpl->dx * iIpl->dy * 3) / 2);
        break;
      default:
        retVal = EUNSUPPORTED;
      }
      break;
    }

    //If both cx and cy of output are not smaller than the dimension of
    //input image, Do simple upsize
    if((oIpl->dx >= iIpl->dx) && (oIpl->dy >= iIpl->dy))
    {
      if (ipl_upsize(iIpl, oIpl, NULL, IPL_QUALITY_MEDIUM))
      {
        retVal = EFAILED;
      }
      break;
    }

    //If both cx and cy of output are not larger than the dimension of 
    //input image, Do simple downsize
    if((oIpl->dx <= iIpl->dx) && (oIpl->dy <= iIpl->dy))
    {
      if(ipl_downsize(iIpl, oIpl, NULL))
      {
        retVal = EFAILED;
      }
      break;
    }

    //If one output size larger than input but the other smaller,
    //Need to upsize first then downsize.
    if((oIpl->dx > iIpl->dx) || (oIpl->dy > iIpl->dy))
    {
      //Get larger cx and cy
      cx = (oIpl->dx > iIpl->dx) ? (uint16)oIpl->dx : (uint16)iIpl->dx;
      cy = (oIpl->dy > iIpl->dy) ? (uint16)oIpl->dy : (uint16)iIpl->dy;

      //Create a tempIpl with larger cx and cy
      if(OEMIPL_CreateIPL(&tempIpl, cx, cy, iIpl->cFormat))
      {
        retVal = EFAILED;
        break;
      }
      
      //Upsize input image to tempIpl so smaller input size is handled
      if (ipl_upsize(iIpl, tempIpl, NULL, IPL_QUALITY_MEDIUM))
      {
        retVal = EFAILED;
        break;
      }
      //Downsize tempIpl to output so smaller output size is handled
      if (ipl_downsize(tempIpl, oIpl, NULL))
      {
        retVal = EFAILED;
        break;
      }
    }
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  OEMIPL_FreeIPLDataIF(tempIpl);

  return retVal;
}
/*lint -restore*/


/*=============================================================================

OEMIPL_Filter()

Description:
    This function filters the input image. There are four types of filters that can be applied to 
    an input image.

    1) BLUR : It'll result in the blurring of the input image
    2) SHARP: It'll result in the sharpening of the input image
    3) GAUSSIAN : It'll add a gaussian blur to the input image
    4) UNIFORM : It'll add a uniform blur to the input image  

    The blurring is done in incrementally. The blur window specifies the rectangular area 
    which is blurred at a time. The blur window can range from 
    [ 3 .. 0.5 * min (input height, input width)]

    For BLUR and SHARP, the blur Window size is restricted to 3.

Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EBADPARM, if the filter type is not one of the permitted values or the blur Window 
              falls outside the range

See Also:

=============================================================================*/
static int OEMIPL_Filter (OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage, 
                          IIPLFilterType filter_type, uint32 blurWindow)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_image_type *tempIpl = NULL;
  uint32 maxDimension = 0;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage)
    return EBADPARM;

  if(filter_type > (IIPL_MAX_FILTER_VAL - 1))
    return EBADPARM;

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
    if (retVal)
      break;

    if(iIpl->dx > iIpl->dy)
    {
      if(((uint32)iIpl->dx/2)%2)
        maxDimension = (uint32)iIpl->dx/2;
      else
        maxDimension = (uint32)iIpl->dx/2 + 1;
    }
    else
    {
      if(((uint32)iIpl->dy/2)%2)
        maxDimension = (uint32)iIpl->dy/2;
      else
        maxDimension = (uint32)iIpl->dy/2 + 1;
    }

    switch(filter_type)
    {
    case IIPL_BLUR:
      if(ipl_filter_image(iIpl, oIpl, IPL_BLUR, 3))
        retVal = EFAILED;
      break;
    case IIPL_BLUR_GAUSSIAN:
      if((blurWindow%2) && (blurWindow < maxDimension))
        {
          if(ipl_filter_image(iIpl, oIpl, IPL_BLUR_GAUSSIAN, blurWindow))
            retVal = EFAILED;
        }
        else
          retVal = EBADPARM;
      break;
    case IIPL_BLUR_UNIFORM:
      if((blurWindow%2) && (blurWindow < maxDimension))
        {
          if(ipl_filter_image(iIpl, oIpl, IPL_BLUR_UNIFORM, blurWindow))
            retVal = EFAILED;
        }
        else
          retVal = EBADPARM;
      break;
    case IIPL_SHARP:
      retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
        if (retVal == AEE_SUCCESS)
        {
          if(ipl_filter_image(tempIpl, oIpl, IPL_SHARP,3))
            retVal = EFAILED;
        }
      break;
    default:
      break;
    }
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  OEMIPL_FreeIPLDataIF(tempIpl);

  return retVal;
}
/*lint -restore*/

/*=============================================================================

OEMIPL_Overlap()

Description:
  This function overlaps one image on top of another. The overlap region may 
  be placed anywhere as long as it fits within the boundaries of the first 
  input image. If the overlap region is smaller than the second input image, 
  the second image will be downsized. The output image must have the same 
  dimensions as the first input image. 

Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EBADPARM, if the overlap region is not within the input image area
    
See Also:
    None

=============================================================================*/
static int OEMIPL_Overlap(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *inImage2,
                          IBitmap *outImage, AEERect *rect)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_image_type *iIpl2 = NULL;
  ipl_rect_type *rectIpl = NULL;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !inImage2 || !outImage || !rect)
    return EBADPARM;

  if((rect->x < 0) || (rect->y < 0) || (rect->dx < 0) || (rect->dy < 0))
    return EBADPARM;

  rectIpl = (ipl_rect_type *)MALLOC(sizeof(ipl_rect_type));
  if(!rectIpl)
    return ENOMEMORY;
  else
  {
    rectIpl->x = (uint16)rect->x;
    rectIpl->y = (uint16)rect->y;
    rectIpl->dx = (uint16)rect->dx;
    rectIpl->dy = (uint16)rect->dy;
  }
  
  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
    if (retVal)
      break;

    retVal = OEMIPL_GetIPL(inImage2, &iIpl2);
    if (retVal)
      break;

    if(((rect->x + rect->dx) > (int16)iIpl->dx) || ((rect->y + rect->dy) > (int16)iIpl->dy))
    {
      retVal = EUNSUPPORTED;
      break;
    }

    if (ipl_compose(iIpl, iIpl2, oIpl, IPL_OVERLAP, rectIpl))
      retVal = EFAILED;

  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(iIpl2);
  OEMIPL_FreeIPLTypeIF(oIpl);

  FREE(rectIpl);
  rectIpl = NULL;

  return retVal;
}
/*lint -restore*/

/*=============================================================================


OEMIPL_ConvertFrame()

Description:
  This function will do frame color conversion. It ensures that the 
  transparent pixel value is unique after color conversion.
  
  Input frame and output frame have the same color format and same dimensions

Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    
See Also:
    None

=============================================================================*/
static int OEMIPL_ConvertFrame(OEMIPL *pIIpl, IBitmap *inFrame, IBitmap *outFrame)
{
  /*lint -save -e715 -e818*/
  return EUNSUPPORTED;
/*int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;

  if (!pIIpl || !inFrame|| !outFrame)
    return EBADPARM;

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE);
    if (retVal)
     break;

    if (ipl_conv_frame(iIpl, oIpl, oDIB->ncTransparent))
      retVal = EFAILED;

  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;*/
}
/*lint -restore*/

/*=============================================================================

OEMIPL_ConvertColor()

Description:
  This function will perform color conversion of the image. The user can specify the 
  desired color format of the output image and this function will return an output image
  with that color format to the user.
  

Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    
See Also:
    

=============================================================================*/
static int OEMIPL_ConvertColor(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage)
{
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  int retVal = AEE_SUCCESS;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage)
    return EBADPARM;

  do
  {
    //Convert input image to ipl struct
    if (OEMIPL_GetIPL(inImage, &iIpl) != AEE_SUCCESS)
    {
      if (OEMIPL_GetYCbCrIPL(inImage, &iIpl) != AEE_SUCCESS)
      {
        retVal = EUNSUPPORTED;
        break;
      }
    }

    //Convert output image to ipl struct
    if (OEMIPL_GetIPL(outImage, &oIpl) != AEE_SUCCESS)
    {
      if (OEMIPL_GetYCbCrIPL(outImage, &oIpl) != AEE_SUCCESS)
      {
        retVal = EUNSUPPORTED;
        break;
      }
    }

    //Do color conversion.
    if (ipl_convert_image(iIpl, oIpl) != IPL_SUCCESS)
    {
      retVal = EFAILED;
    }
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;
}
/*lint -restore*/


/*=============================================================================


OEMIPL_ViewPerspective()

Description:
  This function creates a perspective image shaped like a trapezoid.  
  The user specifies 2 input parameters: (1) the line defining the top 
  of the trapezoid, (2) a pixel value to fill in blank output regions 
  outside of the trapezoid (3) orientation of the trapezoid

============================================================================
  !! Note !! :   The line is strictly horizontal or strictly vertical (i.e. 
  either the X co-ordinate or the Y co-ordinate of the lines are the same)
============================================================================

  The width of the line must be at least 1 pixel and no more than the width 
  of the input image. A long top width corresponds to a vanishing 
  point that is far away from the viewer or camera. A short top width 
  indicates a vanishing point near the camera. The filler pixel value 
  must be given in a format compatible with the output color format. For 
  instance, black is 0 for RGB565 and 0x8010 for YCbCr. White is 0xFFFF 
  for RGB565 and 0x80EB for YCbCr. Notice that for YCbCr, the chroma 
  value (Cb or Cr) comes before the luma value, so white is 0x80EB 
  instead of 0xEB80. The trapezoidís height and bottom width are assumed 
  to be the input height and width for "up" and "down" orientations. 
  For "left" and "right" orientations, the trapezoid's height and bottom 
  width are the input width and height, respectively.

  For any orientation, the top width is defined as the smaller 
  of the two parallel edges of the trapezoid.

Return Value:
    Returns AEEErrorCodes

    AEE_SUCCESS, if the function completed successfully
    EBADPARM, if the line passed as input isn't strictly horizontal or strictly vertical
    
See Also:

=============================================================================*/
static int OEMIPL_ViewPerspective(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                                  IIPLLine *lineIn, uint16 fillerPixel,
                                  IIPLPerspectiveType orientation)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  uint32 topWidth = 0;
  int32 xStart = -1;
  
  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage || !lineIn)
    return EBADPARM; 

  if(orientation > (IIPL_MAX_PERSPECTIVE_VAL-1))
    return EBADPARM;

  if((lineIn->sX != lineIn->eX) && (lineIn->sY != lineIn->eY))
    return EBADPARM;

  if (lineIn->sY == lineIn->eY)
  {
    if ((orientation != IIPL_PERSPECTIVE_UP) && (orientation != IIPL_PERSPECTIVE_DOWN))
    {
      if(lineIn->sX != lineIn->eX)
        return EBADPARM;
    }
    else
    {
      topWidth = (uint16)(lineIn->eX > lineIn->sX ? (lineIn->eX-lineIn->sX) :
                              (lineIn->sX-lineIn->eX));
      xStart = (lineIn->eX > lineIn->sX ? lineIn->sX : lineIn->eX);
    }
  }
  
  if (lineIn->sX == lineIn->eX)
  {
    if ((orientation != IIPL_PERSPECTIVE_LEFT) && (orientation != IIPL_PERSPECTIVE_RIGHT))
    {
      if(lineIn->sY != lineIn->eY)
        return EBADPARM;
    }
    else
    {
      topWidth = (uint16)(lineIn->eY > lineIn->sY ? (lineIn->eY-lineIn->sY) :
                              (lineIn->sY-lineIn->eY));
      xStart = (lineIn->eY > lineIn->sY ? lineIn->sY : lineIn->eY);
    }
  }

  retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
  if (retVal == AEE_SUCCESS)
  {
    if(ipl_perspective(iIpl, oIpl, topWidth, xStart, fillerPixel, (uint8)orientation))
      retVal = EFAILED;
  }

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;

}
/*lint -restore*/

/*=============================================================================

OEMIPL_CopyPaste()
 
Description:
   This function performs copy-and-paste. A region is copied from the input 
  image and pasted onto a region in the output image. Input and output 
  image sizes can be different. If the input "copy" region is strictly 
  larger than the output "paste" region, the input region will be downsized 
  to fit the output region. If the images are RGB565, an input "copy" region 
  can also be upsized if it is strictly smaller than the output "paste" 
  region.

Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EBADPARM, if the paste area falls outside the output image dimensions
    
See Also:

=============================================================================*/
static int OEMIPL_CopyPaste(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                            AEERect *rcCut, AEERect *rcPaste)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_rect_type cutRectIpl;
  ipl_rect_type pasteRectIpl;
  
/*lint -save -e818*/
  if (!pIIpl || !inImage || !outImage || !rcCut || !rcPaste)
    return EBADPARM;

  if ((rcCut->x < 0) || (rcCut->y < 0))
    return EBADPARM;

  if ((rcPaste->x < 0) || (rcPaste->y < 0))
    return EBADPARM;

  cutRectIpl.x = (uint16) rcCut->x;
  cutRectIpl.y = (uint16) rcCut->y;
  cutRectIpl.dx = (uint16) rcCut->dx;
  cutRectIpl.dy = (uint16) rcCut->dy;

  pasteRectIpl.x = (uint16) rcPaste->x;
  pasteRectIpl.y = (uint16) rcPaste->y;
  pasteRectIpl.dx = (uint16) rcPaste->dx;
  pasteRectIpl.dy = (uint16) rcPaste->dy;

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, FALSE, TRUE);
    if (retVal)
      break;

    if ((((uint16)(rcCut->x + rcCut->dx)) > iIpl->dx )|| (((uint16)(rcCut->y + rcCut->dy)) > iIpl->dy))
    {
      retVal = EUNSUPPORTED;
      break;
    }

    if ((((uint16)(rcPaste->x + rcPaste->dx)) > oIpl->dx )|| (((uint16)(rcPaste->y + rcPaste->dy)) > oIpl->dy))
    {
      retVal = EUNSUPPORTED;
      break;
    }

    if(ipl_copy_and_paste(iIpl, oIpl, &cutRectIpl, &pasteRectIpl))
    {
      retVal = EFAILED;
    }
  } while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;
}
/*lint -restore*/

/*=============================================================================

OEMIPL_CutPaste()
 
Description:
  This function performs cut-and-paste. A region is cut from the input 
  image and pasted onto a region in the output image. Input and output 
  image sizes can be different. If the input "cut" region is strictly 
  larger than the output "paste" region, the input region will be downsized 
  to fit the output region. If the images are RGB565, an input "cut" region 
  can also be upsized if it is strictly smaller than the output "paste" 
  region.

  This will also affect the input image and the piece that was cut out
  will be replaced by the fillerPixel passed in by the user.

Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EBADPARM, if the paste area falls outside the output image dimensions
    
See Also:

=============================================================================*/
static int OEMIPL_CutPaste(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                           AEERect *rcCut, AEERect *rcPaste, uint16 fillerPixel)
{/*
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_rect_type *cutRectIpl = NULL;
  ipl_rect_type *pasteRectIpl = NULL;
 
  if (!pIIpl || !inImage || !outImage || !rcCut || !rcPaste)
    return EBADPARM;

  if ((rcCut.x < 0) || (rcCut.y < 0))
    return EBADPARM;

  if ((rcPaste.x < 0) || (rcPaste.y < 0))
    return EBADPARM;

  cutRectIpl = (ipl_rect_type *)MALLOC(sizeof(ipl_rect_type));
  if(!cutRectIpl)
    return ENOMEMORY;
  else
  {
    cutRectIpl->x = rcCut.x;
    cutRectIpl->y = rcCut.y;
    cutRectIpl->dx = rcCut.dx;
    cutRectIpl->dy = rcCut.dy;
  }

  pasteRectIpl = (ipl_rect_type *)MALLOC(sizeof(ipl_rect_type));
  if (!pasteRectIpl)
  {
    FREEIF(cutRectIpl);
    cutRectIpl = NULL;
    return ENOMEMORY;
  }
  else
  {
    pasteRectIpl->x = rcPaste.x;
    pasteRectIpl->y = rcPaste.y;
    pasteRectIpl->dx = rcPaste.dx;
    pasteRectIpl->dy = rcPaste.dy;
  }

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, FALSE);
    if (retVal)
     break;

    if (((rcCut.x + rcCut.dx) > iIpl->dx )|| ((rcCut.y + rcCut.dy) > iIpl->dy))
    {
      retVal = EUNSUPPORTED;
      break;
    }

    if (((rcPaste.x + rcPaste.dx) > oIpl->dx )|| ((rcPaste.y + rcPaste.dy) > oIpl->dy))
    {
      retVal = EUNSUPPORTED;
      break;
    }

    if(ipl_cut_and_paste(iIpl, oIpl, cutRectIpl, pasteRectIpl, fillerPixel))
      retVal = EFAILED;
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  FREEIF(cutRectIpl);
  cutRectIpl = NULL;
  FREEIF(pasteRectIpl);
  pasteRectIpl = NULL;

  return retVal;
*/
  /*lint -save -e715 -e818*/
  return EUNSUPPORTED;
}
/*lint -restore*/

/*=============================================================================


OEMIPL_ArbitRotate()
 
Description:
  This function performs arbitrary rotation. The user must specify 
  the angle of rotation in degrees and the pixel value used to fill 
  in blank regions. Any part of the input that is rotated outside
  image boundaries is cut off. Only rotations of 0, 90, 180, and 270 
  degrees do not cut off any part of the input image. Negative angles 
  produce clockwise rotation. Positive angles produce counterclockwise 
  rotation. Angles can be any integer between positive and negative 
  infinity.
  
Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    
See Also:
    None
=============================================================================*/
static int OEMIPL_ArbitRotate(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                              int16 angle, uint16 fillColor)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage)
    return EBADPARM;

  retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
  if (retVal == AEE_SUCCESS)
  {
    if(ipl_rotate(iIpl, oIpl, angle, fillColor))
    {
      retVal = EFAILED;
    }
  }

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;
}
/*lint -restore*/

/*=============================================================================
OEMIPL_RotateFrame()
 
Description:
  This function performs 90 degree clockwise rotation of frames with special 
  processing for transparent pixels. Four types of rotation are supported: 
  0 degrees, 90 degrees, 180 degrees, and 270 degrees.
  
  Input and output frame images must be in RGB565.
  
Return Value:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EBADPARM, if the rotation value passed in is not one of the four permitted values.
    
See Also:

=============================================================================*/
static int OEMIPL_RotateFrame(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                              IIPLRotateType rotation)
{
  IDIB *iDIB = NULL;
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage)
    return EBADPARM;

  if(rotation > (IIPL_MAX_ROTATE_VAL - 1))
    return EBADPARM;

  if(IBITMAP_QueryInterface(inImage, AEECLSID_DIB, (void **)&iDIB) != AEE_SUCCESS)
    return EUNSUPPORTED;

  retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
  if (retVal == AEE_SUCCESS)
  {
    if(ipl_rotate90_frame(iIpl, oIpl, (ipl_rotate90_type)rotation, (uint16)iDIB->ncTransparent))
    {
      retVal = EFAILED;
    }
  }

  RELEASEIF(iDIB);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;
}
/*lint -restore*/

/*=============================================================================

OEMIPL_ComposeCanvas()
 
Description:
  This function iterates through a list of icons and composes a frame
  from the list. 

  Note:
  1) iconList can be NULL if there are no icons to be added. listSize is ignored
     if iconList is NULL.
  
Return Value:
    Returns AEEError values

    AEE_SUCCESS: if the operation is successful
    
See Also:

=============================================================================*/
static int OEMIPL_ComposeCanvas(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                                IIPLIconInfo *iconList, uint32 listSize)
{
  /*lint -save -e715 -e818*/
  return EUNSUPPORTED;
}
/* lint -restore */
/*=============================================================================
OEMIPL_RotateCropAddFrame()
 
Description:
  This function performs real-time clockwise rotation and addition of a frame. 
  Four types of rotations are supported: 0 degrees, 90 degrees, 180 degrees,
  and 270 degrees. It will also

  Note:

  1) IconList is an optional parameter. It can be NULL if no icons need to be added. Also,
     listSize parameter will be ignored if IconList is NULL.

  2) The cropping rectangle should not lie outside the output image. 

  3) The input Frame can be NULL if there are no frames to be added.
    

Return Value:
   Returns AEEError Codes

   AEE_SUCCESS: operation was successful
   EBADPARM: if the cropping rectangle lies outside the output image dimensions
    
See Also:

=============================================================================*/
static int OEMIPL_RotateCropAddFrame(OEMIPL *pIIpl, IBitmap *inImage,
                                     IBitmap *inFrame, IBitmap *outImage,
                                     IIPLIconInfo *iconList, uint32 listSize,
                                     AEERect *crop, IIPLRotateType rotation)
{
  IDIB *frmDIB = NULL;
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_image_type *frmIpl = NULL;
  ipl_rect_type *rcCropIpl = NULL;
  
  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage)
    return EBADPARM;

  if(!inFrame && !crop)
    return OEMIPL_Transform(pIIpl, inImage, outImage, IIPL_ROTATE, (uint32)rotation);

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
    if (retVal)
      break;

    if(inFrame)
    {
      if(IBITMAP_QueryInterface(inFrame, AEECLSID_DIB, 
                                (void **)&frmDIB) != AEE_SUCCESS)
      {
        retVal = EUNSUPPORTED;
        break;
      }
 
      retVal = OEMIPL_DIBToIPL(frmDIB, &frmIpl);
      if(retVal)
        break;
    }

    if (crop)
    {
      rcCropIpl = (ipl_rect_type *)MALLOC(sizeof(ipl_rect_type));

      if(!rcCropIpl)
      {
        retVal = ENOMEMORY;
        break;
      }
      else
      {
        rcCropIpl->x = (uint16)crop->x;
        rcCropIpl->y = (uint16)crop->y;
        rcCropIpl->dx = (uint16)crop->dx;
        rcCropIpl->dy = (uint16)crop->dy;
      }
    }

    if (frmIpl)
    {
      if (!crop && (rotation == IIPL_NO_ROTATE))
      {
        if (ipl_image_add(iIpl, frmIpl, (uint16)frmDIB->ncTransparent, oIpl))
        {
          retVal = EFAILED;
          break;
        }
      }
      else if (crop)
      {
        if (ipl_rot_add_crop(iIpl, frmIpl, oIpl, rcCropIpl, 
                             (ipl_rotate90_type)rotation, NULL, 
                             (uint16)frmDIB->ncTransparent))
        {
          retVal = EFAILED;
          break;
        }
      }
      else 
      {
        retVal = EUNSUPPORTED;
        break;
      }
    }
    else if (crop)
    {
      if (ipl_rot_add_crop(iIpl, NULL, oIpl, rcCropIpl, 
                           (ipl_rotate90_type)rotation, NULL, 0))
      {
        retVal = EFAILED;
        break;
      }
    }
    break;
  }while(0);

  //cleanup
  RELEASEIF(frmDIB);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);
  OEMIPL_FreeIPLTypeIF(frmIpl);

  FREEIF(rcCropIpl)
  rcCropIpl = NULL;

  return retVal;
}
/*lint -restore*/


/*=============================================================================
OEMIPL_MotionBlur()
 
Description:
  This function produces motion blur in any arbitrary direction. The size 
  of the blurring window must be an odd, positive integer in the range 
  3 <= size <= 1/2 * min(w, h), where w and h are the width and height of 
  the input image. The direction of motion is specified by a positive integer 
  angle measured in degrees from the horizontal.

Return Value:
   Returns AEEError Codes

   AEE_SUCCESS: operation was successful
   EBADPARM: if the blurWindow is not a positive odd integer.
    
See Also:

=============================================================================*/
static int OEMIPL_MotionBlur(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                             uint32 blurWindow, uint32 angle)
{
  AEEBitmapInfo info;
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  uint32 maxDimension = 0;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage )
    return EBADPARM;

  if (IBITMAP_GetInfo(inImage, &info, sizeof(AEEBitmapInfo)) != SUCCESS)
    return EUNSUPPORTED;

  if(info.cx > info.cy)
  {
    if((info.cx/2)%2)
      maxDimension = info.cx/2;
    else
      maxDimension = info.cx/2 + 1;
  }
  else
  {
    if((info.cy/2)%2)
      maxDimension = info.cy/2;
    else
      maxDimension = info.cy/2 + 1;
  }

  if((blurWindow%2) && (blurWindow < maxDimension))
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
    if (retVal == AEE_SUCCESS)
    {
      if(ipl_motion_blur(iIpl, oIpl, blurWindow, (uint16)(angle%360)))
      {
        retVal = EFAILED;
      }
    }
  }
  else
    retVal = EBADPARM;

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;
}
/*lint -restore*/

/*=============================================================================
OEMIPL_RadialBlur()
 
Description:
    
  This function performs radial blur. The blurring level can be set 
  to an integer between 0 and 100, inclusive. No blurring occurs 
  when the blurring level is 0.

Return Value:
   Returns AEEError Codes

   AEE_SUCCESS: operation was successful
   EBADPARM: if the blurring level is outside the permitted range
    
See Also:

=============================================================================*/
static int OEMIPL_RadialBlur(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                             uint32 level)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage )
    return EBADPARM;

  if (level > 100)
    return EBADPARM;

  retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
  if (retVal == AEE_SUCCESS)
  {
    if(ipl_radial_blur(iIpl, oIpl, 0, 0, (uint8)level))
    {
      retVal = EFAILED;
    }
  }

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;
}
/*lint -restore*/

/*=============================================================================
OEMIPL_Warp()
 
Description:
    
  This function performs a warp on the input image. The warping effect is decided 
  by the grid that is passed in. "gridLoc" decides whether the supplied grid is applicable
  to the input image or the output image.

  The effect is achieved as follows: 

  1)  If gridLoc is set to FALSE, 
      The input image is assumed to have been set on a perfect checkerboard grid with rows and 
      columns of equal width and the output image is assumed to be set on the grid that is passed
      in, which could have rows and columns of different widths.

  2)  If gridLoc is set to TRUE,
      The input image is assumed to have been set on the grid passed in, with unequal rows and columns 
      and the output image is assumed to be set on the perfect checkerboard grid with rows and columns 
      of equal widths.
  
  Each block in the input grid is mapped to the corresponding grid in the output grid and the warp
  effect is achieved.

  So, with gridLoc set to TRUE, the grid would have the opposite effect than with gridLoc set to FALSE.

Return Value:
   Returns AEEError Codes

   AEE_SUCCESS: operation was successful
    
See Also:
    

=============================================================================*/
static int OEMIPL_Warp(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                       IIPLGrid *grid, boolean gridLoc)
{
  /*lint -save -e715 -e818*/
  return EUNSUPPORTED;
}
/*lint -restore*/

/*=============================================================================
OEMIPL_Transform()

Description:

  This function performs the transformation of the input image. The type of transformation
  to be applied is determined by the transformation type passed in to the function.

Return Value:
   Returns AEEError Codes

   AEE_SUCCESS: operation was successful
   EBADPARM: if the xFormType is not one of the permitted transformations or if
             the xFormValue is not one of the permitted values (dependent on xFormatype)
    
See Also:

=============================================================================*/
static int OEMIPL_Transform(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage, 
                            IIPLTransformType xFormType, uint32 xFormValue)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage )
    return EBADPARM;

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, FALSE, TRUE);
    if (retVal)
      break;

    switch(xFormType)
    {
    case IIPL_ROTATE:
      switch(xFormValue)
      {
      case IIPL_NO_ROTATE:
        if(ipl_rotate90(iIpl, oIpl, IPL_NOROT))
          retVal = EFAILED;
        break;
      case IIPL_ROTATE_90:
        if((iIpl->dx != oIpl->dy) || (iIpl->dy != oIpl->dx))
        {
          retVal = EBADPARM;
          break;
        }
        if(ipl_rotate90(iIpl, oIpl, IPL_ROT90))
          retVal = EFAILED;
        break;
      case IIPL_ROTATE_180:
        if(ipl_rotate90(iIpl, oIpl, IPL_ROT180))
          retVal = EFAILED;
        break;
      case IIPL_ROTATE_270:
        if((iIpl->dx != oIpl->dy) || (iIpl->dy != oIpl->dx))
        {
          retVal = EBADPARM;
          break;
        }
        if(ipl_rotate90(iIpl, oIpl, IPL_ROT270))
          retVal = EFAILED;
        break;
      default:
        retVal = EBADPARM;
        break;
      }
      break;
    case IIPL_REFLECT:
      if((iIpl->dx != oIpl->dx) || (iIpl->dy != oIpl->dy))
      {
        retVal = EUNSUPPORTED;
        break;
      }
      switch(xFormValue)
      {
      case IIPL_NO_REFLECT:
        if(ipl_reflect(iIpl, oIpl, IPL_NOREF))
          retVal = EFAILED;
        break;
      case IIPL_REFLECT_X:
        if(ipl_reflect(iIpl, oIpl, IPL_XREF))
          retVal = EFAILED;
        break;
      case IIPL_REFLECT_Y:
        if(ipl_reflect(iIpl, oIpl, IPL_YREF))
          retVal = EFAILED;
        break;
      default:
        retVal = EBADPARM;
        break;
      }
      break;
    default:
      retVal = EBADPARM;
    }
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;
}
/*lint -restore*/

/*=============================================================================

OEMIPL_Effect()

Description:

  This function adds special effects to the input image. The type of effect to be 
  applied is determined by the effect type passed in to the function.

  The different effect types are indicated by OEMIPLEffectType

Return Value:
   Returns AEEError Codes

   AEE_SUCCESS: operation was successful
   EBADPARM: if the effectType is not one of the permitted transformations or if
             the effectValue is not one of the permitted values (dependent on effectType)
    

See Also:

=============================================================================*/
static int OEMIPL_Effect(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                         IIPLEffectType effectType, int32 effectValue)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage )
    return EBADPARM;

  if(effectType > (IIPL_MAX_EFFECT_VAL - 1))
    return EBADPARM;

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
    if (retVal)
      break;

    switch(effectType)
    {
    case IIPL_GRAYSCALE :
      retVal = OEMIPL_Grayscale(iIpl, oIpl);
      break;
    case IIPL_HUE :
      retVal = OEMIPL_Hue(iIpl, oIpl, effectValue);
      break;
    case IIPL_NEGATIVE:
      retVal = OEMIPL_Negative(iIpl, oIpl);
      break;
    case IIPL_SATURATE:
      retVal = OEMIPL_Saturation(iIpl, oIpl, effectValue);
      break;
    case IIPL_SEPIA :
      retVal = OEMIPL_Sepia(iIpl, oIpl);
      break;
    case IIPL_POSTERIZE:
      retVal = OEMIPL_Posterize(iIpl, oIpl, effectValue);
       break;
    case IIPL_SOLARIZE:
      if ((effectValue < MIN_SOL_FACTOR) || (effectValue > MAX_SOL_FACTOR))
        retVal = EBADPARM;
      else if(ipl_image_solarize(iIpl, oIpl, (uint8)effectValue))
        retVal = EFAILED;
      break;
    case IIPL_FLASH :
      retVal = OEMIPL_Flash(iIpl, oIpl);
      break;
    case IIPL_INTENSITY:
      retVal = OEMIPL_Intensity(iIpl, oIpl, effectValue);
      break;
    case IIPL_FOG:
      if ((effectValue < 0) || (effectValue > 1))
        retVal = EBADPARM;
      else if(ipl_fog(iIpl, oIpl, (uint8)effectValue))
        retVal = EFAILED;
      break;
    case IIPL_NEON:
      if(ipl_neon(iIpl, oIpl))
        retVal = EFAILED;
      break;
    case IIPL_WHITEBOARD:
      retVal = OEMIPL_Whiteboard(iIpl, oIpl, effectValue);
      break;
    default:
      retVal = EBADPARM;
    }
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;
}
/*lint -restore*/

/*=============================================================================
OEMIPL_Compose()

Description:

  This function composes an output image by combining two input images. There are
  several composition functions that can be used to combine two images. 

  The different composition methods are indicated by OEMIPLComposeType

Return Value:
   Returns AEEError Codes

   AEE_SUCCESS: operation was successful
   EBADPARM: if the composeType is not one of the permitted transformations or if
             the composeType is not one of the permitted values (dependent on composeType)
    
See Also:
=============================================================================*/
static int OEMIPL_Compose(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *inImage2,
                          IBitmap *outImage, IIPLComposeType composeType,
                          int32 composeValue)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_image_type *iIpl2 = NULL;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !inImage2 || !outImage)
    return EBADPARM;

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
    if (retVal)
      break;

    retVal = OEMIPL_GetIPL(inImage2, &iIpl2);
    if (retVal)
      break;

    switch(composeType)
    {
    case IIPL_BLEND:
      if((composeValue < 0) || (composeValue > 100))
        retVal = EBADPARM;
      else
      {
        if(ipl_compose(iIpl, iIpl2, oIpl, IPL_BLEND, (void *)&composeValue))
          retVal = EFAILED;
      }
      break;
    case IIPL_CENTER:
      if((composeValue < 0) || (composeValue > (int32)oIpl->dx))
        retVal = EBADPARM;
      else
      {
        if(ipl_compose(iIpl, iIpl2, oIpl,IPL_CENTER, (void *)&composeValue))
          retVal = EFAILED;
      }
      break;
    case IIPL_FILMSTRIP:
      if((composeValue < 0) || (composeValue > (int32)oIpl->dx))
        retVal = EBADPARM;
      else
      {
        if(ipl_compose(iIpl, iIpl2, oIpl, IPL_FILMSTRIP, (void *)&composeValue))
          retVal = EFAILED;
      }
      break;
    case IIPL_HORIZONTAL:
      if((composeValue < 0) || (composeValue > (int32)oIpl->dx))
        retVal = EBADPARM;
      else
      {
        if(ipl_compose(iIpl, iIpl2, oIpl, IPL_HORIZONTAL, (void *)&composeValue))
          retVal = EFAILED;
      }
      break;
    case IIPL_VERTICAL:
      if((composeValue < 0) || (composeValue > (int32)oIpl->dy))
        retVal = EBADPARM;
      else
      {
        if(ipl_compose(iIpl, iIpl2, oIpl, IPL_VERTICAL, (void *)&composeValue))
          retVal = EFAILED;
      }
      break;
    default:
      retVal = EUNSUPPORTED;
      break;
    }
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(iIpl2);
  OEMIPL_FreeIPLTypeIF(oIpl);

  return retVal;
}
/*lint -restore*/

/*=============================================================================
OEMIPL_DrawHistogram()

Description:

  This function composes a 2D histogram chart over an image.

Return Value:
   Returns AEEError Codes

   AEE_SUCCESS: operation was successful
   EBADPARM: if input params are invalid
    
See Also:
=============================================================================*/
static int OEMIPL_DrawHistogram(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                                AEERect *pRect)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_histogram_type hist;
  ipl_rect_type *pRectIpl = NULL;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage)
    return EBADPARM;

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
    if (retVal)
      break;

    hist.size = 256;
    hist.channel = IPL_CHANNEL_LUMA;
    if(ipl_util_CalcHistogram(iIpl, NULL, &hist))
    {
      retVal = EFAILED;
      break;
    }
    if (pRect)      
    {
      pRectIpl = (ipl_rect_type *)MALLOC(sizeof(ipl_rect_type));
      if(!pRectIpl)
        return ENOMEMORY;
      else
      {
        pRectIpl->dx = (uint16)pRect->dx;
        pRectIpl->dy = (uint16)pRect->dy;
        pRectIpl->x  = (uint16)pRect->x;
        pRectIpl->y  = (uint16)pRect->y;
      }
    }

    //pRectIpl could be NULL pointer. In this case, ipl function will assign
    //a location and size for the histogram.
    if(ipl_comp_DrawHistogram(iIpl, oIpl, &hist, pRectIpl, 255, 128, 0))
    {
      retVal = EFAILED;
      break;
    }
    
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  FREEIF(pRectIpl);
  pRectIpl = NULL;

  return retVal;
}
/*lint -restore*/

/*=============================================================================
OEMIPL_DrawRgbHistogram()

Description:

  This function composes 2D RGB histogram charts over an image. From left to
  right: First chart is R, Second G and last is B 
  
Return Value:
   Returns AEEError Codes

   AEE_SUCCESS: operation was successful
   EBADPARM: if input params are invalid
    
See Also:
=============================================================================*/
static int OEMIPL_DrawRgbHistogram(OEMIPL *pIIpl, IBitmap *inImage,
                                   IBitmap *outImage, AEERect *pRect)
{
  int retVal = AEE_SUCCESS, i;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_histogram_type hist;
  ipl_rect_type *pRectIpl = NULL;

  /*lint -save -e715 -e818*/
  if (!pIIpl || !inImage || !outImage)
    return EBADPARM;

  do
  {
    retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, TRUE, TRUE);
    if (retVal)
      break;

    hist.size = 256;
    hist.channel = IPL_CHANNEL_RED;
    /* Compute R - Histogram */
    if(ipl_util_CalcHistogram(iIpl, NULL, &hist))
    {
      retVal = EFAILED;
      break;
    }
    if (pRect)      
    {
      pRectIpl = (ipl_rect_type *)MALLOC(sizeof(ipl_rect_type));
      if(!pRectIpl)
        return ENOMEMORY;
      else
      {
        pRectIpl->dx = (uint16)pRect->dx;
        pRectIpl->dy = (uint16)pRect->dy;
        pRectIpl->x  = (uint16)pRect->x;
        pRectIpl->y  = (uint16)pRect->y;
      }
    }

    if(ipl_comp_DrawHistogram(iIpl, oIpl, &hist, pRectIpl, 128, 128, 128))
    {
      retVal = EFAILED;
      break;
    }
    
    /* ****************************************************************
      Compute G - Histogram for i = 0 and Plot it on top of R-Histo
      Compute B - Histogram for i = 1 and Plot it on top of G-Histo
    **************************************************************** */
    hist.channel = IPL_CHANNEL_GREEN;
    for(i = 0; i < 2; i++)
    {
      
      /* ****************************************************************
        Important to Update pRectIpl->x since need to draw G Histo from
        after pRectIpl->dx + 5 pixels from Where previous histo is drawn
      **************************************************************** */
      pRectIpl->x  = (pRectIpl->x + pRectIpl->dx + 5); 
    
      if(ipl_util_CalcHistogram(iIpl, NULL, &hist))
      {
        retVal = EFAILED;
        break;
      }
      
      if(ipl_comp_DrawHistogram(oIpl, NULL, &hist, pRectIpl, 128, 128, 128))
      {
        retVal = EFAILED;
        break;
      }
      hist.channel = IPL_CHANNEL_BLUE;
    }
  }while(0);

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  FREEIF(pRectIpl);
  pRectIpl = NULL;

  return retVal;
}
/*lint -restore*/


/*=====================================================================
FUNCTION OEMIPL_RemoveRedEye

  DESCRIPTION:
    This function performs red eye reduction. A region(rcCut) from the input image 
    is searched for the red eye. Once the red eye is removed, the modified region(rcPaste)
    is pasted to the output image. Input and output image sizes can be different.
    The cut/paste region is relative location between the input/output image 
    and the full image.

  PARAMETERS:
    pIIpl[in]:  Pointer to IIpl Interface.
    inImage[in]: Pointer to the input image.
    outImage[in/out]: Pointer to the output image
    rcCut[in]: The rectangular area with the red eye to be modified from the input image
    rcPaste[in]: The rectangular area on the output image where the modified image is to be pasted


  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    ENOMEMORY, if no memory to perform the feature.
======================================================================*/
static int  OEMIPL_RemoveRedEye(OEMIPL *pIIpl, IBitmap *inImage, IBitmap *outImage,
                                AEERect rcCut, AEERect rcPaste)
{
  /*lint -save -e715 -e818*/
#if defined (FEATURE_IPL_EFX_COMPACT)
  return EUNSUPPORTED;
#else
  int retVal = AEE_SUCCESS;
  ipl_image_type *iIpl = NULL;
  ipl_image_type *oIpl = NULL;
  ipl_rect_type *pRectIn = NULL;
  ipl_rect_type *pRectOut = NULL;

  if (!pIIpl || !inImage || !outImage)
    return EBADPARM;

  pRectIn = (ipl_rect_type *)MALLOC(sizeof(ipl_rect_type));
  if(!pRectIn)
    return ENOMEMORY;
  else
  {
    pRectIn->x = (uint16)rcCut.x;
    pRectIn->y = (uint16)rcCut.y;
    pRectIn->dx = (uint16)rcCut.dx;
    pRectIn->dy = (uint16)rcCut.dy;
  }

  pRectOut = (ipl_rect_type *)MALLOC(sizeof(ipl_rect_type));
  if(!pRectOut)
  {
    FREE(pRectIn);
    pRectIn = NULL;
    return ENOMEMORY;
  }
  else
  {
    pRectOut->x = (uint16)rcPaste.x;
    pRectOut->y = (uint16)rcPaste.y;
    pRectOut->dx = (uint16)rcPaste.dx;
    pRectOut->dy = (uint16)rcPaste.dy;
  }

  retVal = OEMIPL_GetIPLs(inImage, outImage, &iIpl, &oIpl, FALSE, TRUE);
  if (retVal == AEE_SUCCESS)
  {
    if (ipl_red_eye(iIpl, oIpl, pRectIn, pRectOut, NULL, NULL))
    {
      retVal = EFAILED;
    }
  }

  OEMIPL_FreeIPLTypeIF(iIpl);
  OEMIPL_FreeIPLTypeIF(oIpl);

  FREE(pRectIn);
  pRectIn = NULL;
  FREE(pRectOut);
  pRectOut = NULL;

  return retVal;
#endif  // IPL_EFX_COMPACT
}
/*lint -restore*/

/*=====================================================================
FUNCTION OEMIPL_Grayscale

  DESCRIPTION:
    This function will convert the color of input image to gray scale

  PARAMETERS:
    iIpl: Pointer to input Image
    oIpl: Pointer to output image
   
  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EFAILED, if failure in performing the feature.
======================================================================*/
static int OEMIPL_Grayscale(const ipl_image_type *iIpl, ipl_image_type *oIpl)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *tempIpl = NULL;

  if (!iIpl || !oIpl)
    return EBADPARM;

  retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
  if (retVal == AEE_SUCCESS)
  {
    if(ipl_image_grayscale(tempIpl, oIpl))
    {
      retVal = EFAILED;
    }
  }

  OEMIPL_FreeIPLDataIF(tempIpl);

  return retVal;
}

/*=====================================================================
FUNCTION OEMIPL_Hue

  DESCRIPTION:
    This function is used to do hue scaling.

  PARAMETERS:
    iIpl: Pointer to input Image
    oIpl: Pointer to output image
    hue factor: Hue scale factor with range -360 to 360
   
  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EUNSUPPORTED,.if the API is not supported.
    EFAILED, if failure in performing the feature.
======================================================================*/
static int OEMIPL_Hue(const ipl_image_type *iIpl, const ipl_image_type *oIpl, int32 hue_factor)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *tempIpl = NULL;
  ipl_image_type *tempIpl2 = NULL;

  if (!iIpl || !oIpl)
    return EBADPARM;

  if ((hue_factor < MIN_HUE_FACTOR) || (hue_factor > MAX_HUE_FACTOR))
    return EUNSUPPORTED;

  do
  {
    retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
    if (retVal)
      break;

    retVal = OEMIPL_CreateIPL(&tempIpl2, (uint16)tempIpl->dx, (uint16)tempIpl->dy,
                              IPL_YCbCr);
    if (retVal)
      break;

    if(ipl_hue(tempIpl,tempIpl2,hue_factor))
    {
      retVal = EFAILED;
      break;
    }

    if(ipl_YCbCr2RGB(tempIpl2->imgPtr, (void *)oIpl->imgPtr, (short)tempIpl2->dx,
                     (short)tempIpl2->dy))
    {
      retVal = EFAILED;
      break;
    }
  }while(0);

  OEMIPL_FreeIPLDataIF(tempIpl);
  OEMIPL_FreeIPLDataIF(tempIpl2);

  return retVal;
}

/*=====================================================================
FUNCTION OEMIPL_Negative

  DESCRIPTION:
    This function will negate the input image.

  PARAMETERS:
    iIpl: Pointer to input Image
    oIpl: Pointer to output image

  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EFAILED, if failure in performing the feature.
======================================================================*/
static int OEMIPL_Negative(const ipl_image_type *iIpl, ipl_image_type *oIpl)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *tempIpl = NULL;

  if (!iIpl || !oIpl)
    return EBADPARM;

  retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
  if (retVal == AEE_SUCCESS)
  {
    if(ipl_image_negative(tempIpl, oIpl))
    {
      retVal = EFAILED;
    }
  }

  OEMIPL_FreeIPLDataIF(tempIpl);

  return retVal;
}

/*=====================================================================
FUNCTION OEMIPL_Saturation

  DESCRIPTION:
    This function is used to do saturation scaling.

  PARAMETERS:
    iIpl: Pointer to input Image
    oIpl: Pointer to output image
    sat_factor: Saturation scale factor with range -255 to 255

  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EUNSUPPORTED,.if the API is not supported.
    EFAILED, if failure in performing the feature.
======================================================================*/
static int OEMIPL_Saturation(const ipl_image_type *iIpl, const ipl_image_type *oIpl,
                             int32 sat_factor)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *tempIpl = NULL;
  ipl_image_type *tempIpl2 = NULL;

  if (!iIpl || !oIpl)
    return EBADPARM;

  if ((sat_factor < MIN_SAT_FACTOR) || (sat_factor > MAX_SAT_FACTOR))
    return EUNSUPPORTED;

  do
  {
    retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
    if (retVal)
      break;

    retVal = OEMIPL_CreateIPL(&tempIpl2, (uint16)tempIpl->dx, (uint16)tempIpl->dy,
                              IPL_YCbCr);
    if (retVal)
      break;

    if(ipl_sat(tempIpl,tempIpl2,sat_factor))
    {
      retVal = EFAILED;
      break;
    }

    if(ipl_YCbCr2RGB(tempIpl2->imgPtr, (void *)oIpl->imgPtr, (short)tempIpl2->dx,
      (short)tempIpl2->dy))
    {
      retVal = EFAILED;
      break;
    }
  }while(0);

  OEMIPL_FreeIPLDataIF(tempIpl);
  OEMIPL_FreeIPLDataIF(tempIpl2);

  return retVal;

}

/*=====================================================================
FUNCTION OEMIPL_Sepia

  DESCRIPTION:
    This function will convert the input image to sepia.

  PARAMETERS:
    iIpl: Pointer to input Image
    oIpl: Pointer to output image

  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EFAILED, if failure in performing the feature.
======================================================================*/
static int OEMIPL_Sepia(const ipl_image_type *iIpl, ipl_image_type *oIpl)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *tempIpl = NULL;

  if (!iIpl || !oIpl)
    return EBADPARM;

  retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
  if (retVal == AEE_SUCCESS)
  {
    if(ipl_image_sepia(tempIpl, oIpl))
    {
      retVal = EFAILED;
    }
  }

  OEMIPL_FreeIPLDataIF(tempIpl);

  return retVal;
}

/*=====================================================================
FUNCTION OEMIPL_Posterize

  DESCRIPTION:
    This function will posterize the input image.

  PARAMETERS:
    iIpl: Pointer to input Image
    oIpl: Pointer to output image
    pos_factor: Number of steps to quantize the input. [1..255]

  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EUNSUPPORTED,.if the API is not supported.
    EFAILED, if failure in performing the feature.
======================================================================*/
static int OEMIPL_Posterize(const ipl_image_type *iIpl, ipl_image_type *oIpl,
                            int32 pos_factor)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *tempIpl = NULL;

  if (!iIpl || !oIpl)
    return EBADPARM;

  if ((pos_factor < MIN_POS_FACTOR) || (pos_factor > MAX_POS_FACTOR))
    return EUNSUPPORTED;

  retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
  if (retVal == AEE_SUCCESS)
  {
    if(ipl_image_posterize(tempIpl, oIpl, (uint8)pos_factor))
    {
      retVal = EFAILED;
    }
  }

  OEMIPL_FreeIPLDataIF(tempIpl);

  return retVal;
}

/*=====================================================================
FUNCTION OEMIPL_Intensity

  DESCRIPTION:
    This function will do intensity scaling.

  PARAMETERS:
    iIpl: Pointer to input Image
    oIpl: Pointer to output image
    int_factor: Intensity scale factor with range -255 to 255

  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EUNSUPPORTED,.if the API is not supported.
    EFAILED, if failure in performing the feature.
======================================================================*/
static int OEMIPL_Intensity(const ipl_image_type *iIpl, const ipl_image_type *oIpl,
                            int32 int_factor)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *tempIpl = NULL;
  ipl_image_type *tempIpl2 = NULL;

  if (!iIpl || !oIpl)
    return EBADPARM;

  if ((int_factor < MIN_INT_FACTOR) || (int_factor > MAX_INT_FACTOR))
    return EUNSUPPORTED;

  do
  {
    retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
    if (retVal)
      break;

    retVal = OEMIPL_CreateIPL(&tempIpl2, (uint16)tempIpl->dx, (uint16)tempIpl->dy,
                              IPL_YCbCr);
    if (retVal)
      break;

    if(ipl_int(tempIpl, tempIpl2, int_factor))
    {
      retVal = EFAILED;
      break;
    }

    if(ipl_YCbCr2RGB(tempIpl2->imgPtr, (void *)oIpl->imgPtr, (short)tempIpl2->dx,
      (short)tempIpl2->dy))
    {
      retVal = EFAILED;
      break;
    }
  }while(0);

  OEMIPL_FreeIPLDataIF(tempIpl);
  OEMIPL_FreeIPLDataIF(tempIpl2);

  return retVal;
}

/*=====================================================================
FUNCTION OEMIPL_Flash

  DESCRIPTION:
    This function will do intensity equalization and color conversion if required.

  PARAMETERS:
    iIpl: Pointer to input Image
    oIpl: Pointer to output image

  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EFAILED, if failure in performing the feature.
======================================================================*/
static int OEMIPL_Flash(const ipl_image_type *iIpl, ipl_image_type *oIpl)
{
  int retVal = AEE_SUCCESS;
  ipl_image_type *tempIpl = NULL;

  if (!iIpl || !oIpl)
    return EBADPARM;

  retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
  if (retVal == AEE_SUCCESS)
  {
    if(ipl_smart_flash(tempIpl, oIpl))
    {
      retVal = EFAILED;
    }
  }

  OEMIPL_FreeIPLDataIF(tempIpl);

  return retVal;
}

/*=====================================================================
FUNCTION OEMIPL_Whiteboard

  DESCRIPTION:
    This function takes an image taken of a white board, or chalk board, and 
    cleans it up for easy viewing and printing.

  PARAMETERS:
    iIpl: Pointer to input Image
    oIpl: Pointer to output image
    threshold: granulatiry when removing non-white board items
               0: high, 1: regular, 2: coarse, 3: do not remove non-white board items.

  LIMITATION:
    ipl API has more options for Whiteboard effect. Here uses default values 
    to simplify the user interface.
    blackmode: 0 auto, 1 whiteboard, 2 blackboard, Default: 0
    th_noise: 0 - 255 Default: 10

  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EFAILED, if failure in performing the feature.
======================================================================*/
static int OEMIPL_Whiteboard(ipl_image_type *iIpl, ipl_image_type *oIpl,
                             int32 threshold)
{
  /*lint -save -e715 -e818*/
  int retVal = AEE_SUCCESS;
  ipl_image_type *tempIpl = NULL;
  ipl_image_type *tempIpl2 = NULL;

  if (!iIpl || !oIpl)
    return EBADPARM;

  do
  {
    retVal = OEMIPL_RGB2YCbCrIPL(iIpl, &tempIpl);
    if (retVal)
      break;

    retVal = OEMIPL_CreateIPL(&tempIpl2, (uint16)tempIpl->dx, (uint16)tempIpl->dy,
                              IPL_YCbCr);
    if (retVal)
      break;

    if(ipl_efx_Whiteboard(tempIpl, tempIpl2, 0, 10, threshold, 0))
    {
      retVal = EFAILED;
      break;
    }

    if(ipl_YCbCr2RGB(tempIpl2->imgPtr, (void *)oIpl->imgPtr, (short)tempIpl2->dx,
      (short)tempIpl2->dy))
    {
      retVal = EFAILED;
      break;
    }
  }while(0);

  OEMIPL_FreeIPLDataIF(tempIpl);
  OEMIPL_FreeIPLDataIF(tempIpl2);

  return retVal;
}
 /*lint -restore*/

/*=====================================================================
FUNCTION OEMIPL_DIBToIPL

  DESCRIPTION:
    This function converts input DIB to output IPL image format.

  PARAMETERS:
    pDIB: Pointer to input Image
    inIpl: Pointer to output image

  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    ENOMEMORY, if no memory to perform the task.
======================================================================*/
static int OEMIPL_DIBToIPL(const IDIB *pDIB, ipl_image_type **inIpl)
{
  ipl_image_type *tmpIpl = NULL;

  if(!pDIB || !inIpl)
    return EBADPARM;

  *inIpl = NULL;

  //Simply check if pDIB is valid
  if (((pDIB->nColorScheme == IDIB_COLORSCHEME_565) && (pDIB->nDepth != 16)) || 
      ((pDIB->nColorScheme == IDIB_COLORSCHEME_888) && (pDIB->nDepth != 24)))
    return EBADPARM;

  tmpIpl = (ipl_image_type *)MALLOC(sizeof(ipl_image_type));
   
  if(!tmpIpl)
    return ENOMEMORY;

  tmpIpl->dy = pDIB->cy;
  tmpIpl->imgPtr = pDIB->pBmp;
  switch (pDIB->nColorScheme)
  {
  case IDIB_COLORSCHEME_565:
    tmpIpl->dx = (pDIB->nPitch > 1) ? (uint32)(int)((pDIB->nPitch * 8) / pDIB->nDepth) : 
                 (uint32)pDIB->cx;
    tmpIpl->cFormat = IPL_RGB565;
    break;
  case IDIB_COLORSCHEME_888:
    tmpIpl->dx = (pDIB->nPitch > 2) ? (uint32)(int)((pDIB->nPitch * 8) / pDIB->nDepth) : 
                 (uint32)pDIB->cx;
    tmpIpl->cFormat = IPL_RGB888;
    break;
  default:
    FREE (tmpIpl);
    tmpIpl = NULL;
    return EUNSUPPORTED;
  }
   

  *inIpl = tmpIpl;
  return AEE_SUCCESS;
}

/*=====================================================================
FUNCTION OEMIPL_YCbCrToIPL

  DESCRIPTION:
    This function converts input YCbCr to output IPL image format.

  PARAMETERS:
    pYCbCr: Pointer to input Image
    inIpl: Pointer to output image

  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    ENOMEMORY, if no memory to perform the task.
======================================================================*/
static int  OEMIPL_YCbCrToIPL(const IYCbCr *pYCbCr, ipl_image_type **inIpl)
{
  ipl_image_type *tmpIpl = NULL;

  if(!inIpl)
    return EBADPARM;

  *inIpl = NULL;

  tmpIpl = (ipl_image_type *)MALLOC(sizeof(ipl_image_type));
  
  if(!tmpIpl)
    return ENOMEMORY;

  tmpIpl->dx = pYCbCr->cx;
  tmpIpl->dy = pYCbCr->cy;
  tmpIpl->imgPtr = pYCbCr->pLuma;
  tmpIpl->clrPtr = pYCbCr->pChroma;
  switch (pYCbCr->uScheme)
  {
  case IYCBCR_COLORSCHEME_420LP:
    tmpIpl->cFormat = IPL_YCbCr420_LINE_PK;
    break;
  case IYCBCR_COLORSCHEME_422LP:
    tmpIpl->cFormat = IPL_YCbCr422_LINE_PK;
    break;
  default:
    FREE (tmpIpl);
    tmpIpl = NULL;
    return EUNSUPPORTED;
  }

  *inIpl = tmpIpl;
  return AEE_SUCCESS;
}

/*=====================================================================
FUNCTION OEMIPL_CreateIPL

  DESCRIPTION:
    This function is used to create temporary IPL.

  PARAMETERS:
    outIpl: Pointer to IPL
    width: Width of input frame.
    height: Height of  input frame
    colorFormat: Type of Color format
  
  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    ENOMEMORY, if no memory to perform the task.
======================================================================*/
static int OEMIPL_CreateIPL(ipl_image_type **outIpl, uint16 width, uint16 height,
                            ipl_col_for_type colorFormat)
{
  ipl_image_type *tmpIpl = NULL;

  if (!outIpl)
    return EBADPARM;

  *outIpl = NULL;
   
  tmpIpl = (ipl_image_type *)MALLOC(sizeof(ipl_image_type));
  if(!tmpIpl)
    return ENOMEMORY;

  switch (colorFormat)
  {
  case IPL_RGB565:
    tmpIpl->imgPtr = (unsigned char *)MALLOC(width*height*2);
    break;
  case IPL_RGB888:
    tmpIpl->imgPtr = (unsigned char *)MALLOC(width*height*3);
    break;
  case IPL_YCbCr:
    tmpIpl->imgPtr = (unsigned char *)MALLOC(width*height*2);
    break;
  case IPL_YCbCr420_LINE_PK:
    tmpIpl->imgPtr = (unsigned char *)MALLOC(width*height*3/2);
    tmpIpl->clrPtr = tmpIpl->imgPtr + (width*height);
    break;
  case IPL_YCbCr422_LINE_PK:
    tmpIpl->imgPtr = (unsigned char *)MALLOC(width*height*2);
    tmpIpl->clrPtr = tmpIpl->imgPtr + (width*height);
    break;
  default:
    FREE (tmpIpl);
    tmpIpl = NULL;
    return EBADPARM;
  }

  if(!tmpIpl->imgPtr)
  {
    FREE(tmpIpl);
    tmpIpl = NULL;
    return ENOMEMORY;
  }
  else
  {
    tmpIpl->dx = width;
    tmpIpl->dy = height;
    tmpIpl->cFormat = colorFormat;
  }

  *outIpl = tmpIpl;
  return AEE_SUCCESS;
}

/*=====================================================================
FUNCTION OEMIPL_FreeIPLTypeIF

  DESCRIPTION:
    This function is used to free IPL type

  PARAMETERS:
    inIpl: Pointer to IPL
    
  RETURN VALUE:
    No return value
======================================================================*/
static void OEMIPL_FreeIPLTypeIF(ipl_image_type *inIpl)
{
  if(inIpl)
  {
    FREE(inIpl);
    inIpl = NULL;
  }
}

/*=====================================================================
FUNCTION OEMIPL_FreeIPLDataIF

  DESCRIPTION:
    This function is used to free IPL data

  PARAMETERS:
    inIpl: Pointer to IPL
    
  RETURN VALUE:
    No return value
  ======================================================================*/
static void OEMIPL_FreeIPLDataIF(ipl_image_type *inIpl)
{
  if (inIpl)
  {
    FREEIF(inIpl->imgPtr);
    inIpl->imgPtr = NULL;
    FREE(inIpl);
    inIpl = NULL;
  }
}

/*=====================================================================
FUNCTION OEMIPL_GetIPL

  DESCRIPTION:
    This is a sharing function between different features.

  PARAMETERS:
    inImg: Pointer to input image
    iIpl: Pointer to IPL type image
    
  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
  ======================================================================*/
static int OEMIPL_GetIPL(IBitmap *inImg, ipl_image_type **iIpl)
{
  int retval = AEE_SUCCESS;
  IDIB *iDIB = NULL;

  if (!inImg || !iIpl)
    return EBADPARM;

  //Retrieve the input DIB
  if(IBITMAP_QueryInterface(inImg, AEECLSID_DIB, (void **)&iDIB) != AEE_SUCCESS)
    return EUNSUPPORTED;
   
  //Convert the Input DIB to an Ipl Image type
  retval = OEMIPL_DIBToIPL(iDIB, iIpl);

  //Cleanup
  RELEASEIF(iDIB);
  return retval;
}


/*=====================================================================
FUNCTION OEMIPL_GetIPLs

  DESCRIPTION:
    This is a sharing function between different features. 

  PARAMETERS:
    inImg: Pointer to input image
    outImg: Pointer to output image
    iIpl: Pointer to input IPL type image
    oIpl: Pointer to output IPL type image
    bCheckDIB: Boolean variable
    
  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
  ======================================================================*/
static int OEMIPL_GetIPLs(IBitmap *inImg, IBitmap *outImg, ipl_image_type **iIpl,
                          ipl_image_type **oIpl, boolean bCheckSize, boolean bDIB)
{
  int retval = AEE_SUCCESS;

  if (!inImg || !outImg || !iIpl || !oIpl)
    return EBADPARM;

  if (bDIB)
  {
    retval = OEMIPL_GetIPL(inImg, iIpl);
    if (retval)
      return retval;

    retval = OEMIPL_GetIPL(outImg, oIpl);
    if (retval)
      return retval;
  }
  else
  {
    retval = OEMIPL_GetYCbCrIPL(inImg, iIpl);
    if (retval)
      return retval;

    retval = OEMIPL_GetYCbCrIPL(outImg, oIpl);
    if (retval)
      return retval;
  }
  if(bCheckSize)
  {
    //Check to see that the input and output images are of the same size
    if(((*iIpl)->dx != (*oIpl)->dx) || ((*iIpl)->dy != (*oIpl)->dy))
    {
      retval = EUNSUPPORTED;
    }
  }
  return retval;
}

/*=====================================================================
FUNCTION OEMIPL_RGB2YCbCrIPL

  DESCRIPTION:
    This function is does color conversion from RGB 565 to YCbCr 4:2:2
    
  PARAMETERS:
    iIpl: Pointer to input data which needs to be color converted
    oIpl: Pointer to converted output 
  
  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
    EFAILED, if failure in performing the feature.
  ======================================================================*/
static int OEMIPL_RGB2YCbCrIPL(const ipl_image_type *iIpl, ipl_image_type**oIpl)
{
  int retval = AEE_SUCCESS;

  if (!iIpl || !oIpl)
    return EBADPARM;

  retval = OEMIPL_CreateIPL(oIpl, (uint16)iIpl->dx, (uint16)iIpl->dy, IPL_YCbCr);
  if (retval)
    return retval;

  if (ipl_RGB2YCbCr(iIpl->imgPtr, (*oIpl)->imgPtr, (int16)iIpl->dx, 
                    (int16)iIpl->dy))
  {
    retval = EFAILED;
  }
  return retval;
}


/*=====================================================================
FUNCTION OEMIPL_GetYCbCrIPL

  DESCRIPTION:
    This is a sharing function and Helper function to convert YCbCr to IPL

  PARAMETERS:
    inImg: Pointer to input image
    iIpl: Pointer to IPL type image
    
  RETURN VALUE:
    Returns AEEError codes

    AEE_SUCCESS, if the function completed successfully
  ======================================================================*/
static int OEMIPL_GetYCbCrIPL(IBitmap *inImg, ipl_image_type **iIpl)
{
  int retval = AEE_SUCCESS;
  IYCbCr *iYCbCr= NULL;

  if (!inImg || !iIpl)
    return EBADPARM;

  //Retrieve the input IYCbCr
  if(IBITMAP_QueryInterface(inImg, AEEIID_YCBCR, (void **)&iYCbCr) != AEE_SUCCESS)
    return EUNSUPPORTED;
   
  //Convert the Input IYCbCr to an Ipl Image type
  retval = OEMIPL_YCbCrToIPL(iYCbCr, iIpl);

  //Cleanup
  RELEASEIF(iYCbCr);
  return retval;
}

/*lint -restore*/

