/*
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_caps_word_ru

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>

#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/modifiers.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct caps_word_continue_item {
    uint16_t page;
    uint32_t id;
    uint8_t implicit_modifiers;
};

struct behavior_caps_word_ru_config {
    zmk_mod_flags_t mods;
    uint8_t continuations_count;
    struct caps_word_continue_item continuations[];
};

struct behavior_caps_word_ru_data {
    bool active;
};

static void activate_caps_word_ru(const struct device *dev) {
    struct behavior_caps_word_ru_data *data = dev->data;
    data->active = true;
}

static void deactivate_caps_word_ru(const struct device *dev) {
    struct behavior_caps_word_ru_data *data = dev->data;
    data->active = false;
}

static int on_caps_word_ru_binding_pressed(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_caps_word_ru_data *data = dev->data;

    if (data->active) {
        deactivate_caps_word_ru(dev);
    } else {
        activate_caps_word_ru(dev);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_caps_word_ru_binding_released(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_caps_word_ru_driver_api = {
    .binding_pressed = on_caps_word_ru_binding_pressed,
    .binding_released = on_caps_word_ru_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

static int caps_word_ru_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_caps_word_ru, caps_word_ru_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_caps_word_ru, zmk_keycode_state_changed);

#define GET_DEV(inst) DEVICE_DT_INST_GET(inst),
static const struct device *devs[] = {DT_INST_FOREACH_STATUS_OKAY(GET_DEV)};

static bool caps_word_ru_is_caps_includelist(const struct behavior_caps_word_ru_config *config,
                                             uint16_t usage_page, uint8_t usage_id,
                                             uint8_t implicit_modifiers) {
    for (int i = 0; i < config->continuations_count; i++) {
        const struct caps_word_continue_item *continuation = &config->continuations[i];

        /* Same logic as upstream:
         * - allow explicit mods to participate in matching (for LS(X) style entries)
         * - but our “cancel-on-ctrl/alt/gui” logic is handled earlier.
         */
        if (continuation->page == usage_page && continuation->id == usage_id &&
            (continuation->implicit_modifiers &
             (implicit_modifiers | zmk_hid_get_explicit_mods())) == continuation->implicit_modifiers) {
            return true;
        }
    }

    return false;
}

/* RU extras: keys that produce ХЪЖЭБЮ on common Russian layout,
 * but are punctuation in US: [ ] ; ' , .
 */
static bool caps_word_ru_is_alpha(uint8_t usage_id) {
    if (usage_id >= HID_USAGE_KEY_KEYBOARD_A && usage_id <= HID_USAGE_KEY_KEYBOARD_Z) {
        return true;
    }

    switch (usage_id) {
    case HID_USAGE_KEY_KEYBOARD_LEFT_BRACKET_AND_LEFT_BRACE:        /* [ -> Х */
    case HID_USAGE_KEY_KEYBOARD_RIGHT_BRACKET_AND_RIGHT_BRACE:      /* ] -> Ъ */
    case HID_USAGE_KEY_KEYBOARD_SEMICOLON_AND_COLON:                /* ; -> Ж */
    case HID_USAGE_KEY_KEYBOARD_APOSTROPHE_AND_QUOTE:               /* ' -> Э */
    case HID_USAGE_KEY_KEYBOARD_COMMA_AND_LESS_THAN:                /* , -> Б */
    case HID_USAGE_KEY_KEYBOARD_PERIOD_AND_GREATER_THAN:            /* . -> Ю */
        return true;
    default:
        return false;
    }
}

static bool caps_word_ru_is_numeric(uint8_t usage_id) {
    return (usage_id >= HID_USAGE_KEY_KEYBOARD_1_AND_EXCLAMATION &&
            usage_id <= HID_USAGE_KEY_KEYBOARD_0_AND_RIGHT_PARENTHESIS);
}

static bool caps_word_ru_is_shift_key(const struct zmk_keycode_state_changed *ev) {
    if (ev->usage_page != HID_USAGE_KEY) {
        return false;
    }

    return (ev->keycode == HID_USAGE_KEY_KEYBOARD_LEFTSHIFT ||
            ev->keycode == HID_USAGE_KEY_KEYBOARD_RIGHTSHIFT);}

