/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"
#include <commoncontrols.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

/* Set DUMP_TASKS to 1 to enable a dump of the tasks and task groups every
   5 seconds */
#define DUMP_TASKS  0
#define DEBUG_SHELL_HOOK 0

#define MAX_TASKS_COUNT (0x7FFF)
#define TASK_ITEM_ARRAY_ALLOC   64

const WCHAR szTaskSwitchWndClass [] = TEXT("MSTaskSwWClass");
const WCHAR szRunningApps [] = TEXT("Running Applications");

#if DEBUG_SHELL_HOOK
const struct {
    INT msg;
    LPCWSTR msg_name;
} hshell_msg [] = {
        { HSHELL_WINDOWCREATED, L"HSHELL_WINDOWCREATED" },
        { HSHELL_WINDOWDESTROYED, L"HSHELL_WINDOWDESTROYED" },
        { HSHELL_ACTIVATESHELLWINDOW, L"HSHELL_ACTIVATESHELLWINDOW" },
        { HSHELL_WINDOWACTIVATED, L"HSHELL_WINDOWACTIVATED" },
        { HSHELL_GETMINRECT, L"HSHELL_GETMINRECT" },
        { HSHELL_REDRAW, L"HSHELL_REDRAW" },
        { HSHELL_TASKMAN, L"HSHELL_TASKMAN" },
        { HSHELL_LANGUAGE, L"HSHELL_LANGUAGE" },
        { HSHELL_SYSMENU, L"HSHELL_SYSMENU" },
        { HSHELL_ENDTASK, L"HSHELL_ENDTASK" },
        { HSHELL_ACCESSIBILITYSTATE, L"HSHELL_ACCESSIBILITYSTATE" },
        { HSHELL_APPCOMMAND, L"HSHELL_APPCOMMAND" },
        { HSHELL_WINDOWREPLACED, L"HSHELL_WINDOWREPLACED" },
        { HSHELL_WINDOWREPLACING, L"HSHELL_WINDOWREPLACING" },
        { HSHELL_RUDEAPPACTIVATED, L"HSHELL_RUDEAPPACTIVATED" },
};
#endif

typedef struct _TASK_GROUP
{
    /* We have to use a linked list instead of an array so we don't have to
       update all pointers to groups in the task item array when removing
       groups. */
    struct _TASK_GROUP *Next;

    DWORD dwTaskCount;
    DWORD dwProcessId;
    INT Index;
    union
    {
        DWORD dwFlags;
        struct
        {

            DWORD IsCollapsed : 1;
        };
    };
} TASK_GROUP, *PTASK_GROUP;

typedef struct _TASK_ITEM
{
    HWND hWnd;
    PTASK_GROUP Group;
    INT Index;
    INT IconIndex;

    union
    {
        DWORD dwFlags;
        struct
        {

            /* IsFlashing is TRUE when the task bar item should be flashing. */
            DWORD IsFlashing : 1;

            /* RenderFlashed is only TRUE if the task bar item should be
               drawn with a flash. */
            DWORD RenderFlashed : 1;
        };
    };
} TASK_ITEM, *PTASK_ITEM;

class CTaskToolbar :
    public CWindowImplBaseT< CToolbar<TASK_ITEM>, CControlWinTraits >
{
public:
    INT UpdateTbButtonSpacing(IN BOOL bHorizontal, IN BOOL bThemed, IN UINT uiRows = 0, IN UINT uiBtnsPerLine = 0)
    {
        TBMETRICS tbm;

        tbm.cbSize = sizeof(tbm);
        tbm.dwMask = TBMF_BARPAD | TBMF_BUTTONSPACING;

        tbm.cxBarPad = tbm.cyBarPad = 0;

        if (bThemed)
        {
            tbm.cxButtonSpacing = 0;
            tbm.cyButtonSpacing = 0;
        }
        else
        {
            if (bHorizontal || uiBtnsPerLine > 1)
                tbm.cxButtonSpacing = (3 * GetSystemMetrics(SM_CXEDGE) / 2);
            else
                tbm.cxButtonSpacing = 0;

            if (!bHorizontal || uiRows > 1)
                tbm.cyButtonSpacing = (3 * GetSystemMetrics(SM_CYEDGE) / 2);
            else
                tbm.cyButtonSpacing = 0;
        }

        SetMetrics(&tbm);

        return tbm.cxButtonSpacing;
    }

    VOID BeginUpdate()
    {
        SetRedraw(FALSE);
    }

    VOID EndUpdate()
    {
        SendMessageW(WM_SETREDRAW, TRUE);
        InvalidateRect(NULL, TRUE);
    }

    BOOL SetButtonCommandId(IN INT iButtonIndex, IN INT iCommandId)
    {
        TBBUTTONINFO tbbi;

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND;
        tbbi.idCommand = iCommandId;

        return SetButtonInfo(iButtonIndex, &tbbi) != 0;
    }

public:
    BEGIN_MSG_MAP(CNotifyToolbar)
    END_MSG_MAP()

    BOOL Initialize(HWND hWndParent)
    {
        DWORD styles = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
            TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
            CCS_TOP | CCS_NORESIZE | CCS_NODIVIDER;

        return SubclassWindow(CToolbar::Create(hWndParent, styles));
    }
};

