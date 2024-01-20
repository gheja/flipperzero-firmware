#include "emv_render.h"

#include "../iso14443_4a/iso14443_4a_render.h"

void nfc_render_emv_issuer(const EmvData* data, FuriString* str) {
    furi_string_cat_printf(str, "Issuer: ");
    if(furi_string_size(data->app->fci_issuer) > 0) {
        furi_string_cat(str, data->app->fci_issuer);
    } else {
        furi_string_cat(str, "(unknown)");
    }
    furi_string_cat(str, "\n");
}

void nfc_render_emv_card_number_16(const EmvData* data, FuriString* str) {
    for(uint8_t i = 0; i < 16; i++) {
        furi_string_cat_printf(str, "%c", furi_string_get_char(data->app->card_number, i));
        if(i % 4 == 3 && i < 15) {
            furi_string_cat(str, "-");
        }
    }
}

void nfc_render_emv_card_number(const EmvData* data, FuriString* str) {
    // furi_string_cat(str, "Card number: ");

    if(furi_string_size(data->app->card_number) == 0) {
        furi_string_cat(str, "(unknown card number)");
    } else if(furi_string_size(data->app->card_number) == 16) {
        nfc_render_emv_card_number_16(data, str);
    } else {
        furi_string_cat(str, data->app->card_number);
    }
    furi_string_cat(str, "\n");
}

void nfc_render_emv_card_expiration(const EmvData* data, FuriString* str) {
    furi_string_cat(str, "Exp: ");
    if(data->app->exp_year == 0 || data->app->exp_month == 0) {
        furi_string_cat(str, "(unknown)");
    } else {
        furi_string_cat_printf(str, "%02d/%02d", data->app->exp_month, data->app->exp_year);
    }
    furi_string_cat(str, "\n");
}

void nfc_render_emv_info_screen(const EmvData* data, FuriString* str) {
    furi_string_cat(str, "\e#EMV Chip Card\n");
    nfc_render_emv_issuer(data, str);
    nfc_render_emv_card_number(data, str);
    nfc_render_emv_card_expiration(data, str);
    furi_string_cat(str, data->app->extra_text);
}
