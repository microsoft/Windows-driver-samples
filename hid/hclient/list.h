/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    list.h

Abstract:

    This module contains the code for manipulating list structures.

Environment:

    User mode

Revision History:

    Nov-97 : Created 

--*/


#ifndef __LIST_H__
#define __LIST_H__

#include <windows.h>

typedef LIST_ENTRY      LIST_NODE_HDR, *PLIST_NODE_HDR;
typedef LIST_NODE_HDR   LIST, *PLIST;

typedef VOID PLIST_CALLBACK(PLIST_NODE_HDR);

VOID
InitializeList(
    IN  PLIST   NewList
);

VOID
InsertHead(
    IN  PLIST           List,
    IN  PLIST_NODE_HDR  ListNode
);

VOID
InsertTail(
    IN  PLIST           List,
    IN  PLIST_NODE_HDR  ListNode
);

VOID
RemoveNode(
    IN  PLIST_NODE_HDR  ListNode   
);

PLIST_NODE_HDR
RemoveHead(
    IN  PLIST   List
);

PLIST_NODE_HDR
RemoveTail(
    IN  PLIST   List
);

BOOL
IsListEmpty(
    IN  PLIST   List
);

PLIST_NODE_HDR
GetListHead(
    IN  PLIST   List
);

PLIST_NODE_HDR
GetListTail(
    IN  PLIST   List
);

PLIST_NODE_HDR
GetNextEntry(
    IN  PLIST_NODE_HDR  ListNode
);

PLIST_NODE_HDR
GetPrevEntry(
    IN  PLIST_NODE_HDR  ListNode
);

VOID
DestroyListNoCallback(
    IN  PLIST           List
);

VOID
DestroyListWithCallback(
    IN  PLIST           List,
    IN  PLIST_CALLBACK  Callback
);


/*****************************************************************************
/* Macro definitions of list functions
/*****************************************************************************/

    /*
    // VOID
    // InitializeList(
    //     IN  PLIST    NewList
    // );
    */
    
    #define InitializeList(nl) \
        { ((PLIST)(nl)) -> Flink = ((PLIST)(nl)) -> Blink = nl; }
    
    /* 
    // VOID
    // InsertHead(
     /     IN  PLIST            List,
    //     IN  PLIST_NODE_HDR   ListNode
    // );
    */
    
    #define InsertHead(List, Node) {                    \
        PLIST_NODE_HDR _EX_Flink;                       \
        PLIST_NODE_HDR _EX_List;                        \
                                                        \
        _EX_List = ((PLIST_NODE_HDR) (List));           \
        _EX_Flink = _EX_List -> Flink;                  \
        ((PLIST_NODE_HDR) (Node))->Blink = _EX_List;    \
        ((PLIST_NODE_HDR) (Node))->Flink = _EX_Flink;   \
        _EX_Flink->Blink = (((PLIST_NODE_HDR) (Node))); \
        _EX_List->Flink = ((PLIST_NODE_HDR) (Node));    \
    }
    
    
    /* 
    // VOID
    // InsertTail(
    //     IN  PLIST            List,
    //     IN  PLIST_NODE_HDR   ListNode
    // );
    */
    
    #define InsertTail(List, Node) {                    \
        PLIST_NODE_HDR _EX_Blink;                       \
        PLIST_NODE_HDR _EX_List;                        \
                                                        \
        _EX_List = ((PLIST_NODE_HDR) (List));           \
        _EX_Blink = _EX_List->Blink;                    \
        ((PLIST_NODE_HDR) (Node))->Flink = _EX_List;    \
        ((PLIST_NODE_HDR) (Node))->Blink = _EX_Blink;   \
        _EX_Blink->Flink = (((PLIST_NODE_HDR) (Node))); \
        _EX_List->Blink = ((PLIST_NODE_HDR) (Node));    \
    }
    
    /*
    //  VOID
    //  RemoveNode(
    //      IN  PLIST_NODE_HDR  ListNode   
    //  );
    */
    
    #define RemoveNode(node) {                          \
        PLIST_NODE_HDR _EX_Blink;                       \
        PLIST_NODE_HDR _EX_Flink;                       \
                                                        \
        _EX_Flink = ((PLIST_NODE_HDR) (node))->Flink;   \
        _EX_Blink = ((PLIST_NODE_HDR) (node))->Blink;   \
        _EX_Blink->Flink = _EX_Flink;                   \
        _EX_Flink->Blink = _EX_Blink;                   \
    }
    
    
    /* 
    // PLIST_NODE_HDR
    // RemoveHead(
    //     IN  PLIST    List
    // );               
    */                  
    
    #define RemoveHead(List)                            \
        GetListHead((List));                            \
        RemoveNode(((PLIST_NODE_HDR) (List))->Flink)  
                        
    /*                  
    // PLIST_NODE_HDR   
    // RemoveTail(      
    //     IN  PLIST    List
    // );               
    */                  
    
    #define RemoveTail(List)                            \
        GetListTail((List));                            \
        RemoveNode(((PLIST_NODE_HDR) (List))->Blink)    
    
    /*                  
    // BOOL             
    // IsListEmpty(     
    //     IN  PLIST    List
    // );               
    */                  
    
    #define IsListEmpty(List)                           \
        (((PLIST_NODE_HDR) (List))->Flink == ((PLIST_NODE_HDR) (List)))
                        
    /*                  
    // PLIST_NODE_HDR   
    // GetListHead(     
    //     IN  PLIST    List
    // );               
    */                  
    
    #define GetListHead(List)                           \
        (((PLIST_NODE_HDR) (List))->Flink)
    
    /*                  
    // PLIST_NODE_HDR   
    // GetListTail(     
    //     IN  PLIST    List
    // );               
    */                  
    
    #define GetListTail(List)                           \
        (((PLIST_NODE_HDR) (List))->Blink)
                        

    /*
    // PLIST_NODE_HDR
    // GetNextEntry(
    //    IN  PLIST_NODE_HDR  ListNode
    // );
    */

    #define GetNextEntry(ListNode)                      \
        (((PLIST_NODE_HDR) (ListNode)) -> Flink);

    /* 
    // PLIST_NODE_HDR
    // GetPrevEntry(
    //    IN  PLIST_NODE_HDR  ListNode
    // );
    */

    #define GetPrevEntry(ListNode)                      \
        (((PLIST_NODE_HDR) (ListNode)) -> Blink);

    /* 
    // VOID
    // DestroyListNoCallback(
    //     IN  PLIST           List,
    //     IN  PLIST_CALLBACK  Callback
    // );
    */ 
    
    #define DestroyListNoCallback(list)                 \
        PLIST_NODE_HDR  currNode;                       \
                                                        \
        while (!IsListEmpty((list))) {                  \
            currNode = RemoveHead((list));              \
        }                                               \
    }

    /* 
    // VOID
    // DestroyListWithCallback(
    //     IN  PLIST           List,
    //     IN  PLIST_CALLBACK  Callback
    // );
    */ 
    
    #define DestroyListWithCallback(list, cb) {         \
        PLIST_NODE_HDR  currNode;                       \
                                                        \
        while (!IsListEmpty((list))) {                  \
            currNode = RemoveHead((list));              \
            (cb)(currNode);                             \
        }                                               \
    }
    
#endif
