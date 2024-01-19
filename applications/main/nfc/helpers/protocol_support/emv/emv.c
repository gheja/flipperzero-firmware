#include "emv.h"
#include "emv_render.h"

#include <nfc/protocols/emv/emv_poller.h>

#include "nfc/nfc_app_i.h"

#include "../nfc_protocol_support_common.h"
#include "../nfc_protocol_support_gui_common.h"
#include "../iso14443_4a/iso14443_4a_i.h"

static NfcCommand nfc_scene_read_poller_callback_emv(NfcGenericEvent event, void* context) {
    furi_assert(event.protocol == NfcProtocolEmv);

    NfcApp* instance = context;
    const EmvPollerEvent* emv_event = event.event_data;

    if(emv_event->type == EmvPollerEventTypeReadSuccess) {
        nfc_device_set_data(
            instance->nfc_device, NfcProtocolEmv, nfc_poller_get_data(instance->poller));
        view_dispatcher_send_custom_event(instance->view_dispatcher, NfcCustomEventPollerSuccess);
        return NfcCommandStop;
    } else if(emv_event->type == EmvPollerEventTypeReadFailed) {
        view_dispatcher_send_custom_event(instance->view_dispatcher, NfcCustomEventPollerFailure);
        return NfcCommandStop;
    }

    return NfcCommandContinue;
}

static void nfc_scene_read_on_enter_emv(NfcApp* instance) {
    nfc_poller_start(instance->poller, nfc_scene_read_poller_callback_emv, instance);
}

static void nfc_scene_read_success_on_enter_emv(NfcApp* instance) {
    FuriString* temp_str = furi_string_alloc();

    EmvData* data = emv_alloc();
    nfc_device_copy_data(instance->nfc_device, NfcProtocolEmv, data);
    nfc_render_emv_info_screen(data, temp_str);
    free(data);

    widget_add_text_scroll_element(
        instance->widget, 0, 0, 128, 52, furi_string_get_cstr(temp_str));

    furi_string_free(temp_str);
}

const NfcProtocolSupportBase nfc_protocol_support_emv = {
    .features = NfcProtocolFeatureNone,

    .scene_info =
        {
            .on_enter = nfc_protocol_support_common_on_enter_empty,
            .on_event = nfc_protocol_support_common_on_event_empty,
        },
    .scene_read =
        {
            .on_enter = nfc_scene_read_on_enter_emv,
            .on_event = nfc_protocol_support_common_on_event_empty,
        },
    .scene_read_menu =
        {
            .on_enter = nfc_protocol_support_common_on_enter_empty,
            .on_event = nfc_protocol_support_common_on_event_empty,
        },
    .scene_read_success =
        {
            .on_enter = nfc_scene_read_success_on_enter_emv,
            .on_event = nfc_protocol_support_common_on_event_empty,
        },
};
