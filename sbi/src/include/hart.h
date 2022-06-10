#pragma once
#include <stdbool.h>

#define HART_DEFAULT_CTXSWT   100000

//Defines the different states a hart can be in
typedef enum HartStatus {
    HS_INVALID, // A HART that doesn't exist
    HS_STOPPED,
    HS_STARTING,
    HS_STARTED,
    HS_STOPPING
} HartStatus;

//Defines the types of mode our harts can be in
typedef enum HartPrivMode {
    HPM_USER,
    HPM_SUPERVISOR,
    HPM_HYPERVISOR, //unused
    HPM_MACHINE
} HartPrivMode;

//defines an entry in the hart table
typedef struct HartData {
    HartStatus status;
    HartPrivMode priv_mode;
    unsigned long target_address;
    unsigned long scratch;
} HartData;


extern HartData sbi_hart_data[8];

HartStatus hart_get_status(unsigned int hart);
bool hart_start(unsigned int hart, unsigned long target, unsigned long scratch);
bool hart_stop(unsigned int hart);
void hart_handle_msip(unsigned int hart);
