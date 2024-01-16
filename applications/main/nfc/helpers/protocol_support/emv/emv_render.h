#pragma once

#include <nfc/protocols/emv/emv.h>

#include "../nfc_protocol_support_render_common.h"

void nfc_render_emv_issuer(const EmvData* data, FuriString* str);

void nfc_render_emv_card_number(const EmvData* data, FuriString* str);

void nfc_render_emv_card_expiration(const EmvData* data, FuriString* str);

void nfc_render_emv_info_screen(const EmvData* data, FuriString* str);
