#ifndef _SPORTSMENU_H_
#define _SPORTSMENU_H_
/*********************************************************************************

模块名       : <SportApplication>
文件名       : <SportSApp.h>
相关文件     : 
文件实现功能 : Sports Application
作者         : 
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 

******************************************************************************/

/*==============================================================================
                                 本文件包含的外部文件
==============================================================================*/


/*==============================================================================
                                 宏定义和常数
==============================================================================*/


/*==============================================================================
                                 类型定义
==============================================================================*/


/*==============================================================================
                                 类接口定义
==============================================================================*/
typedef struct _ISportsMenu  ISportsMenu;
 QINTERFACE(ISportsMenu)
{
    INHERIT_IApplet(ISportsMenu);
};


/*==============================================================================
                                 类接口函数定义
==============================================================================*/
#define ISportsMenu_AddRef(p)                GET_PVTBL(p,ISportsMenu)->AddRef(p)

#define ISportsMenu_Release(p)                GET_PVTBL(p,ISportsMenu)->Release(p)

#define ISportsMenu_HandleEvent(p,ec,wp,dw) \
GET_PVTBL(p,ISportsMenu)->HandleEvent(p, ec, wp, dw)

#endif

