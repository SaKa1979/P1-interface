/*****************************************************************************/
/*!
 *\MODULE              Application
 *
 *\COMPONENT           $HeadURL: http://svn/apps/Application_Notes/JN-AN-1085-JenNet-Tutorial/Tags/Release_1v4-Public/Step2_Common/Source/App.h $
 *
 *\VERSION			   $Revision: 5394 $
 *
 *\REVISION            $Id: App.h 5394 2010-02-15 14:15:22Z mlook $
 *
 *\DATED               $Date: 2010-02-15 14:15:22 +0000 (Mon, 15 Feb 2010) $
 *
 *\AUTHOR              $Author: mlook $
 *
 *\DESCRIPTION         Appliication - public interface.
 */
/*****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2010. All rights reserved
 *
 ****************************************************************************/

#ifndef  APP_H_INCLUDED
#define  APP_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <Jenie.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* Network parameters - these MUST be changed to suit the target application */
#define APPLICATION_ID 		    0x10850002
#define PAN_ID                  0x8522
#define CHANNEL                 0
#if CHANNEL
#define SCAN_CHANNELS          (1<<CHANNEL)
#else
#define SCAN_CHANNELS           0x07FFF800
#endif
#define ROUTING_TABLE_SIZE		100

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Public Function Prototypes                                    ***/
/****************************************************************************/

PUBLIC void vApp_CbInit(bool_t);     						  /* Warm start */
PUBLIC void vApp_CbMain(void);
PUBLIC void vApp_CbStackMgmtEvent(teEventType, void *);
PUBLIC void vApp_CbStackDataEvent(teEventType, void *);
PUBLIC void vApp_CbHwEvent(uint32, uint32);

#if defined __cplusplus
}
#endif

#endif  /* APP_H_INCLUDED */
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
