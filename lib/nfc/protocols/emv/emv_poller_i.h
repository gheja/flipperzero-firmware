#pragma once

#include "emv_poller.h"

#include <lib/nfc/protocols/iso14443_4a/iso14443_4a_poller_i.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EmvPollerStateIdle,
    EmvPollerStateSelectPpse,
    EmvPollerStateStartApplication,
    EmvPollerStateGetProcessingOptions,
    EmvPollerStateReadFailed,
    EmvPollerStateReadSuccess,

    EmvPollerStateNum,
} EmvPollerState;

typedef enum {
    EmvPollerSessionStateIdle,
    EmvPollerSessionStateActive,
    EmvPollerSessionStateStopRequest,
} EmvPollerSessionState;

struct EmvPoller {
    Iso14443_4aPoller* iso14443_4a_poller;
    EmvPollerSessionState session_state;
    EmvPollerState state;
    EmvError error;
    EmvData* data;
    BitBuffer* tx_buffer;
    BitBuffer* rx_buffer;
    BitBuffer* input_buffer;
    BitBuffer* result_buffer;
    EmvPollerEventData emv_event_data;
    EmvPollerEvent emv_event;
    NfcGenericEvent general_event;
    NfcGenericCallback callback;
    EmvApplicationCard* app;
    void* context;
};

EmvError emv_process_error(Iso14443_4aError error);

const EmvData* emv_poller_get_data(EmvPoller* instance);

bool emv_check_select_ppse_response(const BitBuffer* buf);

EmvError emv_poller_select_ppse(EmvPoller* instance);

EmvError emv_poller_start_application(EmvPoller* instance);

EmvError emv_poller_get_processing_options(EmvPoller* instance);

#ifdef __cplusplus
}
#endif