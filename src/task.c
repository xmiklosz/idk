#include "task.h"
#include "ram.h"
#include <string.h>
#include <stdio.h>

// -----------------------------------------------------------------------------
// Internal static storage
// -----------------------------------------------------------------------------
static tTaskMgr *g_taskMgr = NULL;
static int g_nextPID = 1;

// -----------------------------------------------------------------------------
// Helper: find free slot
// -----------------------------------------------------------------------------
static int find_free_slot() {
    if (!g_taskMgr) return -1;
    for (int i = 0; i < TASK_TABLE_SIZE; i++) {
        if (g_taskMgr->tasks[i].pid == -1)
            return i;
    }
    return -1;
}

// -----------------------------------------------------------------------------
// Initialize task manager — allocates RAM frames
// -----------------------------------------------------------------------------
int init_taskMgr() {
    printf("[TASK DEBUG] init_taskMgr: Starting initialization\n");
    const tRam *ram = get_ram_state();
    if (!ram) {
        printf("[TASK DEBUG] init_taskMgr: RAM not initialized, returning -1\n");
        return -1;  // Check RAM is initialized
    }

    // Calculate how many frames we need for tTaskMgr
    size_t mgr_size = sizeof(tTaskMgr);
    uint16_t frames_needed = (mgr_size + ram->page_size - 1) / ram->page_size;
    printf("[TASK DEBUG] init_taskMgr: tTaskMgr size=%zu bytes, page_size=%u, frames_needed=%u\n",
           mgr_size, ram->page_size, frames_needed);

    uint16_t frame_id;
    int err = falloc(&frame_id, frames_needed);
    if (err != 0) {
        printf("[TASK DEBUG] init_taskMgr: falloc failed with %d, returning -1\n", err);
        return -1;
    }

    g_taskMgr = (tTaskMgr *)((uint8_t *)ram + frame_id * ram->page_size);
    printf("[TASK DEBUG] init_taskMgr: Allocated %u frames starting at frame %u, g_taskMgr=%p\n",
           frames_needed, frame_id, (void*)g_taskMgr);

    // Initialize manager
    for (int i = 0; i < TASK_TABLE_SIZE; i++) {
        g_taskMgr->tasks[i].pid = -1;
        g_taskMgr->tasks[i].max_frames = 0;
        g_taskMgr->tasks[i].address_space = NULL;
        memset(g_taskMgr->tasks[i].page_table, 0, sizeof(g_taskMgr->tasks[i].page_table));
    }

    g_nextPID = 1;
    printf("[TASK DEBUG] init_taskMgr: Success, returning 0\n");
    return 0;
}

// -----------------------------------------------------------------------------
// Destroy task manager
// -----------------------------------------------------------------------------
void destroy_taskMgr() {
    if (!g_taskMgr) return;

    const tRam *ram = get_ram_state();
    if (!ram) {
        g_taskMgr = NULL;
        return;  // Can't properly free if RAM is destroyed
    }

    // Destroy all active tasks
    for (int i = 0; i < TASK_TABLE_SIZE; i++) {
        if (g_taskMgr->tasks[i].pid != -1) {
            destroy_task(g_taskMgr->tasks[i].pid);
        }
    }

    // Free all frames where task manager is stored
    uintptr_t mgr_addr = (uintptr_t)g_taskMgr;
    uintptr_t ram_base = (uintptr_t)get_ram_state();
    uint16_t page_size = get_ram_state()->page_size;

    uint16_t frame_id = (mgr_addr - ram_base) / page_size;

    // Calculate how many frames were allocated
    size_t mgr_size = sizeof(tTaskMgr);
    uint16_t frames_needed = (mgr_size + page_size - 1) / page_size;

    ffree(frame_id, frames_needed);

    g_taskMgr = NULL;
}

// -----------------------------------------------------------------------------
// Create task
// -----------------------------------------------------------------------------
int create_task(const tPageTableEntry *page_table, uint8_t max_frames, void *address_space) {
    printf("[TASK DEBUG] create_task: g_taskMgr=%p, max_frames=%u, address_space=%p\n",
           (void*)g_taskMgr, max_frames, address_space);

    if (!g_taskMgr) {
        printf("[TASK DEBUG] create_task: Task manager not initialized, returning -3\n");
        return -3;
    }
    if (!page_table || !address_space) {
        printf("[TASK DEBUG] create_task: Invalid parameters, returning -2\n");
        return -2;
    }

    int slot = find_free_slot();
    if (slot < 0) {
        printf("[TASK DEBUG] create_task: No free slots, returning -1\n");
        return -1;
    }

    tTaskStruct *task = &g_taskMgr->tasks[slot];

    // Assign PID
    task->pid = g_nextPID++;
    task->max_frames = max_frames;
    task->address_space = address_space;

    // Copy page table
    memcpy(task->page_table, page_table, sizeof(task->page_table));

    printf("[TASK DEBUG] create_task: Created task with PID=%d in slot %d\n", task->pid, slot);
    return task->pid;
}

// -----------------------------------------------------------------------------
// Destroy task, free all frames used by page table
// -----------------------------------------------------------------------------
int destroy_task(int pid) {
    if (!g_taskMgr) return -1;

    for (int i = 0; i < TASK_TABLE_SIZE; i++) {
        if (g_taskMgr->tasks[i].pid == pid) {

            // Free frames used by this task
            for (int p = 0; p < PAGE_TABLE_SIZE; p++) {
                tPageTableEntry *entry = &g_taskMgr->tasks[i].page_table[p];

                if (entry->p_bit) {
                    ffree(entry->frame_id, 1);
                }
            }

            // Reset entry
            g_taskMgr->tasks[i].pid = -1;
            g_taskMgr->tasks[i].max_frames = 0;
            g_taskMgr->tasks[i].address_space = NULL;
            memset(g_taskMgr->tasks[i].page_table, 0, sizeof(g_taskMgr->tasks[i].page_table));

            return 0;
        }
    }
    return -1;
}

// -----------------------------------------------------------------------------
// Get pointer to taskMgr
// -----------------------------------------------------------------------------
const tTaskMgr *get_task_mgr() {
    return g_taskMgr;
}

// -----------------------------------------------------------------------------
// Get task by PID
// -----------------------------------------------------------------------------
tTaskStruct *get_task_struct(int pid) {
    printf("[TASK DEBUG] get_task_struct: Looking for PID=%d, g_taskMgr=%p\n", pid, (void*)g_taskMgr);

    if (!g_taskMgr) {
        printf("[TASK DEBUG] get_task_struct: Task manager not initialized, returning NULL\n");
        return NULL;
    }

    for (int i = 0; i < TASK_TABLE_SIZE; i++) {
        if (g_taskMgr->tasks[i].pid == pid) {
            printf("[TASK DEBUG] get_task_struct: Found task PID=%d at slot %d\n", pid, i);
            return &g_taskMgr->tasks[i];
        }
    }

    printf("[TASK DEBUG] get_task_struct: Task PID=%d not found. Active PIDs: ", pid);
    for (int i = 0; i < TASK_TABLE_SIZE; i++) {
        if (g_taskMgr->tasks[i].pid != -1) {
            printf("%d ", g_taskMgr->tasks[i].pid);
        }
    }
    printf("\n");

    return NULL;
}
