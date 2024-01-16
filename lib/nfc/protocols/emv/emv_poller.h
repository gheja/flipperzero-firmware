#pragma once

#include "emv.h"

#include <lib/nfc/protocols/iso14443_4a/iso14443_4a_poller.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EmvPoller opaque type definition.
 */
typedef struct EmvPoller EmvPoller;

/**
 * @brief Enumeration of possible Emv poller event types.
 */
typedef enum {
    EmvPollerEventTypeReadSuccess, /**< Card was read successfully. */
    EmvPollerEventTypeReadFailed, /**< Poller failed to read card. */
} EmvPollerEventType;

/**
 * @brief Emv poller event data.
 */
typedef union {
    EmvError error; /**< Error code indicating card reading fail reason. */
} EmvPollerEventData;

/**
 * @brief Emv poller event structure.
 *
 * Upon emission of an event, an instance of this struct will be passed to the callback.
 */
typedef struct {
    EmvPollerEventType type; /**< Type of emmitted event. */
    EmvPollerEventData* data; /**< Pointer to event specific data. */
} EmvPollerEvent;

#ifdef __cplusplus
}
#endif
