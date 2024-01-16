#include "emv.h"

#include <furi.h>

#define EMV_PROTOCOL_NAME "EMV Chip Card"

const NfcDeviceBase nfc_device_emv = {
    .protocol_name = EMV_PROTOCOL_NAME,
    .alloc = (NfcDeviceAlloc)emv_alloc,
    .free = (NfcDeviceFree)emv_free,
    .reset = (NfcDeviceReset)emv_reset,
    .copy = (NfcDeviceCopy)emv_copy,
    .get_name = (NfcDeviceGetName)emv_get_device_name,
    .get_uid = (NfcDeviceGetUid)emv_get_uid,
    .set_uid = (NfcDeviceSetUid)emv_set_uid,
    .get_base_data = (NfcDeviceGetBaseData)emv_get_base_data,
};

EmvApplicationCard* emv_app_alloc() {
    EmvApplicationCard* app = malloc(sizeof(EmvApplicationCard));
    app->name = furi_string_alloc();
    app->card_number = furi_string_alloc();
    emv_app_reset(app);
    return app;
}

void emv_app_free(EmvApplicationCard* app) {
    furi_assert(app);
    furi_assert(app->name);
    furi_assert(app->card_number);

    furi_string_free(app->name);
    furi_string_free(app->card_number);
    free(app);
}

void emv_app_reset(EmvApplicationCard* app) {
    furi_assert(app);
    furi_assert(app->name);
    furi_assert(app->card_number);

    app->priority = 0;

    memset(app->aid.data, 0x00, sizeof(uint8_t) * EVM_MAX_AID_SIZE); // TODO: is this ok?
    app->aid.size = 0;

    furi_string_reset(app->name);
    furi_string_reset(app->card_number);
    app->exp_month = 0;
    app->exp_year = 0;
    app->country_code = 0;
    app->currency_code = 0;

    memset(app->pdol.data, 0x00, sizeof(uint8_t) * EVM_MAX_APDU_LEN); // TODO: is this ok?
    app->pdol.size = 0;

    memset(app->afl.data, 0x00, sizeof(uint8_t) * EVM_MAX_APDU_LEN); // TODO: is this ok?
    app->afl.size = 0;

    app->transaction_log_sfi = 0;
    app->transaction_log_length = 0;
}

void emv_app_copy(EmvApplicationCard* app, const EmvApplicationCard* other) {
    furi_assert(app);
    furi_assert(app->name);
    furi_assert(app->card_number);

    furi_assert(other);
    furi_assert(other->name);
    furi_assert(other->card_number);

    app->priority = other->priority;

    memcpy(app->aid.data, other->aid.data, sizeof(uint8_t) * EVM_MAX_AID_SIZE);
    app->aid.size = other->aid.size;

    furi_string_set(app->name, other->name);
    furi_string_set(app->card_number, other->card_number);
    app->exp_month = other->exp_month;
    app->exp_year = other->exp_year;
    app->country_code = other->country_code;
    app->currency_code = other->currency_code;

    memcpy(app->pdol.data, other->pdol.data, sizeof(uint8_t) * EVM_MAX_APDU_LEN);
    app->pdol.size = other->pdol.size;

    memcpy(app->afl.data, other->afl.data, sizeof(uint8_t) * EVM_MAX_APDU_LEN);
    app->afl.size = other->afl.size;

    app->transaction_log_sfi = other->transaction_log_sfi;
    app->transaction_log_length = other->transaction_log_length;
}

EmvData* emv_alloc() {
    EmvData* data = malloc(sizeof(EmvData));
    data->iso14443_4a_data = iso14443_4a_alloc();
    data->app = emv_app_alloc();
    emv_reset(data);

    return data;
}

void emv_free(EmvData* data) {
    furi_assert(data);
    furi_assert(data->iso14443_4a_data);
    furi_assert(data->app);

    emv_reset(data);

    iso14443_4a_free(data->iso14443_4a_data);
    emv_app_free(data->app);
    free(data);
}

void emv_reset(EmvData* data) {
    furi_assert(data);
    furi_assert(data->iso14443_4a_data);
    furi_assert(data->app);

    iso14443_4a_reset(data->iso14443_4a_data);
    emv_app_reset(data->app);
}

// this function is called by the nfc_device_copy_data()
void emv_copy(EmvData* data, const EmvData* other) {
    furi_assert(data);
    furi_assert(data->iso14443_4a_data);
    furi_assert(data->app);

    furi_assert(other);
    furi_assert(other->iso14443_4a_data);
    furi_assert(other->app);

    emv_reset(data);

    iso14443_4a_copy(data->iso14443_4a_data, other->iso14443_4a_data);
    emv_app_copy(data->app, other->app);
}

const char* emv_get_device_name(const EmvData* data, NfcDeviceNameType name_type) {
    UNUSED(data);
    UNUSED(name_type);
    return EMV_PROTOCOL_NAME;
}

const uint8_t* emv_get_uid(const EmvData* data, size_t* uid_len) {
    furi_assert(data);

    return iso14443_4a_get_uid(data->iso14443_4a_data, uid_len);
}

bool emv_set_uid(EmvData* data, const uint8_t* uid, size_t uid_len) {
    furi_assert(data);

    return iso14443_4a_set_uid(data->iso14443_4a_data, uid, uid_len);
}

Iso14443_4aData* emv_get_base_data(const EmvData* data) {
    furi_assert(data);

    return data->iso14443_4a_data;
}
