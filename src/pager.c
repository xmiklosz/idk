#include "pager.h"
#include "task.h"
#include "ram.h"
#include <stdint.h>
#include <string.h>

// Helper: compute NRU class (0..3) from r_bit and m_bit
static int nru_class(const tPageTableEntry *e) {
    // class encoding: (r,m) -> value
    // (0,0) -> 0
    // (0,1) -> 1
    // (1,0) -> 2
    // (1,1) -> 3
    return (e->r_bit << 1) | (e->m_bit);
}

int page_fault(int pid, uint16_t virtual_address) {
    printf("[PAGER DEBUG] page_fault: pid=%d, virt_addr=%u\n", pid, virtual_address);

    // 1) find task
    tTaskStruct *task = get_task_struct(pid);
    if (!task) {
        printf("[PAGER DEBUG] page_fault: Task not found, returning -1\n");
        return -1; // Task not found
    }

    const tRam *ram = get_ram_state();
    if (!ram) {
        printf("[PAGER DEBUG] page_fault: RAM not initialized, returning -1\n");
        return -1; // RAM not initialized (treat as out of resources)
    }

    uint16_t page_size = ram->page_size;

    // 2) compute page index
    uint32_t page_index = virtual_address / page_size;
    if (page_index >= PAGE_TABLE_SIZE) {
        return -4; // segmentation fault (address out of task's virtual space)
    }

    tPageTableEntry *pte = &task->page_table[page_index];

    // 3) check r/w/x bits: if all zero -> segmentation fault
    if (pte->r == 0 && pte->w == 0 && pte->x == 0) {
        return -4;
    }

    // 4) if already present -> nothing to do
    if (pte->p_bit) {
        return -2;
    }

    // 5) count current frames present for this task
    int present_count = 0;
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        if (task->page_table[i].p_bit) present_count++;
    }

    // 6) decide whether we need to evict (respect max_frames)
    int need_evict = 0;
    if (task->max_frames != 0 && present_count >= task->max_frames) {
        need_evict = 1;
    }

    int victim_idx = -1;
    int victim_frame = -1;

    // If eviction required, choose victim using NRU (local scope)
    if (need_evict) {
        int best_class = 4; // larger than any NRU class
        for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
            if (!task->page_table[i].p_bit) continue; // not in RAM
            // We may evict any present page (including possibly page_index if present, but page_index is not present)
            int cls = nru_class(&task->page_table[i]);
            if (cls < best_class) {
                best_class = cls;
                victim_idx = i;
                if (best_class == 0) break; // can't get better than class 0
            }
        }
        // Should always find a victim because present_count >= max_frames
        if (victim_idx == -1) {
            // Nothing to evict (shouldn't happen) -> out of resources
            return -3;
        }
        victim_frame = task->page_table[victim_idx].frame_id;
    }

    // 7) Write back all modified pages of the task to its address_space
    // Per spec: "During page_fault execution, all modified pages of the task are first written to the task's address space."
    // Also, afterwards we clear r_bit and m_bit for all task pages (per spec).
    uint8_t *ram_base = (uint8_t *)ram;
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        tPageTableEntry *e = &task->page_table[i];
        if (e->p_bit && e->m_bit) {
            // write frame content back to task's address_space
            uint8_t *frame_ptr = ram_base + (size_t)e->frame_id * page_size;
            uint8_t *dst = (uint8_t *)task->address_space + (size_t)i * page_size;
            memcpy(dst, frame_ptr, page_size);
            // leave m_bit for now; will be cleared below
        }
    }

    // 8) Clear r_bit and m_bit of all pages of the task (per specification)
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        task->page_table[i].r_bit = 0;
        task->page_table[i].m_bit = 0;
    }

    // 9) If eviction required, free victim frame (we freed after writebacks and clearing bits)
    if (need_evict) {
        // Mark victim entry as not present before freeing
        task->page_table[victim_idx].p_bit = 0;
        task->page_table[victim_idx].frame_id = 0;
        // free the frame in RAM
        ffree((uint16_t)victim_frame, 1);
        present_count--; // Update count after eviction
    }

    // 10) allocate a frame for the missing page
    uint16_t new_frame;
    int alloc_res = falloc(&new_frame, 1);

    // If allocation failed and we haven't evicted yet, try evicting due to RAM being full
    if (alloc_res != 0 && !need_evict && present_count > 0) {
        printf("[PAGER DEBUG] page_fault: Allocation failed, RAM full. Attempting to evict from %d present pages\n", present_count);

        // Find victim using NRU (same logic as before)
        int best_class = 4;
        victim_idx = -1;
        for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
            if (!task->page_table[i].p_bit) continue; // not in RAM
            int cls = nru_class(&task->page_table[i]);
            if (cls < best_class) {
                best_class = cls;
                victim_idx = i;
                if (best_class == 0) break; // can't get better than class 0
            }
        }

        if (victim_idx != -1) {
            printf("[PAGER DEBUG] page_fault: Evicting page %d (NRU class %d) to free frame %d\n",
                   victim_idx, best_class, task->page_table[victim_idx].frame_id);

            int victim_frame_id = task->page_table[victim_idx].frame_id;
            task->page_table[victim_idx].p_bit = 0;
            task->page_table[victim_idx].frame_id = 0;
            ffree((uint16_t)victim_frame_id, 1);

            // Try allocating again
            alloc_res = falloc(&new_frame, 1);
            printf("[PAGER DEBUG] page_fault: After eviction, falloc returned %d\n", alloc_res);
        } else {
            printf("[PAGER DEBUG] page_fault: No victim found to evict\n");
        }
    }

    if (alloc_res != 0) {
        printf("[PAGER DEBUG] page_fault: Failed to allocate frame, returning -3\n");
        return -3; // out of resources
    }

    // 11) Load page content from task's address_space into the allocated frame
    uint8_t *frame_ptr = ram_base + (size_t)new_frame * page_size;
    uint8_t *src = (uint8_t *)task->address_space + (size_t)page_index * page_size;
    memcpy(frame_ptr, src, page_size);

    // 12) Update page table entry for the loaded page
    pte->p_bit = 1;
    pte->frame_id = new_frame;
    pte->r_bit = 0; // cleared per spec
    pte->m_bit = 0; // cleared per spec

    return 0;
}
