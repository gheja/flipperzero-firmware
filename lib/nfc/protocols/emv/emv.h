#pragma once

#include <lib/nfc/protocols/iso14443_4a/iso14443_4a.h>

#include <lib/toolbox/simple_array.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVM_MAX_APDU_LEN 128
#define EVM_MAX_AID_SIZE 32

#define EMV_TAG_APP_TEMPLATE 0x61 //
#define EMV_TAG_AID 0x4F //
#define EMV_TAG_PRIORITY 0x87 //
#define EMV_TAG_PDOL 0x9F38
#define EMV_TAG_FCI_ISSUER 0x50
#define EMV_TAG_FCI 0xBF0C
#define EMV_TAG_LOG_CTRL 0x9F4D
#define EMV_TAG_CARD_NUM 0x57
#define EMV_TAG_PAN 0x5A
#define EMV_TAG_AFL 0x94
#define EMV_TAG_EXP_DATE 0x5F24
#define EMV_TAG_COUNTRY_CODE 0x5F28
#define EMV_TAG_CURRENCY_CODE 0x9F42
#define EMV_TAG_CARDHOLDER_NAME 0x5F20

#define EMV_TAG_DF_NAME 0x84
#define EMV_TAG_TRANSACTION_LOG_INFO 0x9F4D
#define EMV_TAG_TRACK1_DATA 0x56
#define EMV_TAG_TRACK2_DATA 0x9F6B

typedef struct {
    uint16_t tag;
    uint8_t data[];
} PDOLValue;

typedef struct {
    uint8_t size;
    uint8_t data[EVM_MAX_APDU_LEN];
} APDU;

typedef struct {
    uint8_t size;
    uint8_t data[EVM_MAX_AID_SIZE];
} AID;

typedef struct {
    uint8_t priority;
    AID aid;
    FuriString* fci_issuer;
    FuriString* card_number;
    uint8_t exp_month;
    uint8_t exp_year;
    uint16_t country_code;
    uint16_t currency_code;
    APDU pdol;
    APDU afl;
    uint8_t transaction_log_sfi;
    uint8_t transaction_log_length;
    FuriString* extra_text; // this is just so I don't have to do a full rebuild every time
} EmvApplicationCard;

#define EMV_CMD_GET_VERSION (0x60)
#define EMV_CMD_GET_FREE_MEMORY (0x6E)
#define EMV_CMD_GET_KEY_SETTINGS (0x45)
#define EMV_CMD_GET_KEY_VERSION (0x64)
#define EMV_CMD_GET_APPLICATION_IDS (0x6A)
#define EMV_CMD_SELECT_APPLICATION (0x5A)
#define EMV_CMD_GET_FILE_IDS (0x6F)
#define EMV_CMD_GET_FILE_SETTINGS (0xF5)

#define EMV_CMD_READ_DATA (0xBD)
#define EMV_CMD_GET_VALUE (0x6C)
#define EMV_CMD_READ_RECORDS (0xBB)

#define EMV_FLAG_HAS_NEXT (0xAF)

#define EMV_MAX_KEYS (14)
#define EMV_MAX_FILES (32)

#define EMV_UID_SIZE (7)
#define EMV_BATCH_SIZE (5)
#define EMV_APP_ID_SIZE (3)
#define EMV_VALUE_SIZE (4)
typedef struct EmvApplication {
} EmvApplication;

typedef enum {
    EmvErrorNone,
    EmvErrorNotPresent,
    EmvErrorProtocol,
    EmvErrorTimeout,
} EmvError;

typedef struct {
    Iso14443_4aData* iso14443_4a_data;
    EmvApplicationCard* app;
} EmvData;

extern const NfcDeviceBase nfc_device_emv;

//

EmvApplicationCard* emv_app_alloc();

void emv_app_free(EmvApplicationCard* app);

void emv_app_reset(EmvApplicationCard* app);

void emv_app_copy(EmvApplicationCard* app, const EmvApplicationCard* other);

// Virtual methods

EmvData* emv_alloc();

void emv_free(EmvData* data);

void emv_reset(EmvData* data);

void emv_copy(EmvData* data, const EmvData* other);

const char* emv_get_device_name(const EmvData* data, NfcDeviceNameType name_type);

const uint8_t* emv_get_uid(const EmvData* data, size_t* uid_len);

bool emv_set_uid(EmvData* data, const uint8_t* uid, size_t uid_len);

Iso14443_4aData* emv_get_base_data(const EmvData* data);

#ifdef __cplusplus
}
#endif
