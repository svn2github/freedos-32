/* FD32 LDT Management Services
 * by Salvo Isaja & Luca Abeni
 *
 * This header is intented to implement the FD32 Flat Programming Interface
 */
 
#include<ll/i386/hw-data.h> /* for basic data types WORD, DWORD */


/* Not yet implemented routines:
 *   DPMI 0.9 service 000Dh - Allocate Specific LDT Descriptor
 */


/* Error codes for Descriptor Management Routines
 * I've set them the same as the DPMI error codes
 */
#define NO_ERROR                      0
#define ERROR_DESCRIPTOR_UNAVAILABLE  0xFFFF8011
#define ERROR_INVALID_SELECTOR        0xFFFF8022


/* Implementation of DPMI 0.9 service 0000h "Allocate LDT Descriptors" */
int fd32_allocate_descriptors(WORD NumSelectors);

/* Implementation of DPMI 0.9 service 0001h "Free LDT Descriptor" */
int fd32_free_descriptor(WORD Selector);

/* Implementation of DPMI 0.9 service 0002h "Segment to Descriptor" */
int fd32_segment_to_descriptor(WORD RealModeSegment);

/* Implementation of DPMI 0.9 service 0003h "Get Selector Increment Value" */
WORD fd32_get_selector_increment_value();

/* Implementation of DPMI 0.9 service 0006h "Get Segment Base Address" */
int fd32_get_segment_base_address(WORD Selector, DWORD *BaseAddress);

/* Implementation of DPMI 0.9 service 0007h "Set Segment Base Address" */
int fd32_set_segment_base_address(WORD Selector, DWORD BaseAddress);

/* Implementation of DPMI 0.9 service 0008h "Set Segment Limit" */
int fd32_set_segment_limit(WORD Selector, DWORD Limit);

/* Implementation of DPMI 0.9 service 0009h "Set Descriptor Access Rights" */
int fd32_set_descriptor_access_rights(WORD Selector, WORD Rights);

/* Implementation of DPMI 0.9 service 000Ah "Create Alias Descriptor" */
int fd32_create_alias_descriptor(WORD Selector);

/* Implementation of DPMI 0.9 service 000Bh "Get Descriptor" */
int fd32_get_descriptor(WORD Selector, WORD BufferSelector, DWORD BufferOffset);

/* Implementation of DPMI 0.9 service 000Ch "Set Descriptor" */
int fd32_set_descriptor(WORD Selector, WORD BufferSelector, DWORD BufferOffset);

/* Implementation of DPMI 1.0 service 000Eh "Get Multiple Descriptors" */
int fd32_get_multiple_descriptors(WORD Descriptors, WORD BufferSelector, DWORD BufferOffset);

/* Implementation of DPMI 1.0 service 000Fh "Set Multiple Descriptors" */
int fd32_set_multiple_descriptors(WORD Descriptors, WORD BufferSelector, DWORD BufferOffset);

