#include "emv_poller_i.h"

#include <furi.h>

#define TAG "EMV"

EmvError emv_process_error(Iso14443_4aError error) {
    switch(error) {
    case Iso14443_4aErrorNone:
        return EmvErrorNone;
    case Iso14443_4aErrorNotPresent:
        return EmvErrorNotPresent;
    case Iso14443_4aErrorTimeout:
        return EmvErrorTimeout;
    default:
        return EmvErrorProtocol;
    }
}

EmvError emv_send_chunks(EmvPoller* instance, const BitBuffer* tx_buffer, BitBuffer* rx_buffer) {
    furi_assert(instance);
    furi_assert(instance->iso14443_4a_poller);
    furi_assert(instance->tx_buffer);
    furi_assert(instance->rx_buffer);
    furi_assert(tx_buffer);
    furi_assert(rx_buffer);

    EmvError error = EmvErrorNone;

    do {
        Iso14443_4aError iso14443_4a_error = iso14443_4a_poller_send_block(
            instance->iso14443_4a_poller, tx_buffer, instance->rx_buffer);

        if(iso14443_4a_error != Iso14443_4aErrorNone) {
            error = emv_process_error(iso14443_4a_error);
            break;
        }

        bit_buffer_reset(instance->tx_buffer);

        if(bit_buffer_get_capacity_bytes(rx_buffer) <
           bit_buffer_get_capacity_bytes(instance->rx_buffer)) {
            error = EmvErrorProtocol;
            break;
        }

        bit_buffer_copy(rx_buffer, instance->rx_buffer);
    } while(false);

    return error;
}

void emv_furi_string(FuriString* string, const uint8_t* buff, uint16_t length) {
    furi_string_set_strn(string, (const char*)buff, length);
}

void emv_furi_string_card_number(FuriString* string, const uint8_t* buff, uint16_t length) {
    furi_string_reset(string);
    for(uint8_t i = 0; i < length; i++) {
        furi_string_cat_printf(string, "%02X", buff[i]);
    }
}

