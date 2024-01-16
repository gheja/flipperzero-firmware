#include "nfc_supported_card_plugin.h"

#include <flipper_application/flipper_application.h>

#include <nfc/nfc_device.h>
#include <nfc/helpers/nfc_util.h>
#include <nfc/protocols/emv/emv.h>
#include "../../helpers/protocol_support/emv/emv_render.h"

#define TAG "EmvSCC"

bool emv_verify(Nfc* nfc) {
    furi_assert(nfc);

    FURI_LOG_D(TAG, "verify()");

    return false;
}

static bool emv_read(Nfc* nfc, NfcDevice* instance) {
    furi_assert(nfc);
    furi_assert(instance);

    FURI_LOG_D(TAG, "read()");

    return true;
}

// this function sets the string displayed on the screen after reading
static bool emv_parse(const NfcDevice* instance, FuriString* parsed_data) {
    furi_assert(instance);
    furi_assert(parsed_data);

    FURI_LOG_D(TAG, "parse()");

    EmvData* data = emv_alloc();
    nfc_device_copy_data(instance, NfcProtocolEmv, data);

    bool parsed = false;

    do {
        nfc_render_emv_info_screen(data, parsed_data);
        parsed = true;

    } while(false);

    emv_free(data);

    return parsed;
}

/* Actual implementation of app<>plugin interface */
static const NfcSupportedCardsPlugin emv_plugin = {
    .protocol = NfcProtocolEmv,
    .verify = emv_verify,
    .read = emv_read,
    .parse = emv_parse,
};

/* Plugin descriptor to comply with basic plugin specification */
static const FlipperAppPluginDescriptor emv_plugin_descriptor = {
    .appid = NFC_SUPPORTED_CARD_PLUGIN_APP_ID,
    .ep_api_version = NFC_SUPPORTED_CARD_PLUGIN_API_VERSION,
    .entry_point = &emv_plugin,
};

/* Plugin entry point - must return a pointer to const descriptor  */
const FlipperAppPluginDescriptor* emv_plugin_ep() {
    FURI_LOG_D(TAG, "emv_plugin_ep()");

    return &emv_plugin_descriptor;
}
