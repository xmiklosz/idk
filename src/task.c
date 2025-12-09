#include "task.h"
#include "ram.h"
#include <string.h>

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
    const tRam *ram = get_ram_state();
    if (!ram) return -1;  // Check RAM is initialized

    uint16_t frame_id;
    int err = falloc(&frame_id, 1);
    if (err != 0) return -1;

    g_taskMgr = (tTaskMgr *)((uint8_t *)ram + frame_id * ram->page_size);

    // Initialize manager
    for (int i = 0; i < TASK_TABLE_SIZE; i++) {
        g_taskMgr->tasks[i].pid = -1;
        g_taskMgr->tasks[i].max_frames = 0;
        g_taskMgr->tasks[i].address_space = NULL;
        memset(g_taskMgr->tasks[i].page_table, 0, sizeof(g_taskMgr->tasks[i].page_table));
    }

    g_nextPID = 1;
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

    // Free the frame where task manager is stored
    uintptr_t mgr_addr = (uintptr_t)g_taskMgr;
    uintptr_t ram_base = (uintptr_t)get_ram_state();
    uint16_t page_size = get_ram_state()->page_size;

    uint16_t frame_id = (mgr_addr - ram_base) / page_size;

    ffree(frame_id, 1);

    g_taskMgr = NULL;
}

// -----------------------------------------------------------------------------
// Create task
// -----------------------------------------------------------------------------
int create_task(const tPageTableEntry *page_table, uint8_t max_frames, void *address_space) {
    if (!g_taskMgr) return -3;
    if (!page_table || !address_space) return -2;

    int slot = find_free_slot();
    if (slot < 0) return -1;

    tTaskStruct *task = &g_taskMgr->tasks[slot];

    // Assign PID
    task->pid = g_nextPID++;
    task->max_frames = max_frames;
    task->address_space = address_space;

    // Copy page table
    memcpy(task->page_table, page_table, sizeof(task->page_table));

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
    if (!g_taskMgr) return NULL;

    for (int i = 0; i < TASK_TABLE_SIZE; i++) {
        if (g_taskMgr->tasks[i].pid == pid)
            return &g_taskMgr->tasks[i];
    }
    return NULL;
}
