#include "mmu.h"
#include "ram.h"
#include <stddef.h>
#include <stdio.h>

// Static pointer to the current active page table
static tPageTableEntry *g_current_page_table = NULL;

// Sets a new active page table for the MMU
void set_page_table(tPageTableEntry *page_table) {
    g_current_page_table = page_table;
    printf("[MMU DEBUG] set_page_table: page_table=%p\n", (void*)page_table);
}

// Calculates the physical address corresponding to a virtual address
int get_physical_address(uint16_t virtual_address, uint16_t *physical_address) {
    printf("[MMU DEBUG] get_physical_address: virt_addr=%u\n", virtual_address);

    // Check for null pointer
    if (!physical_address) {
        printf("[MMU DEBUG] get_physical_address: physical_address is nullptr, returning -3\n");
        return -3;  // physical_address is nullptr
    }

    // Check if page table is set
    if (!g_current_page_table) {
        printf("[MMU DEBUG] get_physical_address: No page table present, returning -4\n");
        return -4;  // No page table present
    }

    // Get RAM state
    const tRam *ram = get_ram_state();
    if (!ram) {
        printf("[MMU DEBUG] get_physical_address: RAM not initialized, returning -5\n");
        return -5;  // RAM not initialized
    }

    uint16_t page_size = ram->page_size;
    printf("[MMU DEBUG] get_physical_address: page_size=%u\n", page_size);

    // Calculate page index and offset within page
    uint16_t page_index = virtual_address / page_size;
    uint16_t page_offset = virtual_address % page_size;
    printf("[MMU DEBUG] get_physical_address: page_index=%u, page_offset=%u\n", page_index, page_offset);

    tPageTableEntry *pte = &g_current_page_table[page_index];
    printf("[MMU DEBUG] get_physical_address: pte r=%u w=%u x=%u p_bit=%u r_bit=%u m_bit=%u frame_id=%u\n",
           pte->r, pte->w, pte->x, pte->p_bit, pte->r_bit, pte->m_bit, pte->frame_id);

    // Check if page is accessible (at least one of r/w/x must be set)
    if (pte->r == 0 && pte->w == 0 && pte->x == 0) {
        printf("[MMU DEBUG] get_physical_address: Page not accessible (r=w=x=0), returning -2\n");
        return -2;  // Segmentation fault (page not available)
    }

    // Check if page is present in RAM
    if (!pte->p_bit) {
        printf("[MMU DEBUG] get_physical_address: Page not present (p_bit=0), returning -1\n");
        return -1;  // Page fault
    }

    // Calculate physical address
    *physical_address = pte->frame_id * page_size + page_offset;
    printf("[MMU DEBUG] get_physical_address: Success! phys_addr=%u (frame_id=%u * page_size=%u + offset=%u)\n",
           *physical_address, pte->frame_id, page_size, page_offset);

    return 0;  // Success
}

// Loads RAM content at the calculated physical address into data (for instruction fetch)
int fetch_instruction(uint16_t virtual_address, uint8_t *data) {
    printf("[MMU DEBUG] fetch_instruction: virt_addr=%u\n", virtual_address);

    // Check if page table is set
    if (!g_current_page_table) {
        printf("[MMU DEBUG] fetch_instruction: No page table present, returning -4\n");
        return -4;  // No page table present
    }

    // Get RAM state
    const tRam *ram = get_ram_state();
    if (!ram) {
        printf("[MMU DEBUG] fetch_instruction: RAM not initialized, returning -5\n");
        return -5;  // RAM not initialized
    }

    uint16_t page_size = ram->page_size;
    uint16_t page_index = virtual_address / page_size;
    printf("[MMU DEBUG] fetch_instruction: page_size=%u, page_index=%u\n", page_size, page_index);

    tPageTableEntry *pte = &g_current_page_table[page_index];
    printf("[MMU DEBUG] fetch_instruction: pte r=%u w=%u x=%u p_bit=%u frame_id=%u\n",
           pte->r, pte->w, pte->x, pte->p_bit, pte->frame_id);

    // Check if page is accessible
    if (pte->r == 0 && pte->w == 0 && pte->x == 0) {
        printf("[MMU DEBUG] fetch_instruction: Page not accessible (r=w=x=0), returning -2\n");
        return -2;  // Segmentation fault
    }

    // Check execute permission
    if (!pte->x) {
        printf("[MMU DEBUG] fetch_instruction: No execute permission (x=0), returning -3\n");
        return -3;  // Access violation (no execute permission)
    }

    // Check if page is present
    if (!pte->p_bit) {
        printf("[MMU DEBUG] fetch_instruction: Page not present (p_bit=0), returning -1\n");
        return -1;  // Page fault
    }

    // Get physical address
    uint16_t physical_address;
    int result = get_physical_address(virtual_address, &physical_address);
    if (result != 0) {
        printf("[MMU DEBUG] fetch_instruction: get_physical_address failed with %d\n", result);
        return result;
    }

    // Read data from RAM
    uint8_t *ram_base = (uint8_t *)ram;
    *data = ram_base[physical_address];
    printf("[MMU DEBUG] fetch_instruction: Read data=0x%02x from phys_addr=%u\n", *data, physical_address);

    // Set referenced bit
    pte->r_bit = 1;
    printf("[MMU DEBUG] fetch_instruction: Set r_bit=1, returning 0\n");

    return 0;  // Success
}