static bool emv_decode_response(const uint8_t* buff, uint16_t len, EmvApplicationCard* app) {
    uint16_t i = 0;
    uint16_t tag = 0, first_byte = 0;
    uint16_t tlen = 0;
    bool success = false;

    // returns success if at least one EMV tag was read

    while(i < len) {
        first_byte = buff[i];
        tlen = 0;
        if(first_byte == 0x64 || first_byte == 0x65) {
            FURI_LOG_W(TAG, "Execution error reported (%02X %02X)", first_byte, buff[i + 1]);
            i += 2;
        } else if(first_byte >= 0x67 && first_byte <= 0x6F) {
            FURI_LOG_W(TAG, "Checking error reported (%02X %02X)", first_byte, buff[i + 1]);
            i += 2;
        } else if(first_byte >= 0x67 && first_byte <= 0x6F) {
            FURI_LOG_W(TAG, "Completed with warning (%02X %02X)", first_byte, buff[i + 1]);
            i += 2;
        } else if(first_byte == 0x90) {
            FURI_LOG_T(TAG, "Completed successfully (%02X %02X)", first_byte, buff[i + 1]);
            i += 2;
        } else {
            if((first_byte & 31) == 31) { // 2-byte tag
                tag = buff[i] << 8 | buff[i + 1];
                i++;
                FURI_LOG_T(TAG, " 2-byte TLV EMV tag: %04X", tag);
            } else {
                tag = buff[i];
                FURI_LOG_T(TAG, " 1-byte TLV EMV tag: %02X", tag);
            }
            i++;
            tlen = buff[i];
            if((tlen & 128) == 128) { // long length value
                i++;
                tlen = buff[i];
                FURI_LOG_T(TAG, " 2-byte TLV length: %d", tlen);
            } else {
                FURI_LOG_T(TAG, " 1-byte TLV length: %d", tlen);
            }
            i++;
            if((first_byte & 32) == 32) { // "Constructed" -- contains more TLV data to parse
                FURI_LOG_T(TAG, "Constructed TLV %X", tag);
                if(!emv_decode_response(&buff[i], tlen, app)) {
                    FURI_LOG_T(TAG, "Failed to decode response for %02X", tag);
                    // return false;
                } else {
                    success = true;
                }
            } else {
                switch(tag) {
                case EMV_TAG_AID:
                    memcpy(app->aid.data, &buff[i], tlen);
                    app->aid.size = tlen;
                    success = true;
                    FURI_LOG_T(TAG, "found EMV_TAG_AID %02X, len: %d", tag, tlen);
                    break;
                case EMV_TAG_PRIORITY:
                    memcpy(&app->priority, &buff[i], tlen);
                    FURI_LOG_T(
                        TAG,
                        "found EMV_TAG_PRIORITY %02X, len: %d, value: %x",
                        tag,
                        tlen,
                        app->priority);
                    success = true;
                    break;
                case EMV_TAG_CARD_NAME:
                    emv_furi_string(app->name, &buff[i], tlen);
                    success = true;
                    FURI_LOG_T(
                        TAG,
                        "found EMV_TAG_CARD_NAME %02X, len: %d, value: %s",
                        tag,
                        tlen,
                        furi_string_get_cstr(app->name));
                    break;
                case EMV_TAG_PDOL:
                    memcpy(app->pdol.data, &buff[i], tlen);
                    app->pdol.size = tlen;
                    success = true;
                    FURI_LOG_T(TAG, "found EMV_TAG_PDOL %02X, len: %d", tag, tlen);
                    break;
                case EMV_TAG_AFL:
                    memcpy(app->afl.data, &buff[i], tlen);
                    app->afl.size = tlen;
                    success = true;
                    FURI_LOG_T(TAG, "found EMV_TAG_AFL %02X, len: %d", tag, tlen);
                    break;
                case EMV_TAG_CARD_NUM: // Track 2 Equivalent Data. 0xD0 delimits PAN from expiry (YYMM)
                    // [[cc cc cc cc cc cc cc cc]] D[[y ym m]]. .. .. ..
                    for(int x = 1; x < tlen; x++) {
                        if(buff[i + x + 1] > 0xD0) {
                            emv_furi_string_card_number(app->card_number, &buff[i], x + 1);

                            if(x + 3 < tlen) {
                                app->exp_year = (buff[i + x + 1] & 0x0F) * 10;
                                app->exp_year += (buff[i + x + 2] & 0xF0) >> 4;

                                app->exp_month = (buff[i + x + 2] & 0x0F) * 10;
                                app->exp_month += (buff[i + x + 3] & 0xF0) >> 4;
                            }
                            break;
                        }
                    }
                    success = true;
                    FURI_LOG_T(
                        TAG,
                        "found EMV_TAG_CARD_NUM %02X, len: %d",
                        EMV_TAG_CARD_NUM,
                        furi_string_size(app->card_number));
                    FURI_LOG_T(TAG, "Card number: %s", furi_string_get_cstr(app->card_number));
                    FURI_LOG_T(TAG, "Expiration: %02d/%02d", app->exp_month, app->exp_year);
                    break;
                case EMV_TAG_PAN:
                    emv_furi_string(app->card_number, &buff[i], tlen);
                    success = true;
                    break;
                case EMV_TAG_EXP_DATE:
                    app->exp_year = buff[i];
                    app->exp_month = buff[i + 1];
                    success = true;
                    break;
                case EMV_TAG_CURRENCY_CODE:
                    app->currency_code = (buff[i] << 8 | buff[i + 1]);
                    success = true;
                    break;
                case EMV_TAG_COUNTRY_CODE:
                    app->country_code = (buff[i] << 8 | buff[i + 1]);
                    success = true;
                    break;
                case EMV_TAG_TRANSACTION_LOG_INFO:
                    app->transaction_log_sfi = buff[i];
                    app->transaction_log_length = buff[i + 1];
                    success = true;
                    FURI_LOG_T(
                        TAG,
                        "found EMV_TAG_TRANSACTION_LOG_INFO %02X, len: %d, log sfi: %02X, log len: %d",
                        tag,
                        tlen,
                        app->transaction_log_sfi,
                        app->transaction_log_length);
                    break;
                case EMV_TAG_TRACK1_DATA:
                    FURI_LOG_T(TAG, "found EMV_TAG_TRACK1_DATA %02X, len: %d", tag, tlen);
                    break;
                case EMV_TAG_TRACK2_DATA:
                    // [[cc cc cc cc cc cc cc cc]] D[[y ym m]]. .. .. ..
                    for(int x = 1; x < tlen; x++) {
                        if(buff[i + x + 1] > 0xD0) {
                            emv_furi_string_card_number(app->card_number, &buff[i], x + 1);

                            if(x + 3 < tlen) {
                                app->exp_year = (buff[i + x + 1] & 0x0F) * 10;
                                app->exp_year += (buff[i + x + 2] & 0xF0) >> 4;

                                app->exp_month = (buff[i + x + 2] & 0x0F) * 10;
                                app->exp_month += (buff[i + x + 3] & 0xF0) >> 4;
                            }
                            break;
                        }
                    }
                    success = true;
                    FURI_LOG_T(TAG, "found EMV_TAG_TRACK2_DATA %04X, len: %d", tag, tlen);
                    FURI_LOG_T(TAG, "Card number: %s", furi_string_get_cstr(app->card_number));
                    FURI_LOG_T(TAG, "Expiration: %02d/%02d", app->exp_month, app->exp_year);
                    break;
                }
            }
        }
        i += tlen;
    }
    return success;
}