class CTaskSwitchWnd :
    public CWindowImpl < CTaskSwitchWnd, CWindow, CControlWinTraits >
{
    CTaskToolbar m_TaskBar;

    CComPtr<ITrayWindow> m_Tray;

    UINT m_ShellHookMsg;

    WORD m_TaskItemCount;
    WORD m_AllocatedTaskItems;

    PTASK_GROUP m_TaskGroups;
    PTASK_ITEM m_TaskItems;
    PTASK_ITEM m_ActiveTaskItem;

    HTHEME m_Theme;
    UINT m_ButtonsPerLine;
    WORD m_ButtonCount;

    HIMAGELIST m_ImageList;

    BOOL m_IsGroupingEnabled;
    BOOL m_IsDestroying;

    SIZE m_ButtonSize;

public:
    CTaskSwitchWnd() :
        m_ShellHookMsg(NULL),
        m_TaskItemCount(0),
        m_AllocatedTaskItems(0),
        m_TaskGroups(NULL),
        m_TaskItems(NULL),
        m_ActiveTaskItem(NULL),
        m_Theme(NULL),
        m_ButtonsPerLine(0),
        m_ButtonCount(0),
        m_ImageList(NULL),
        m_IsGroupingEnabled(FALSE),
        m_IsDestroying(FALSE)
    {
        ZeroMemory(&m_ButtonSize, sizeof(m_ButtonSize));
    }
    virtual ~CTaskSwitchWnd() { }

    VOID TaskSwitchWnd_UpdateButtonsSize(IN BOOL bRedrawDisabled);

    INT GetWndTextFromTaskItem(IN PTASK_ITEM TaskItem, LPWSTR szBuf, DWORD cchBuf)
    {
        /* Get the window text without sending a message so we don't hang if an
           application isn't responding! */
        return InternalGetWindowText(TaskItem->hWnd, szBuf, cchBuf);
    }


#if DUMP_TASKS != 0
    VOID DumpTasks()
    {
        PTASK_GROUP CurrentGroup;
        PTASK_ITEM CurrentTaskItem, LastTaskItem;

        TRACE("Tasks dump:\n");
        if (m_IsGroupingEnabled)
        {
            CurrentGroup = m_TaskGroups;
            while (CurrentGroup != NULL)
            {
                TRACE("- Group PID: 0x%p Tasks: %d Index: %d\n", CurrentGroup->dwProcessId, CurrentGroup->dwTaskCount, CurrentGroup->Index);

                CurrentTaskItem = m_TaskItems;
                LastTaskItem = CurrentTaskItem + m_TaskItemCount;
                while (CurrentTaskItem != LastTaskItem)
                {
                    if (CurrentTaskItem->Group == CurrentGroup)
                    {
                        TRACE("  + Task hwnd: 0x%p Index: %d\n", CurrentTaskItem->hWnd, CurrentTaskItem->Index);
                    }
                    CurrentTaskItem++;
                }

                CurrentGroup = CurrentGroup->Next;
            }

            CurrentTaskItem = m_TaskItems;
            LastTaskItem = CurrentTaskItem + m_TaskItemCount;
            while (CurrentTaskItem != LastTaskItem)
            {
                if (CurrentTaskItem->Group == NULL)
                {
                    TRACE("- Task hwnd: 0x%p Index: %d\n", CurrentTaskItem->hWnd, CurrentTaskItem->Index);
                }
                CurrentTaskItem++;
            }
        }
        else
        {
            CurrentTaskItem = m_TaskItems;
            LastTaskItem = CurrentTaskItem + m_TaskItemCount;
            while (CurrentTaskItem != LastTaskItem)
            {
                TRACE("- Task hwnd: 0x%p Index: %d\n", CurrentTaskItem->hWnd, CurrentTaskItem->Index);
                CurrentTaskItem++;
            }
        }
    }
#endif

    VOID UpdateIndexesAfter(IN INT iIndex, BOOL bInserted)
    {
        PTASK_GROUP CurrentGroup;
        PTASK_ITEM CurrentTaskItem, LastTaskItem;
        INT NewIndex;

        int offset = bInserted ? +1 : -1;

        if (m_IsGroupingEnabled)
        {
            /* Update all affected groups */
            CurrentGroup = m_TaskGroups;
            while (CurrentGroup != NULL)
            {
                if (CurrentGroup->IsCollapsed &&
                    CurrentGroup->Index >= iIndex)
                {
                    /* Update the toolbar buttons */
                    NewIndex = CurrentGroup->Index + offset;
                    if (m_TaskBar.SetButtonCommandId(CurrentGroup->Index + offset, NewIndex))
                    {
                        CurrentGroup->Index = NewIndex;
                    }
                    else
                        CurrentGroup->Index = -1;
                }

                CurrentGroup = CurrentGroup->Next;
            }
        }

        /* Update all affected task items */
        CurrentTaskItem = m_TaskItems;
        LastTaskItem = CurrentTaskItem + m_TaskItemCount;
        while (CurrentTaskItem != LastTaskItem)
        {
            CurrentGroup = CurrentTaskItem->Group;
            if (CurrentGroup != NULL)
            {
                if (!CurrentGroup->IsCollapsed &&
                    CurrentTaskItem->Index >= iIndex)
                {
                    goto UpdateTaskItemBtn;
                }
            }
            else if (CurrentTaskItem->Index >= iIndex)
            {
            UpdateTaskItemBtn:
                /* Update the toolbar buttons */
                NewIndex = CurrentTaskItem->Index + offset;
                if (m_TaskBar.SetButtonCommandId(CurrentTaskItem->Index + offset, NewIndex))
                {
                    CurrentTaskItem->Index = NewIndex;
                }
                else
                    CurrentTaskItem->Index = -1;
            }

            CurrentTaskItem++;
        }
    }


    INT UpdateTaskGroupButton(IN PTASK_GROUP TaskGroup)
    {
        ASSERT(TaskGroup->Index >= 0);

        /* FIXME: Implement */

        return TaskGroup->Index;
    }

    VOID ExpandTaskGroup(IN PTASK_GROUP TaskGroup)
    {
        ASSERT(TaskGroup->dwTaskCount > 0);
        ASSERT(TaskGroup->IsCollapsed);
        ASSERT(TaskGroup->Index >= 0);

        /* FIXME: Implement */
    }

    HICON GetWndIcon(HWND hwnd)
    {
        HICON hIcon = 0;

        SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR) &hIcon);
        if (hIcon)
            return hIcon;

        SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR) &hIcon);
        if (hIcon)
            return hIcon;

        SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR) &hIcon);
        if (hIcon)
            return hIcon;

        hIcon = (HICON) GetClassLongPtr(hwnd, GCL_HICONSM);
        if (hIcon)
            return hIcon;

        hIcon = (HICON) GetClassLongPtr(hwnd, GCL_HICON);

        return hIcon;
    }

    INT UpdateTaskItemButton(IN PTASK_ITEM TaskItem)
    {
        TBBUTTONINFO tbbi = { 0 };
        HICON icon;
        WCHAR windowText[255];

        ASSERT(TaskItem->Index >= 0);

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_BYINDEX | TBIF_STATE | TBIF_TEXT | TBIF_IMAGE;
        tbbi.fsState = TBSTATE_ENABLED;
        if (m_ActiveTaskItem == TaskItem)
            tbbi.fsState |= TBSTATE_CHECKED;

        if (TaskItem->RenderFlashed)
            tbbi.fsState |= TBSTATE_MARKED;

        /* Check if we're updating a button that is the last one in the
           line. If so, we need to set the TBSTATE_WRAP flag! */
        if (!m_Tray->IsHorizontal() || (m_ButtonsPerLine != 0 &&
            (TaskItem->Index + 1) % m_ButtonsPerLine == 0))
        {
            tbbi.fsState |= TBSTATE_WRAP;
        }

        if (GetWndTextFromTaskItem(TaskItem, windowText, _countof(windowText)) > 0)
        {
            tbbi.pszText = windowText;
        }

        icon = GetWndIcon(TaskItem->hWnd);
        if (!icon)
            icon = static_cast<HICON>(LoadImage(NULL, MAKEINTRESOURCE(OIC_SAMPLE), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
        TaskItem->IconIndex = ImageList_ReplaceIcon(m_ImageList, TaskItem->IconIndex, icon);
        tbbi.iImage = TaskItem->IconIndex;

        if (!m_TaskBar.SetButtonInfo(TaskItem->Index, &tbbi))
        {
            TaskItem->Index = -1;
            return -1;
        }

        TRACE("Updated button %d for hwnd 0x%p\n", TaskItem->Index, TaskItem->hWnd);
        return TaskItem->Index;
    }

    VOID RemoveIcon(IN PTASK_ITEM TaskItem)
    {
        TBBUTTONINFO tbbi;
        PTASK_ITEM currentTaskItem, LastItem;

        if (TaskItem->IconIndex == -1)
            return;

        tbbi.cbSize = sizeof(tbbi);
        tbbi.dwMask = TBIF_IMAGE;

        currentTaskItem = m_TaskItems;
        LastItem = currentTaskItem + m_TaskItemCount;
        while (currentTaskItem != LastItem)
        {
            if (currentTaskItem->IconIndex > TaskItem->IconIndex)
            {
                currentTaskItem->IconIndex--;
                tbbi.iImage = currentTaskItem->IconIndex;

                m_TaskBar.SetButtonInfo(currentTaskItem->Index, &tbbi);
            }
            currentTaskItem++;
        }

        ImageList_Remove(m_ImageList, TaskItem->IconIndex);
    }

    PTASK_ITEM FindLastTaskItemOfGroup(
        IN PTASK_GROUP TaskGroup  OPTIONAL,
        IN PTASK_ITEM NewTaskItem  OPTIONAL)
    {
        PTASK_ITEM TaskItem, LastTaskItem, FoundTaskItem = NULL;
        DWORD dwTaskCount;

        ASSERT(m_IsGroupingEnabled);

        TaskItem = m_TaskItems;
        LastTaskItem = TaskItem + m_TaskItemCount;

        dwTaskCount = (TaskGroup != NULL ? TaskGroup->dwTaskCount : MAX_TASKS_COUNT);

        ASSERT(dwTaskCount > 0);

        while (TaskItem != LastTaskItem)
        {
            if (TaskItem->Group == TaskGroup)
            {
                if ((NewTaskItem != NULL && TaskItem != NewTaskItem) || NewTaskItem == NULL)
                {
                    FoundTaskItem = TaskItem;
                }

                if (--dwTaskCount == 0)
                {
                    /* We found the last task item in the group! */
                    break;
                }
            }

            TaskItem++;
        }

        return FoundTaskItem;
    }

    INT CalculateTaskItemNewButtonIndex(IN PTASK_ITEM TaskItem)
    {
        PTASK_GROUP TaskGroup;
        PTASK_ITEM LastTaskItem;

        /* NOTE: This routine assumes that the group is *not* collapsed! */

        TaskGroup = TaskItem->Group;
        if (m_IsGroupingEnabled)
        {
            if (TaskGroup != NULL)
            {
                ASSERT(TaskGroup->Index < 0);
                ASSERT(!TaskGroup->IsCollapsed);

                if (TaskGroup->dwTaskCount > 1)
                {
                    LastTaskItem = FindLastTaskItemOfGroup(TaskGroup, TaskItem);
                    if (LastTaskItem != NULL)
                    {
                        /* Since the group is expanded the task items must have an index */
                        ASSERT(LastTaskItem->Index >= 0);

                        return LastTaskItem->Index + 1;
                    }
                }
            }
            else
            {
                /* Find the last NULL group button. NULL groups are added at the end of the
                   task item list when grouping is enabled */
                LastTaskItem = FindLastTaskItemOfGroup(NULL, TaskItem);
                if (LastTaskItem != NULL)
                {
                    ASSERT(LastTaskItem->Index >= 0);

                    return LastTaskItem->Index + 1;
                }
            }
        }

        return m_ButtonCount;
    }

    INT AddTaskItemButton(IN OUT PTASK_ITEM TaskItem)
    {
        WCHAR windowText[255];
        TBBUTTON tbBtn = { 0 };
        INT iIndex;
        HICON icon;

        if (TaskItem->Index >= 0)
        {
            return UpdateTaskItemButton(TaskItem);
        }

        if (TaskItem->Group != NULL &&
            TaskItem->Group->IsCollapsed)
        {
            /* The task group is collapsed, we only need to update the group button */
            return UpdateTaskGroupButton(TaskItem->Group);
        }

        icon = GetWndIcon(TaskItem->hWnd);
        if (!icon)
            icon = static_cast<HICON>(LoadImage(NULL, MAKEINTRESOURCE(OIC_SAMPLE), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE));
        TaskItem->IconIndex = ImageList_ReplaceIcon(m_ImageList, -1, icon);

        tbBtn.iBitmap = TaskItem->IconIndex;
        tbBtn.fsState = TBSTATE_ENABLED | TBSTATE_ELLIPSES;
        tbBtn.fsStyle = BTNS_CHECK | BTNS_NOPREFIX | BTNS_SHOWTEXT;
        tbBtn.dwData = TaskItem->Index;

        if (GetWndTextFromTaskItem(TaskItem, windowText, _countof(windowText)) > 0)
        {
            tbBtn.iString = (DWORD_PTR) windowText;
        }

        /* Find out where to insert the new button */
        iIndex = CalculateTaskItemNewButtonIndex(TaskItem);
        ASSERT(iIndex >= 0);
        tbBtn.idCommand = iIndex;

        m_TaskBar.BeginUpdate();

        if (m_TaskBar.InsertButton(iIndex, &tbBtn))
        {
            UpdateIndexesAfter(iIndex, TRUE);

            TRACE("Added button %d for hwnd 0x%p\n", iIndex, TaskItem->hWnd);

            TaskItem->Index = iIndex;
            m_ButtonCount++;

            /* Update button sizes and fix the button wrapping */
            UpdateButtonsSize(TRUE);
            return iIndex;
        }

        m_TaskBar.EndUpdate();

        return -1;
    }

    BOOL DeleteTaskItemButton(IN OUT PTASK_ITEM TaskItem)
    {
        PTASK_GROUP TaskGroup;
        INT iIndex;

        TaskGroup = TaskItem->Group;

        if (TaskItem->Index >= 0)
        {
            if ((TaskGroup != NULL && !TaskGroup->IsCollapsed) ||
                TaskGroup == NULL)
            {
                m_TaskBar.BeginUpdate();

                RemoveIcon(TaskItem);
                iIndex = TaskItem->Index;
                if (m_TaskBar.DeleteButton(iIndex))
                {
                    TaskItem->Index = -1;
                    m_ButtonCount--;

                    UpdateIndexesAfter(iIndex, FALSE);

                    /* Update button sizes and fix the button wrapping */
                    UpdateButtonsSize(TRUE);
                    return TRUE;
                }

                m_TaskBar.EndUpdate();
            }
        }

        return FALSE;
    }

    PTASK_GROUP AddToTaskGroup(IN HWND hWnd)
    {
        DWORD dwProcessId;
        PTASK_GROUP TaskGroup, *PrevLink;

        if (!GetWindowThreadProcessId(hWnd,
            &dwProcessId))
        {
            TRACE("Cannot get process id of hwnd 0x%p\n", hWnd);
            return NULL;
        }

        /* Try to find an existing task group */
        TaskGroup = m_TaskGroups;
        PrevLink = &m_TaskGroups;
        while (TaskGroup != NULL)
        {
            if (TaskGroup->dwProcessId == dwProcessId)
            {
                TaskGroup->dwTaskCount++;
                return TaskGroup;
            }

            PrevLink = &TaskGroup->Next;
            TaskGroup = TaskGroup->Next;
        }

        /* Allocate a new task group */
        TaskGroup = (PTASK_GROUP) HeapAlloc(hProcessHeap,
            HEAP_ZERO_MEMORY,
            sizeof(*TaskGroup));
        if (TaskGroup != NULL)
        {
            TaskGroup->dwTaskCount = 1;
            TaskGroup->dwProcessId = dwProcessId;
            TaskGroup->Index = -1;

            /* Add the task group to the list */
            *PrevLink = TaskGroup;
        }

        return TaskGroup;
    }

    VOID RemoveTaskFromTaskGroup(IN OUT PTASK_ITEM TaskItem)
    {
        PTASK_GROUP TaskGroup, CurrentGroup, *PrevLink;

        TaskGroup = TaskItem->Group;
        if (TaskGroup != NULL)
        {
            DWORD dwNewTaskCount = --TaskGroup->dwTaskCount;
            if (dwNewTaskCount == 0)
            {
                /* Find the previous pointer in the chain */
                CurrentGroup = m_TaskGroups;
                PrevLink = &m_TaskGroups;
                while (CurrentGroup != TaskGroup)
                {
                    PrevLink = &CurrentGroup->Next;
                    CurrentGroup = CurrentGroup->Next;
                }

                /* Remove the group from the list */
                ASSERT(TaskGroup == CurrentGroup);
                *PrevLink = TaskGroup->Next;

                /* Free the task group */
                HeapFree(hProcessHeap,
                    0,
                    TaskGroup);
            }
            else if (TaskGroup->IsCollapsed &&
                TaskGroup->Index >= 0)
            {
                if (dwNewTaskCount > 1)
                {
                    /* FIXME: Check if we should expand the group */
                    /* Update the task group button */
                    UpdateTaskGroupButton(TaskGroup);
                }
                else
                {
                    /* Expand the group of one task button to a task button */
                    ExpandTaskGroup(TaskGroup);
                }
            }
        }
    }

    PTASK_ITEM FindTaskItem(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem, LastItem;

        TaskItem = m_TaskItems;
        LastItem = TaskItem + m_TaskItemCount;
        while (TaskItem != LastItem)
        {
            if (TaskItem->hWnd == hWnd)
                return TaskItem;

            TaskItem++;
        }

        return NULL;
    }

    PTASK_ITEM FindOtherTaskItem(IN HWND hWnd)
    {
        PTASK_ITEM LastItem, TaskItem;
        PTASK_GROUP TaskGroup;
        DWORD dwProcessId;

        if (!GetWindowThreadProcessId(hWnd, &dwProcessId))
        {
            return NULL;
        }

        /* Try to find another task that belongs to the same
           process as the given window */
        TaskItem = m_TaskItems;
        LastItem = TaskItem + m_TaskItemCount;
        while (TaskItem != LastItem)
        {
            TaskGroup = TaskItem->Group;
            if (TaskGroup != NULL)
            {
                if (TaskGroup->dwProcessId == dwProcessId)
                    return TaskItem;
            }
            else
            {
                DWORD dwProcessIdTask;

                if (GetWindowThreadProcessId(TaskItem->hWnd,
                    &dwProcessIdTask) &&
                    dwProcessIdTask == dwProcessId)
                {
                    return TaskItem;
                }
            }

            TaskItem++;
        }

        return NULL;
    }

    PTASK_ITEM AllocTaskItem()
    {
        if (m_TaskItemCount >= MAX_TASKS_COUNT)
        {
            /* We need the most significant bit in 16 bit command IDs to indicate whether it
               is a task group or task item. WM_COMMAND limits command IDs to 16 bits! */
            return NULL;
        }

        ASSERT(m_AllocatedTaskItems >= m_TaskItemCount);

        if (m_TaskItemCount == 0)
        {
            m_TaskItems = (PTASK_ITEM) HeapAlloc(hProcessHeap,
                0,
                TASK_ITEM_ARRAY_ALLOC * sizeof(*m_TaskItems));
            if (m_TaskItems != NULL)
            {
                m_AllocatedTaskItems = TASK_ITEM_ARRAY_ALLOC;
            }
            else
                return NULL;
        }
        else if (m_TaskItemCount >= m_AllocatedTaskItems)
        {
            PTASK_ITEM NewArray;
            SIZE_T NewArrayLength, ActiveTaskItemIndex;

            NewArrayLength = m_AllocatedTaskItems + TASK_ITEM_ARRAY_ALLOC;

            NewArray = (PTASK_ITEM) HeapReAlloc(hProcessHeap,
                0,
                m_TaskItems,
                NewArrayLength * sizeof(*m_TaskItems));
            if (NewArray != NULL)
            {
                if (m_ActiveTaskItem != NULL)
                {
                    /* Fixup the ActiveTaskItem pointer */
                    ActiveTaskItemIndex = m_ActiveTaskItem - m_TaskItems;
                    m_ActiveTaskItem = NewArray + ActiveTaskItemIndex;
                }
                m_AllocatedTaskItems = (WORD) NewArrayLength;
                m_TaskItems = NewArray;
            }
            else
                return NULL;
        }

        return m_TaskItems + m_TaskItemCount++;
    }

    VOID FreeTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        WORD wIndex;

        if (TaskItem == m_ActiveTaskItem)
            m_ActiveTaskItem = NULL;

        wIndex = (WORD) (TaskItem - m_TaskItems);
        if (wIndex + 1 < m_TaskItemCount)
        {
            MoveMemory(TaskItem,
                TaskItem + 1,
                (m_TaskItemCount - wIndex - 1) * sizeof(*TaskItem));
        }

        m_TaskItemCount--;
    }

    VOID DeleteTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        if (!m_IsDestroying)
        {
            /* Delete the task button from the toolbar */
            DeleteTaskItemButton(TaskItem);
        }

        /* Remove the task from it's group */
        RemoveTaskFromTaskGroup(TaskItem);

        /* Free the task item */
        FreeTaskItem(TaskItem);
    }

    VOID CheckActivateTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        PTASK_ITEM CurrentTaskItem;
        PTASK_GROUP TaskGroup = NULL;

        CurrentTaskItem = m_ActiveTaskItem;

        if (TaskItem != NULL)
            TaskGroup = TaskItem->Group;

        if (m_IsGroupingEnabled &&
            TaskGroup != NULL &&
            TaskGroup->IsCollapsed)
        {
            /* FIXME */
            return;
        }

        if (CurrentTaskItem != NULL)
        {
            PTASK_GROUP CurrentTaskGroup;

            if (CurrentTaskItem == TaskItem)
                return;

            CurrentTaskGroup = CurrentTaskItem->Group;

            if (m_IsGroupingEnabled &&
                CurrentTaskGroup != NULL &&
                CurrentTaskGroup->IsCollapsed)
            {
                if (CurrentTaskGroup == TaskGroup)
                    return;

                /* FIXME */
            }
            else
            {
                m_ActiveTaskItem = NULL;
                if (CurrentTaskItem->Index >= 0)
                {
                    UpdateTaskItemButton(CurrentTaskItem);
                }
            }
        }

        m_ActiveTaskItem = TaskItem;

        if (TaskItem != NULL && TaskItem->Index >= 0)
        {
            UpdateTaskItemButton(TaskItem);
        }
        else if (TaskItem == NULL)
        {
            TRACE("Active TaskItem now NULL\n");
        }
    }

    PTASK_ITEM FindTaskItemByIndex(IN INT Index)
    {
        PTASK_ITEM TaskItem, LastItem;

        TaskItem = m_TaskItems;
        LastItem = TaskItem + m_TaskItemCount;
        while (TaskItem != LastItem)
        {
            if (TaskItem->Index == Index)
                return TaskItem;

            TaskItem++;
        }

        return NULL;
    }

    PTASK_GROUP FindTaskGroupByIndex(IN INT Index)
    {
        PTASK_GROUP CurrentGroup;

        CurrentGroup = m_TaskGroups;
        while (CurrentGroup != NULL)
        {
            if (CurrentGroup->Index == Index)
                break;

            CurrentGroup = CurrentGroup->Next;
        }

        return CurrentGroup;
    }

    BOOL AddTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        if (!::IsWindow(hWnd) || m_Tray->IsSpecialHWND(hWnd))
            return FALSE;

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem == NULL)
        {
            TRACE("Add window 0x%p\n", hWnd);
            TaskItem = AllocTaskItem();
            if (TaskItem != NULL)
            {
                ZeroMemory(TaskItem,
                    sizeof(*TaskItem));
                TaskItem->hWnd = hWnd;
                TaskItem->Index = -1;
                TaskItem->Group = AddToTaskGroup(hWnd);

                if (!m_IsDestroying)
                {
                    AddTaskItemButton(TaskItem);
                }
            }
        }

        return TaskItem != NULL;
    }

    BOOL ActivateTaskItem(IN OUT PTASK_ITEM TaskItem  OPTIONAL)
    {
        if (TaskItem != NULL)
        {
            TRACE("Activate window 0x%p on button %d\n", TaskItem->hWnd, TaskItem->Index);
        }

        CheckActivateTaskItem(TaskItem);

        return FALSE;
    }

    BOOL ActivateTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        if (!hWnd)
        {
            return ActivateTaskItem(NULL);
        }

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem == NULL)
        {
            TaskItem = FindOtherTaskItem(hWnd);
        }

        if (TaskItem == NULL)
        {
            WARN("Activate window 0x%p, could not find task\n", hWnd);
            RefreshWindowList();
        }

        return ActivateTaskItem(TaskItem);
    }

    BOOL DeleteTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem != NULL)
        {
            TRACE("Delete window 0x%p on button %d\n", hWnd, TaskItem->Index);
            DeleteTaskItem(TaskItem);
            return TRUE;
        }
        //else
        //TRACE("Failed to delete window 0x%p\n", hWnd);

        return FALSE;
    }

    VOID DeleteAllTasks()
    {
        PTASK_ITEM CurrentTask;

        if (m_TaskItemCount > 0)
        {
            CurrentTask = m_TaskItems + m_TaskItemCount;
            do
            {
                DeleteTaskItem(--CurrentTask);
            } while (CurrentTask != m_TaskItems);
        }
    }

    VOID FlashTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        TaskItem->RenderFlashed = 1;
        UpdateTaskItemButton(TaskItem);
    }

    BOOL FlashTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem != NULL)
        {
            TRACE("Flashing window 0x%p on button %d\n", hWnd, TaskItem->Index);
            FlashTaskItem(TaskItem);
            return TRUE;
        }

        return FALSE;
    }

    VOID RedrawTaskItem(IN OUT PTASK_ITEM TaskItem)
    {
        PTASK_GROUP TaskGroup;

        TaskGroup = TaskItem->Group;
        if (m_IsGroupingEnabled && TaskGroup != NULL)
        {
            if (TaskGroup->IsCollapsed && TaskGroup->Index >= 0)
            {
                UpdateTaskGroupButton(TaskGroup);
            }
            else if (TaskItem->Index >= 0)
            {
                goto UpdateTaskItem;
            }
        }
        else if (TaskItem->Index >= 0)
        {
        UpdateTaskItem:
            TaskItem->RenderFlashed = 0;
            UpdateTaskItemButton(TaskItem);
        }
    }


    BOOL RedrawTask(IN HWND hWnd)
    {
        PTASK_ITEM TaskItem;

        TaskItem = FindTaskItem(hWnd);
        if (TaskItem != NULL)
        {
            RedrawTaskItem(TaskItem);
            return TRUE;
        }

        return FALSE;
    }

    VOID UpdateButtonsSize(IN BOOL bRedrawDisabled)
    {
        RECT rcClient;
        UINT uiRows, uiMax, uiMin, uiBtnsPerLine, ui;
        LONG NewBtnSize;
        BOOL Horizontal;

        if (GetClientRect(&rcClient) && !IsRectEmpty(&rcClient))
        {
            if (m_ButtonCount > 0)
            {
                Horizontal = m_Tray->IsHorizontal();

                if (Horizontal)
                {
                    TBMETRICS tbm = { 0 };
                    tbm.cbSize = sizeof(tbm);
                    tbm.dwMask = TBMF_BUTTONSPACING;
                    m_TaskBar.GetMetrics(&tbm);

                    uiRows = (rcClient.bottom + tbm.cyButtonSpacing) / (m_ButtonSize.cy + tbm.cyButtonSpacing);
                    if (uiRows == 0)
                        uiRows = 1;

                    uiBtnsPerLine = (m_ButtonCount + uiRows - 1) / uiRows;
                }
                else
                {
                    uiBtnsPerLine = 1;
                    uiRows = m_ButtonCount;
                }

                if (!bRedrawDisabled)
                    m_TaskBar.BeginUpdate();

                /* We might need to update the button spacing */
                int cxButtonSpacing = m_TaskBar.UpdateTbButtonSpacing(
                    Horizontal, m_Theme != NULL,
                    uiRows, uiBtnsPerLine);

                /* Determine the minimum and maximum width of a button */
                uiMin = GetSystemMetrics(SM_CXSIZE) + (2 * GetSystemMetrics(SM_CXEDGE));
                if (Horizontal)
                {
                    uiMax = GetSystemMetrics(SM_CXMINIMIZED);

                    /* Calculate the ideal width and make sure it's within the allowed range */
                    NewBtnSize = (rcClient.right - (uiBtnsPerLine * cxButtonSpacing)) / uiBtnsPerLine;

                    if (NewBtnSize < (LONG) uiMin)
                        NewBtnSize = uiMin;
                    if (NewBtnSize >(LONG)uiMax)
                        NewBtnSize = uiMax;

                    /* Recalculate how many buttons actually fit into one line */
                    uiBtnsPerLine = rcClient.right / (NewBtnSize + cxButtonSpacing);
                    if (uiBtnsPerLine == 0)
                        uiBtnsPerLine++;
                }
                else
                {
                    NewBtnSize = uiMax = rcClient.right;
                }

                m_ButtonSize.cx = NewBtnSize;

                m_ButtonsPerLine = uiBtnsPerLine;

                for (ui = 0; ui != m_ButtonCount; ui++)
                {
                    TBBUTTONINFOW tbbi = { 0 };
                    tbbi.cbSize = sizeof(tbbi);
                    tbbi.dwMask = TBIF_BYINDEX | TBIF_SIZE | TBIF_STATE;
                    tbbi.cx = (INT) NewBtnSize;
                    tbbi.fsState = TBSTATE_ENABLED;

                    /* Check if we're updating a button that is the last one in the
                       line. If so, we need to set the TBSTATE_WRAP flag! */
                    if (Horizontal)
                    {
                        if ((ui + 1) % uiBtnsPerLine == 0)
                            tbbi.fsState |= TBSTATE_WRAP;
                    }
                    else
                    {
                        tbbi.fsState |= TBSTATE_WRAP;
                    }

                    if (m_ActiveTaskItem != NULL &&
                        m_ActiveTaskItem->Index == (INT)ui)
                    {
                        tbbi.fsState |= TBSTATE_CHECKED;
                    }

                    m_TaskBar.SetButtonInfo(ui, &tbbi);
                }
            }
            else
            {
                m_ButtonsPerLine = 0;
                m_ButtonSize.cx = 0;
            }
        }

        // FIXME: This seems to be enabling redraws prematurely, but moving it to its right place doesn't work!
        m_TaskBar.EndUpdate();
    }

    BOOL CALLBACK EnumWindowsProc(IN HWND hWnd)
    {
        /* Only show windows that still exist and are visible and none of explorer's
        special windows (such as the desktop or the tray window) */
        if (::IsWindow(hWnd) && ::IsWindowVisible(hWnd) &&
            !m_Tray->IsSpecialHWND(hWnd))
        {
            DWORD exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
            /* Don't list popup windows and also no tool windows */
            if ((GetWindow(hWnd, GW_OWNER) == NULL || exStyle & WS_EX_APPWINDOW) &&
                !(exStyle & WS_EX_TOOLWINDOW))
            {
                TRACE("Adding task for %p...\n", hWnd);
                AddTask(hWnd);
            }

        }

        return TRUE;
    }

    static BOOL CALLBACK s_EnumWindowsProc(IN HWND hWnd, IN LPARAM lParam)
    {
        CTaskSwitchWnd * This = (CTaskSwitchWnd *) lParam;

        return This->EnumWindowsProc(hWnd);
    }

    BOOL RefreshWindowList()
    {
        TRACE("Refreshing window list...\n");
        /* Add all windows to the toolbar */
        return EnumWindows(s_EnumWindowsProc, (LPARAM)this);
    }

    LRESULT OnThemeChanged()
    {
        TRACE("OmThemeChanged\n");

        if (m_Theme)
            CloseThemeData(m_Theme);

        if (IsThemeActive())
            m_Theme = OpenThemeData(m_hWnd, L"TaskBand");
        else
            m_Theme = NULL;

        return TRUE;
    }

    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return OnThemeChanged();
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (!m_TaskBar.Initialize(m_hWnd))
            return FALSE;

        SetWindowTheme(m_TaskBar.m_hWnd, L"TaskBand", NULL);
        OnThemeChanged();

        m_ImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 1000);
        m_TaskBar.SetImageList(m_ImageList);

        /* Calculate the default button size. Don't save this in m_ButtonSize.cx so that
        the actual button width gets updated correctly on the first recalculation */
        int cx = GetSystemMetrics(SM_CXMINIMIZED);
        int cy = m_ButtonSize.cy = GetSystemMetrics(SM_CYSIZE) + (2 * GetSystemMetrics(SM_CYEDGE));
        m_TaskBar.SetButtonSize(cx, cy);

        /* Set proper spacing between buttons */
        m_TaskBar.UpdateTbButtonSpacing(m_Tray->IsHorizontal(), m_Theme != NULL);

        /* Register the shell hook */
        m_ShellHookMsg = RegisterWindowMessage(TEXT("SHELLHOOK"));

        TRACE("ShellHookMsg got assigned number %d\n", m_ShellHookMsg);

        RegisterShellHook(m_hWnd, 3); /* 1 if no NT! We're targeting NT so we don't care! */

        RefreshWindowList();

        /* Recalculate the button size */
        UpdateButtonsSize(FALSE);

