#pragma once
#include "esphome.h"
#include "lvgl.h"
#include <map>
#include <string>
#include <vector>
#include <sstream>

using esphome::i18n::tr;
using esphome::i18n::set_locale;

class I18nHelper {
public:
    // Registry for Labels: widget pointer -> translation key
    std::map<lv_obj_t *, std::string> registry_label;
    
    // Registry for Dropdowns: widget pointer -> list of translation keys
    std::map<lv_obj_t *, std::vector<std::string>> registry_dropdown;

    // Callback to clean up registries when a widget is deleted
    static void on_widget_delete(lv_event_t *e) {
        ESP_LOGD("i18n_helper", "on_widget_delete invoked");
        I18nHelper *self = (I18nHelper *)lv_event_get_user_data(e);
        lv_obj_t *obj = lv_event_get_current_target(e);
        if (self) {
            self->registry_label.erase(obj);
            self->registry_dropdown.erase(obj);
        }
    }

    // --- LABEL LOGIC ---
    std::string translate_label(lv_obj_t *obj, const char *key) {
        if (this->registry_label.find(obj) == this->registry_label.end()) {
            lv_obj_add_event_cb(obj, on_widget_delete, LV_EVENT_DELETE, this);
            this->registry_label[obj] = std::string(key);
        } 
        this->registry_label[obj] = std::string(key);
        return tr(key);
    }

    // --- DROPDOWN LOGIC ---
    void translate_select(esphome::lvgl::LvDropdownType* dropdown) {
        if (dropdown == nullptr) return;
        lv_obj_t* obj = dropdown->obj;

        // 1. If this is the first time we see this dropdown, read and store its keys
        if (this->registry_dropdown.find(obj) == this->registry_dropdown.end()) {
            const char *raw_opts = lv_dropdown_get_options(obj);
            if (raw_opts == nullptr) {
              ESP_LOGW("i18n_helper", "Failed to register dropdown - no options listed")
              return;
            }

            // Split the raw string "key1\nkey2" into a vector
            std::vector<std::string> keys;
            std::string s(raw_opts);
            std::string token;
            std::istringstream tokenStream(s);
            while (std::getline(tokenStream, token, '\n')) {
                // Handle Windows-style \r\n just in case, though LVGL usually uses \n
                if (!token.empty() && token.back() == '\r') {
                    token.pop_back();
                }
                keys.push_back(token);
            }

            // Store keys and add delete callback
            this->registry_dropdown[obj] = keys;
            lv_obj_add_event_cb(obj, on_widget_delete, LV_EVENT_DELETE, this);
        }

        // 2. Translate and Build new options string
        refresh_dropdown_options(obj);
    }

    // Helper to rebuild options from stored keys
    void refresh_dropdown_options(lv_obj_t *obj) {
        if (this->registry_dropdown.find(obj) == this->registry_dropdown.end()) return;
        
        std::vector<std::string> &keys = this->registry_dropdown[obj];
        std::string final_opts = "";

        for (size_t i = 0; i < keys.size(); i++) {
            final_opts += tr(keys[i].c_str());
            if (i < keys.size() - 1) {
                final_opts += "\n";
            }
        }

        // 3. Set the new translated options
        lv_dropdown_set_options(obj, final_opts.c_str());
    }

    // --- UPDATE ALL ---
    void update_translations() {
        for (auto const &[obj, key] : this->registry_label) {
            std::string new_text = tr(key.c_str());
            lv_label_set_text(obj, new_text.c_str());
        }

        // Update Dropdowns
        for (auto const &[obj, keys] : this->registry_dropdown) {
            refresh_dropdown_options(obj);
        }
    }
};

// Global instance
I18nHelper i18n_helper;

inline std::string tr_label(lv_obj_t *obj, const char *key) {
    return i18n_helper.translate_label(obj, key);
}

inline void tr_select(esphome::lvgl::LvDropdownType *obj) {
    i18n_helper.translate_select(obj);
}