// Loads RAM content at the calculated physical address into data
int load_data(uint16_t virtual_address, uint8_t *data) {
    printf("[MMU DEBUG] load_data: virt_addr=%u\n", virtual_address);

    // Check if page table is set
    if (!g_current_page_table) {
        printf("[MMU DEBUG] load_data: No page table present, returning -4\n");
        return -4;  // No page table present
    }

    // Get RAM state
    const tRam *ram = get_ram_state();
    if (!ram) {
        printf("[MMU DEBUG] load_data: RAM not initialized, returning -5\n");
        return -5;  // RAM not initialized
    }

    uint16_t page_size = ram->page_size;
    uint16_t page_index = virtual_address / page_size;
    printf("[MMU DEBUG] load_data: page_size=%u, page_index=%u\n", page_size, page_index);

    tPageTableEntry *pte = &g_current_page_table[page_index];
    printf("[MMU DEBUG] load_data: pte r=%u w=%u x=%u p_bit=%u frame_id=%u\n",
           pte->r, pte->w, pte->x, pte->p_bit, pte->frame_id);

    // Check if page is accessible
    if (pte->r == 0 && pte->w == 0 && pte->x == 0) {
        printf("[MMU DEBUG] load_data: Page not accessible (r=w=x=0), returning -2\n");
        return -2;  // Segmentation fault
    }

    // Check read permission
    if (!pte->r) {
        printf("[MMU DEBUG] load_data: No read permission (r=0), returning -3\n");
        return -3;  // Access violation (no read permission)
    }

    // Check if page is present
    if (!pte->p_bit) {
        printf("[MMU DEBUG] load_data: Page not present (p_bit=0), returning -1\n");
        return -1;  // Page fault
    }

    // Get physical address
    uint16_t physical_address;
    int result = get_physical_address(virtual_address, &physical_address);
    if (result != 0) {
        printf("[MMU DEBUG] load_data: get_physical_address failed with %d\n", result);
        return result;
    }

    // Read data from RAM
    uint8_t *ram_base = (uint8_t *)ram;
    *data = ram_base[physical_address];
    printf("[MMU DEBUG] load_data: Read data=0x%02x from phys_addr=%u\n", *data, physical_address);

    // Set referenced bit
    pte->r_bit = 1;
    printf("[MMU DEBUG] load_data: Set r_bit=1, returning 0\n");

    return 0;  // Success
}

// Stores data to RAM at the calculated physical address
int store_data(uint16_t virtual_address, uint8_t data) {
    printf("[MMU DEBUG] store_data: virt_addr=%u, data=0x%02x\n", virtual_address, data);

    // Check if page table is set
    if (!g_current_page_table) {
        printf("[MMU DEBUG] store_data: No page table present, returning -4\n");
        return -4;  // No page table present
    }

    // Get RAM state
    const tRam *ram = get_ram_state();
    if (!ram) {
        printf("[MMU DEBUG] store_data: RAM not initialized, returning -5\n");
        return -5;  // RAM not initialized
    }

    uint16_t page_size = ram->page_size;
    uint16_t page_index = virtual_address / page_size;
    printf("[MMU DEBUG] store_data: page_size=%u, page_index=%u\n", page_size, page_index);

    tPageTableEntry *pte = &g_current_page_table[page_index];
    printf("[MMU DEBUG] store_data: pte r=%u w=%u x=%u p_bit=%u frame_id=%u\n",
           pte->r, pte->w, pte->x, pte->p_bit, pte->frame_id);

    // Check if page is accessible
    if (pte->r == 0 && pte->w == 0 && pte->x == 0) {
        printf("[MMU DEBUG] store_data: Page not accessible (r=w=x=0), returning -2\n");
        return -2;  // Segmentation fault
    }

    // Check write permission
    if (!pte->w) {
        printf("[MMU DEBUG] store_data: No write permission (w=0), returning -3\n");
        return -3;  // Access violation (no write permission)
    }

    // Check if page is present
    if (!pte->p_bit) {
        printf("[MMU DEBUG] store_data: Page not present (p_bit=0), returning -1\n");
        return -1;  // Page fault
    }

    // Get physical address
    uint16_t physical_address;
    int result = get_physical_address(virtual_address, &physical_address);
    if (result != 0) {
        printf("[MMU DEBUG] store_data: get_physical_address failed with %d\n", result);
        return result;
    }

    // Write data to RAM
    uint8_t *ram_base = (uint8_t *)ram;
    ram_base[physical_address] = data;
    printf("[MMU DEBUG] store_data: Wrote data=0x%02x to phys_addr=%u\n", data, physical_address);

    // Set referenced and modified bits
    pte->r_bit = 1;
    pte->m_bit = 1;
    printf("[MMU DEBUG] store_data: Set r_bit=1, m_bit=1, returning 0\n");

    return 0;  // Success
}