bool emv_decode_response_bit_buffer(const BitBuffer* buffer, EmvApplicationCard* app) {
    uint16_t len = bit_buffer_get_size_bytes(buffer);
    const uint8_t* data = bit_buffer_get_data(buffer);

    return emv_decode_response(data, len, app);
}

EmvError emv_poller_select_ppse(EmvPoller* instance) {
    furi_assert(instance);
    EmvData* data = instance->data;
    EmvError error;

    FURI_LOG_T(TAG, "Sending Select PPSE...");

    const uint8_t emv_select_ppse_cmd[] = {
        0x00, 0xA4, // SELECT ppse
        0x04, 0x00, // P1:By name, P2: empty
        0x0e, // Lc: Data length
        0x32, 0x50, 0x41, 0x59, 0x2e, 0x53, 0x59, // Data string: 2PAY.SYS.DDF01 (PPSE)
        0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31,
        0x00 // Le
    };

    bit_buffer_reset(instance->input_buffer);
    bit_buffer_append_bytes(
        instance->input_buffer, emv_select_ppse_cmd, sizeof(emv_select_ppse_cmd));

    do {
        error = emv_send_chunks(instance, instance->input_buffer, instance->result_buffer);

        if(error != EmvErrorNone) break;

        if(!emv_decode_response_bit_buffer(instance->result_buffer, data->app)) {
            error = EmvErrorProtocol;
            break;
        }
    } while(false);

    if(error != EmvErrorNone) {
        FURI_LOG_W(TAG, "Select PPSE failed, error: %d", error);
    }

    return error;
}

EmvError emv_poller_start_application(EmvPoller* instance) {
    furi_assert(instance);
    EmvData* data = instance->data;
    EmvError error;

    const uint8_t emv_select_header[] = {
        0x00,
        0xA4, // SELECT application
        0x04,
        0x00 // P1:By name, P2:First or only occurence
    };

    bit_buffer_reset(instance->input_buffer);

    // Copy header
    bit_buffer_append_bytes(instance->input_buffer, emv_select_header, sizeof(emv_select_header));

    // Copy AID length
    bit_buffer_append_byte(instance->input_buffer, (uint8_t)data->app->aid.size);

    // Copy AID
    bit_buffer_append_bytes(instance->input_buffer, data->app->aid.data, data->app->aid.size);

    // Copy final NUL
    bit_buffer_append_byte(instance->input_buffer, 0x00);

    FURI_LOG_T(TAG, "Sending Start Application...");
    do {
        error = emv_send_chunks(instance, instance->input_buffer, instance->result_buffer);

        if(error != EmvErrorNone) break;

        if(!emv_decode_response_bit_buffer(instance->result_buffer, data->app)) {
            error = EmvErrorProtocol;
            break;
        }
    } while(false);

    if(error != EmvErrorNone) {
        FURI_LOG_E(TAG, "Start Application failed, error: %d", error);
    }

    return error;
}

EmvError emv_poller_get_processing_options(EmvPoller* instance) {
    bool card_num_read = false;
    // Cla: 80, Ins: A8, P1: 00, P2: 00
    const uint8_t emv_gpo_header[] = {0x80, 0xA8, 0x00, 0x00};
    APDU pdol_data = {0, {0}};
    EmvData* data = instance->data;
    EmvError error;

    // TODO: update pdol_data

    furi_assert(instance->input_buffer);

    bit_buffer_reset(instance->input_buffer);

    // Copy header
    bit_buffer_append_bytes(instance->input_buffer, emv_gpo_header, sizeof(emv_gpo_header));

    // Prepare and copy PDOL parameters
    bit_buffer_append_byte(instance->input_buffer, 0x02 + pdol_data.size);
    bit_buffer_append_byte(instance->input_buffer, 0x83);
    bit_buffer_append_byte(instance->input_buffer, pdol_data.size);

    bit_buffer_append_bytes(instance->input_buffer, pdol_data.data, pdol_data.size);

    // Copy final NUL
    bit_buffer_append_byte(instance->input_buffer, 0x00);

    FURI_LOG_T(TAG, "Sending Get Processing Options...");

    do {
        error = emv_send_chunks(instance, instance->input_buffer, instance->result_buffer);

        if(error != EmvErrorNone) {
            FURI_LOG_I(TAG, "b1");
            break;
        }

        if(!emv_decode_response_bit_buffer(instance->result_buffer, data->app)) {
            FURI_LOG_I(TAG, "b2");
            error = EmvErrorProtocol;
            break;
        }

        if(!furi_string_empty(instance->app->card_number)) {
            card_num_read = true;
        }
    } while(false);

    if(error != EmvErrorNone) {
        FURI_LOG_W(TAG, "Get Processing Options failed, error: %d", error);
    }

    return error;
}