/* Only shift is allowed while caps_word_ru is active.
 * If ctrl/alt/gui are involved (explicitly held OR implicitly applied),
 * we cancel caps_word_ru immediately.
 */
static bool caps_word_ru_has_disallowed_mods(const struct zmk_keycode_state_changed *ev) {
    const zmk_mod_flags_t explicit_mods = zmk_hid_get_explicit_mods();
    const uint8_t implicit_mods = ev->implicit_modifiers;

    const zmk_mod_flags_t disallowed_explicit =
        explicit_mods & (MOD_LCTL | MOD_RCTL | MOD_LALT | MOD_RALT | MOD_LGUI | MOD_RGUI);

    const uint8_t disallowed_implicit =
        implicit_mods & (MOD_LCTL | MOD_RCTL | MOD_LALT | MOD_RALT | MOD_LGUI | MOD_RGUI);

    return (disallowed_explicit != 0) || (disallowed_implicit != 0);
}

static void caps_word_ru_enhance_usage(const struct behavior_caps_word_ru_config *config,
                                       struct zmk_keycode_state_changed *ev) {
    if (ev->usage_page != HID_USAGE_KEY || !caps_word_ru_is_alpha(ev->keycode)) {
        return;
    }

    ev->implicit_modifiers |= config->mods;
}

static int caps_word_ru_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < ARRAY_SIZE(devs); i++) {
        const struct device *dev = devs[i];
        struct behavior_caps_word_ru_data *data = dev->data;

        if (!data->active) {
            continue;
        }

        const struct behavior_caps_word_ru_config *config = dev->config;

        /* Cancel on any ctrl/alt/gui involvement */
        if (caps_word_ru_has_disallowed_mods(ev)) {
            deactivate_caps_word_ru(dev);
            continue;
        }

        /* Cancel on modifier key press except Shift */
        if (is_mod(ev->usage_page, ev->keycode) && !caps_word_ru_is_shift_key(ev)) {
            deactivate_caps_word_ru(dev);
            continue;
        }

        /* Apply shift (or configured mods) to “alphabetic” keys */
        caps_word_ru_enhance_usage(config, ev);

        /* Deactivate on anything not in the “continue set”.
         * Like upstream: alphas + numerics always continue.
         * Mods continue only for Shift (handled above).
         */
        if (!caps_word_ru_is_alpha(ev->keycode) && !caps_word_ru_is_numeric(ev->keycode) &&
            !caps_word_ru_is_caps_includelist(config, ev->usage_page, ev->keycode, ev->implicit_modifiers)) {
            deactivate_caps_word_ru(dev);
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

#define CAPS_WORD_RU_LABEL(i, _n) DT_INST_LABEL(i)
#define PARSE_BREAK(i)                                                                                 \
    {                                                                                                  \
        .page = ZMK_HID_USAGE_PAGE(i), .id = ZMK_HID_USAGE_ID(i), .implicit_modifiers = SELECT_MODS(i) \
    }
#define BREAK_ITEM(i, n) PARSE_BREAK(DT_INST_PROP_BY_IDX(n, continue_list, i))

#define KP_INST(n)                                                                                    \
    static struct behavior_caps_word_ru_data behavior_caps_word_ru_data_##n = {.active = false};      \
    static const struct behavior_caps_word_ru_config behavior_caps_word_ru_config_##n = {             \
        .mods = DT_INST_PROP_OR(n, mods, MOD_LSFT),                                                   \
        .continuations = {LISTIFY(DT_INST_PROP_LEN(n, continue_list), BREAK_ITEM, (, ), n)},          \
        .continuations_count = DT_INST_PROP_LEN(n, continue_list),                                    \
    };                                                                                                \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &behavior_caps_word_ru_data_##n,                           \
                            &behavior_caps_word_ru_config_##n, POST_KERNEL,                           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_caps_word_ru_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY */
