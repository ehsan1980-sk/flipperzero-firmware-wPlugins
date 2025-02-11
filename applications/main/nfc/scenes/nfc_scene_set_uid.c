#include "../nfc_app_i.h"

#include "../helpers/protocol_support/nfc_protocol_support_gui_common.h"

// Sync UID from #UID to block 0 data
void mfclassic_sync_uid(NfcDevice* instance) {
    size_t uid_len;
    const uint8_t* uid = nfc_device_get_uid(instance, &uid_len);

    MfClassicData* mfc_data = (MfClassicData*)nfc_device_get_data(instance, NfcProtocolMfClassic);
    uint8_t* block = mfc_data->block[0].data;

    // Sync UID
    for(uint8_t i = 0; i < (uint8_t)uid_len; i++) {
        block[i] = uid[i];
    }

    if(uid_len == 4) {
        // Calculate BCC
        block[uid_len] = 0;

        for(uint8_t i = 0; i < (uint8_t)uid_len; i++) {
            block[uid_len] ^= block[i];
        }
    }
}

void nfc_scene_set_uid_on_enter(void* context) {
    NfcApp* instance = context;

    size_t uid_len;
    const uint8_t* uid = nfc_device_get_uid(instance->nfc_device, &uid_len);
    memcpy(instance->byte_input_store, uid, uid_len);

    // Setup view
    ByteInput* byte_input = instance->byte_input;
    byte_input_set_header_text(byte_input, "Enter UID in hex");
    byte_input_set_result_callback(
        byte_input,
        nfc_protocol_support_common_byte_input_done_callback,
        NULL,
        instance,
        instance->byte_input_store,
        uid_len);

    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcViewByteInput);
}

bool nfc_scene_set_uid_on_event(void* context, SceneManagerEvent event) {
    NfcApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcCustomEventByteInputDone) {
            size_t uid_len = 0;
            nfc_device_get_uid(instance->nfc_device, &uid_len);
            nfc_device_set_uid(instance->nfc_device, instance->byte_input_store, uid_len);
            if(scene_manager_has_previous_scene(instance->scene_manager, NfcSceneSavedMenu)) {
                if(nfc_save(instance)) {
                    scene_manager_next_scene(instance->scene_manager, NfcSceneSaveSuccess);
                    consumed = true;
                }
            } else if(scene_manager_has_previous_scene(instance->scene_manager, NfcSceneReadMenu)) {
                scene_manager_search_and_switch_to_previous_scene(
                    instance->scene_manager, NfcSceneReadMenu);
                consumed = true;
            } else {
                if(nfc_device_get_protocol(instance->nfc_device) == NfcProtocolMfClassic)
                    mfclassic_sync_uid(instance->nfc_device);

                scene_manager_next_scene(instance->scene_manager, NfcSceneSaveName);
                consumed = true;
            }
        }
    }

    return consumed;
}

void nfc_scene_set_uid_on_exit(void* context) {
    NfcApp* instance = context;

    // Clear view
    byte_input_set_result_callback(instance->byte_input, NULL, NULL, NULL, NULL, 0);
    byte_input_set_header_text(instance->byte_input, "");
}