#if DUMP_TASKS != 0
        SetTimer(hwnd, 1, 5000, NULL);
#endif
        return TRUE;
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_IsDestroying = TRUE;

        /* Unregister the shell hook */
        RegisterShellHook(m_hWnd, FALSE);

        CloseThemeData(m_Theme);
        DeleteAllTasks();
        return TRUE;
    }

    BOOL HandleAppCommand(IN WPARAM wParam, IN LPARAM lParam)
    {
        BOOL Ret = FALSE;

        switch (GET_APPCOMMAND_LPARAM(lParam))
        {
        case APPCOMMAND_BROWSER_SEARCH:
            Ret = SHFindFiles(NULL,
                NULL);
            break;

        case APPCOMMAND_BROWSER_HOME:
        case APPCOMMAND_LAUNCH_MAIL:
        default:
            TRACE("Shell app command %d unhandled!\n", (INT) GET_APPCOMMAND_LPARAM(lParam));
            break;
        }

        return Ret;
    }

    LRESULT HandleShellHookMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        BOOL Ret = FALSE;

        /* In case the shell hook wasn't registered properly, ignore WM_NULLs*/
        if (uMsg == 0)
        {
            bHandled = FALSE;
            return 0;
        }

        TRACE("Received shell hook message: wParam=%08lx, lParam=%08lx\n", wParam, lParam);

        switch ((INT) wParam)
        {
        case HSHELL_APPCOMMAND:
            HandleAppCommand(wParam, lParam);
            Ret = TRUE;
            break;

        case HSHELL_WINDOWCREATED:
            Ret = AddTask((HWND) lParam);
            break;

        case HSHELL_WINDOWDESTROYED:
            /* The window still exists! Delay destroying it a bit */
            DeleteTask((HWND) lParam);
            Ret = TRUE;
            break;

        case HSHELL_RUDEAPPACTIVATED:
        case HSHELL_WINDOWACTIVATED:
            if (lParam)
            {
                ActivateTask((HWND) lParam);
                Ret = TRUE;
            }
            break;

        case HSHELL_FLASH:
            FlashTask((HWND) lParam);
            Ret = TRUE;
            break;

        case HSHELL_REDRAW:
            RedrawTask((HWND) lParam);
            Ret = TRUE;
            break;

        case HSHELL_TASKMAN:
            PostMessage(m_Tray->GetHWND(), TWM_OPENSTARTMENU, 0, 0);
            break;

        case HSHELL_ACTIVATESHELLWINDOW:
        case HSHELL_LANGUAGE:
        case HSHELL_SYSMENU:
        case HSHELL_ENDTASK:
        case HSHELL_ACCESSIBILITYSTATE:
        case HSHELL_WINDOWREPLACED:
        case HSHELL_WINDOWREPLACING:

        case HSHELL_GETMINRECT:
        default:
        {
#if DEBUG_SHELL_HOOK
            int i, found;
            for (i = 0, found = 0; i != sizeof(hshell_msg) / sizeof(hshell_msg[0]); i++)
            {
                if (hshell_msg[i].msg == (INT) wParam)
                {
                    TRACE("Shell message %ws unhandled (lParam = 0x%p)!\n", hshell_msg[i].msg_name, lParam);
                    found = 1;
                    break;
                }
            }
            if (found)
                break;
#endif
            TRACE("Shell message %d unhandled (lParam = 0x%p)!\n", (INT) wParam, lParam);
            break;
        }
        }

        return Ret;
    }

    VOID EnableGrouping(IN BOOL bEnable)
    {
        m_IsGroupingEnabled = bEnable;

        /* Collapse or expand groups if neccessary */
        UpdateButtonsSize(FALSE);
    }

    VOID HandleTaskItemClick(IN OUT PTASK_ITEM TaskItem)
    {
        BOOL bIsMinimized;
        BOOL bIsActive;

        if (::IsWindow(TaskItem->hWnd))
        {
            bIsMinimized = IsIconic(TaskItem->hWnd);
            bIsActive = (TaskItem == m_ActiveTaskItem);

            TRACE("Active TaskItem %p, selected TaskItem %p\n", m_ActiveTaskItem, TaskItem);
            if (m_ActiveTaskItem)
                TRACE("Active TaskItem hWnd=%p, TaskItem hWnd %p\n", m_ActiveTaskItem->hWnd, TaskItem->hWnd);

            TRACE("Valid button clicked. HWND=%p, IsMinimized=%s, IsActive=%s...\n",
                TaskItem->hWnd, bIsMinimized ? "Yes" : "No", bIsActive ? "Yes" : "No");

            if (!bIsMinimized && bIsActive)
            {
                PostMessage(TaskItem->hWnd,
                    WM_SYSCOMMAND,
                    SC_MINIMIZE,
                    0);
                TRACE("Valid button clicked. App window Minimized.\n");
            }
            else
            {
                if (bIsMinimized)
                {
                    PostMessage(TaskItem->hWnd,
                        WM_SYSCOMMAND,
                        SC_RESTORE,
                        0);
                    TRACE("Valid button clicked. App window Restored.\n");
                }

                SetForegroundWindow(TaskItem->hWnd);
                TRACE("Valid button clicked. App window Activated.\n");
            }
        }
    }

    VOID HandleTaskGroupClick(IN OUT PTASK_GROUP TaskGroup)
    {
        /* TODO: Show task group menu */
    }

    BOOL HandleButtonClick(IN WORD wIndex)
    {
        PTASK_ITEM TaskItem;
        PTASK_GROUP TaskGroup;

        if (m_IsGroupingEnabled)
        {
            TaskGroup = FindTaskGroupByIndex((INT) wIndex);
            if (TaskGroup != NULL && TaskGroup->IsCollapsed)
            {
                HandleTaskGroupClick(TaskGroup);
                return TRUE;
            }
        }

        TaskItem = FindTaskItemByIndex((INT) wIndex);
        if (TaskItem != NULL)
        {
            HandleTaskItemClick(TaskItem);
            return TRUE;
        }

        return FALSE;
    }


    VOID HandleTaskItemRightClick(IN OUT PTASK_ITEM TaskItem)
    {

        HMENU hmenu = GetSystemMenu(TaskItem->hWnd, FALSE);

        if (hmenu)
        {
            POINT pt;
            int cmd;
            GetCursorPos(&pt);
            cmd = TrackPopupMenu(hmenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, m_TaskBar.m_hWnd, NULL);
            if (cmd)
            {
                SetForegroundWindow(TaskItem->hWnd);    // reactivate window after the context menu has closed
                PostMessage(TaskItem->hWnd, WM_SYSCOMMAND, cmd, 0);
            }
        }
    }

    VOID HandleTaskGroupRightClick(IN OUT PTASK_GROUP TaskGroup)
    {
        /* TODO: Show task group right click menu */
    }

    BOOL HandleButtonRightClick(IN WORD wIndex)
    {
        PTASK_ITEM TaskItem;
        PTASK_GROUP TaskGroup;
        if (m_IsGroupingEnabled)
        {
            TaskGroup = FindTaskGroupByIndex((INT) wIndex);
            if (TaskGroup != NULL && TaskGroup->IsCollapsed)
            {
                HandleTaskGroupRightClick(TaskGroup);
                return TRUE;
            }
        }

        TaskItem = FindTaskItemByIndex((INT) wIndex);

        if (TaskItem != NULL)
        {
            HandleTaskItemRightClick(TaskItem);
            return TRUE;
        }

        return FALSE;
    }


    LRESULT HandleItemPaint(IN OUT NMTBCUSTOMDRAW *nmtbcd)
    {
        LRESULT Ret = CDRF_DODEFAULT;
        PTASK_GROUP TaskGroup;
        PTASK_ITEM TaskItem;

        TaskItem = FindTaskItemByIndex((INT) nmtbcd->nmcd.dwItemSpec);
        TaskGroup = FindTaskGroupByIndex((INT) nmtbcd->nmcd.dwItemSpec);
        if (TaskGroup == NULL && TaskItem != NULL)
        {
            ASSERT(TaskItem != NULL);

            if (TaskItem != NULL && ::IsWindow(TaskItem->hWnd))
            {
                /* Make the entire button flashing if neccessary */
                if (nmtbcd->nmcd.uItemState & CDIS_MARKED)
                {
                    Ret = TBCDRF_NOBACKGROUND;
                    if (!m_Theme)
                    {
                        SelectObject(nmtbcd->nmcd.hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                        Rectangle(nmtbcd->nmcd.hdc,
                            nmtbcd->nmcd.rc.left,
                            nmtbcd->nmcd.rc.top,
                            nmtbcd->nmcd.rc.right,
                            nmtbcd->nmcd.rc.bottom);
                    }
                    else
                    {
                        DrawThemeBackground(m_Theme, nmtbcd->nmcd.hdc, TDP_FLASHBUTTON, 0, &nmtbcd->nmcd.rc, 0);
                    }
                    nmtbcd->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
                    return Ret;
                }
            }
        }
        else if (TaskGroup != NULL)
        {
            /* FIXME: Implement painting for task groups */
        }
        return Ret;
    }

    LRESULT HandleToolbarNotification(IN const NMHDR *nmh)
    {
        LRESULT Ret = 0;

        switch (nmh->code)
        {
        case NM_CUSTOMDRAW:
        {
            LPNMTBCUSTOMDRAW nmtbcd = (LPNMTBCUSTOMDRAW) nmh;

            switch (nmtbcd->nmcd.dwDrawStage)
            {

            case CDDS_ITEMPREPAINT:
                Ret = HandleItemPaint(nmtbcd);
                break;

            case CDDS_PREPAINT:
                Ret = CDRF_NOTIFYITEMDRAW;
                break;

            default:
                Ret = CDRF_DODEFAULT;
                break;
            }
            break;
        }
        }

        return Ret;
    }

    LRESULT DrawBackground(HDC hdc)
    {
        RECT rect;

        GetClientRect(&rect);
        DrawThemeParentBackground(m_hWnd, hdc, &rect);

        return TRUE;
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!IsAppThemed())
        {
            bHandled = FALSE;
            return 0;
        }

        return DrawBackground(hdc);
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SIZE szClient;

        szClient.cx = LOWORD(lParam);
        szClient.cy = HIWORD(lParam);
        if (m_TaskBar.m_hWnd != NULL)
        {
            m_TaskBar.SetWindowPos(NULL, 0, 0, szClient.cx, szClient.cy, SWP_NOZORDER);

            UpdateButtonsSize(FALSE);
        }
        return TRUE;
    }

    LRESULT OnNcHitTestToolbar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt;

        /* See if the mouse is on a button */
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(&pt);

        INT index = m_TaskBar.HitTest(&pt);
        if (index < 0)
        {
            /* Make the control appear to be transparent outside of any buttons */
            return HTTRANSPARENT;
        }

        bHandled = FALSE;
        return 0;
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = TRUE;
        /* We want the tray window to be draggable everywhere, so make the control
        appear transparent */
        Ret = DefWindowProc(uMsg, wParam, lParam);
        if (Ret != HTVSCROLL && Ret != HTHSCROLL)
            Ret = HTTRANSPARENT;
        return Ret;
    }

    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = TRUE;
        if (lParam != 0 && (HWND) lParam == m_TaskBar.m_hWnd)
        {
            HandleButtonClick(LOWORD(wParam));
        }
        return Ret;
    }

    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = TRUE;
        const NMHDR *nmh = (const NMHDR *) lParam;

        if (nmh->hwndFrom == m_TaskBar.m_hWnd)
        {
            Ret = HandleToolbarNotification(nmh);
        }
        return Ret;
    }

    LRESULT OnEnableGrouping(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = m_IsGroupingEnabled;
        if ((BOOL)wParam != m_IsGroupingEnabled)
        {
            EnableGrouping((BOOL) wParam);
        }
        return Ret;
    }

    LRESULT OnUpdateTaskbarPos(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        /* Update the button spacing */
        m_TaskBar.UpdateTbButtonSpacing(m_Tray->IsHorizontal(), m_Theme != NULL);
        return TRUE;
    }

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        LRESULT Ret = 0;
        INT_PTR iBtn = -1;

        if (m_TaskBar.m_hWnd != NULL)
        {
            POINT pt;

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            ::ScreenToClient(m_TaskBar.m_hWnd, &pt);

            iBtn = m_TaskBar.HitTest(&pt);
            if (iBtn >= 0)
            {
                HandleButtonRightClick(iBtn);
            }
        }
        if (iBtn < 0)
        {
            /* Not on a taskbar button, so forward message to tray */
            Ret = SendMessage(m_Tray->GetHWND(), uMsg, wParam, lParam);
        }
        return Ret;
    }

    LRESULT OnKludgeItemRect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PTASK_ITEM TaskItem = FindTaskItem((HWND) wParam);
        if (TaskItem)
        {
            RECT* prcMinRect = (RECT*) lParam;
            RECT rcItem, rcToolbar;
            m_TaskBar.GetItemRect(TaskItem->Index, &rcItem);
            GetWindowRect(m_TaskBar.m_hWnd, &rcToolbar);

            OffsetRect(&rcItem, rcToolbar.left, rcToolbar.top);

            *prcMinRect = rcItem;
            return TRUE;
        }
        return FALSE;
    }

    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
#if DUMP_TASKS != 0
        switch (wParam)
        {
        case 1:
            DumpTasks();
            break;
        }
#endif
        return TRUE;
    }

    DECLARE_WND_CLASS_EX(szTaskSwitchWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTaskSwitchWnd)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(TSWM_ENABLEGROUPING, OnEnableGrouping)
        MESSAGE_HANDLER(TSWM_UPDATETASKBARPOS, OnUpdateTaskbarPos)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(m_ShellHookMsg, HandleShellHookMsg)
    ALT_MSG_MAP(1)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTestToolbar)
    END_MSG_MAP()

    HWND _Init(IN HWND hWndParent, IN OUT ITrayWindow *tray)
    {
        m_Tray = tray;
        m_IsGroupingEnabled = TRUE; /* FIXME */
        return Create(hWndParent, 0, szRunningApps, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP);
    }
};

HWND
CreateTaskSwitchWnd(IN HWND hWndParent, IN OUT ITrayWindow *Tray)
{
    CTaskSwitchWnd * instance;

    // TODO: Destroy after the window is destroyed
    instance = new CTaskSwitchWnd();

    return instance->_Init(hWndParent, Tray);
}
