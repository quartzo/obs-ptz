/* Pan Tilt Zoom VISCA over TCP implementation
	*
	* Copyright 2021 Grant Likely <grant.likely@secretlab.ca>
	*
	* SPDX-License-Identifier: GPLv2
	*/

#include "imported/qt-wrappers.hpp"
#include "ptz-device.hpp"
#include <obs-data.h>
#include <obs-properties.h>
#include <obs.h>
#include <obs.hpp>
#include "ptz-usb-cam.hpp"

void PTZUSBCam::ptz_tick_callback(void *param, float seconds) {
    PTZUSBCam *cam = static_cast<PTZUSBCam *>(param);
    cam->ptz_tick(seconds);
}

PTZUSBCam::PTZUSBCam(OBSData config) : PTZDevice(config)
{
	set_config(config);
	pantilt_abs(0, 0);
	zoom_abs(0);
	set_autofocus(true);
	obs_add_tick_callback(ptz_tick_callback, this);
}

PTZUSBCam::~PTZUSBCam()
{
	obs_remove_tick_callback(ptz_tick_callback, this);
}

QString PTZUSBCam::description()
{
	return QString("USB CAM (UVC)");
}

void PTZUSBCam::set_config(OBSData config)
{
	PTZDevice::set_config(config);
	OBSDataArrayAutoRelease presetArray = obs_data_get_array(config, "presets_memory");
    size_t count = obs_data_array_count(presetArray);
    for (size_t i = 0; i < count; ++i) {
        OBSDataAutoRelease preset = obs_data_array_item(presetArray, i);
        int id = static_cast<int>(obs_data_get_int(preset, "preset_id"));
        PtzUsbCamPos p = PtzUsbCamPos();
        p.pan = obs_data_get_double(preset, "pan");
        p.tilt = obs_data_get_double(preset, "tilt");
        p.zoom = obs_data_get_double(preset, "zoom");
        p.focusAuto = obs_data_get_bool(preset, "focusauto");
        p.focus = obs_data_get_double(preset, "focus");
        // p.whitebalAuto = obs_data_get_bool(preset, "whitebalauto");
        // p.temperature = obs_data_get_double(preset, "temperature");
        presets[id] = p;
    }
}

OBSData PTZUSBCam::get_config()
{
	OBSData config = PTZDevice::get_config();
	OBSDataArrayAutoRelease presetArray = obs_data_array_create();
	for (auto it = presets.constBegin(); it != presets.constEnd(); ++it) {
		const PtzUsbCamPos &preset = it.value();
		OBSDataAutoRelease presetData = obs_data_create();
		obs_data_set_double(presetData, "preset_id", it.key());
		obs_data_set_double(presetData, "pan", preset.pan);
		obs_data_set_double(presetData, "tilt", preset.tilt);
		obs_data_set_double(presetData, "zoom", preset.zoom);
		obs_data_set_bool(presetData, "focusauto", preset.focusAuto);
		obs_data_set_double(presetData, "focus", preset.focus);
		// obs_data_set_bool(presetData, "whitebalauto",
		// 		  preset.whitebalAuto);
		// obs_data_set_double(presetData, "temperature", preset.temperature);
		obs_data_array_push_back(presetArray, presetData);
    }
    obs_data_set_array(config, "presets_memory", presetArray);
	return config;
}

obs_properties_t *PTZUSBCam::get_obs_properties()
{
	obs_properties_t *ptz_props = PTZDevice::get_obs_properties();
	obs_properties_remove_by_name(ptz_props, "interface");
	log_source_settings();
	return ptz_props;
}

#define PTZ_PAN_ABSOLUTE "Pan, Absolute"
#define PTZ_TILT_ABSOLUTE "Tilt, Absolute"
#define PTZ_ZOOM_ABSOLUTE "Zoom, Absolute"
#define PTZ_FOCUS_AUTO "Focus, Automatic Continuous"
#define PTZ_FOCUS_ABSOLUTE "Focus, Absolute"
#define PTZ_WHITE_BALANCE_TEMP_AUTO "White Balance, Automatic"
#define PTZ_WHITE_BALANCE_TEMP "White Balance Temperature"

void PTZUSBCam::do_update()
{
    status &= ~(STATUS_PANTILT_SPEED_CHANGED |
			    STATUS_ZOOM_SPEED_CHANGED | STATUS_FOCUS_SPEED_CHANGED);
}

void PTZUSBCam::ptz_tick(float seconds) {
    tick_elapsed += seconds;
    if (tick_elapsed < 0.05f) // Atualiza a cada 50 ms
        return;
    if (pan_speed != 0.0 || tilt_speed != 0.0) {
        pantilt_rel(pan_speed*tick_elapsed, tilt_speed*tick_elapsed);
    }
    if (zoom_speed != 0.0) {
        zoom_abs(now_pos.zoom + zoom_speed*tick_elapsed);
    }
    if (focus_speed != 0.0) {
        focus_abs(now_pos.focus + focus_speed*tick_elapsed);
    }
    tick_elapsed = 0.0f; // Reseta após a atualização
}

// Autorelease object definitions
inline void ___properties_dummy_addref(obs_properties_t *) {}
using OBSPropertiesAutoDestroy = OBSRef<obs_properties_t *, ___properties_dummy_addref, obs_properties_destroy>;

void PTZUSBCam::pantilt_abs(double pan, double tilt)
{
    now_pos.pan = std::clamp(pan, -1.0, 1.0);
    now_pos.tilt = std::clamp(tilt, -1.0, 1.0);

    OBSSourceAutoRelease src = obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;
    OBSPropertiesAutoDestroy props = obs_source_properties(src);
    if (!props)
        return;
    OBSDataAutoRelease settings = obs_source_get_settings(src);

    obs_property_t *pan_prop = obs_properties_get(props, PTZ_PAN_ABSOLUTE);
    if (!pan_prop || obs_property_get_type(pan_prop) != OBS_PROPERTY_INT)
        return;
    int min = obs_property_int_min(pan_prop);
    int max = obs_property_int_max(pan_prop);
    int i_pan = std::clamp(static_cast<int>(now_pos.pan * max), min, max);
    obs_data_set_int(settings, PTZ_PAN_ABSOLUTE, i_pan);
    obs_property_modified(pan_prop, settings);

    obs_property_t *tilt_prop = obs_properties_get(props, PTZ_TILT_ABSOLUTE);
    if (!tilt_prop || obs_property_get_type(tilt_prop) != OBS_PROPERTY_INT)
        return;
    min = obs_property_int_min(tilt_prop);
    max = obs_property_int_max(tilt_prop);
    int i_tilt = std::clamp(static_cast<int>(now_pos.tilt * max), min, max);
    obs_data_set_int(settings, PTZ_TILT_ABSOLUTE, i_tilt);
    obs_property_modified(tilt_prop, settings);

    obs_source_update(src, settings);
}

void PTZUSBCam::pantilt_rel(double pan, double tilt)
{
    pantilt_abs(now_pos.pan+pan, now_pos.tilt+tilt);
}

void PTZUSBCam::pantilt_home()
{
    pantilt_abs(0, 0);
}

void PTZUSBCam::zoom_abs(double pos)
{
    now_pos.zoom = std::clamp(pos, 0.0, 1.0);

    OBSSourceAutoRelease src = obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;
    OBSPropertiesAutoDestroy props = obs_source_properties(src);
    if (!props)
        return;
    OBSDataAutoRelease settings = obs_source_get_settings(src);

    obs_property_t *zoom_prop = obs_properties_get(props, PTZ_ZOOM_ABSOLUTE);
    if (!zoom_prop || obs_property_get_type(zoom_prop) != OBS_PROPERTY_INT)
        return;
    int min = obs_property_int_min(zoom_prop);
    int max = obs_property_int_max(zoom_prop);
    int i_zoom = std::clamp(static_cast<int>(now_pos.zoom * max), min, max);
    obs_data_set_int(settings, PTZ_ZOOM_ABSOLUTE, i_zoom);
    obs_property_modified(zoom_prop, settings);

    obs_source_update(src, settings);
}

void PTZUSBCam::focus_abs(double pos)
{
    now_pos.focus = std::clamp(pos, 0.0, 1.0);

    OBSSourceAutoRelease src = obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;
    OBSPropertiesAutoDestroy props = obs_source_properties(src);
    if (!props)
        return;
    OBSDataAutoRelease settings = obs_source_get_settings(src);

    obs_property_t *focus_prop = obs_properties_get(props, PTZ_FOCUS_ABSOLUTE);
    if (!focus_prop || obs_property_get_type(focus_prop) != OBS_PROPERTY_INT)
        return;
    int min = obs_property_int_min(focus_prop);
    int max = obs_property_int_max(focus_prop);
    int i_focus = std::clamp(static_cast<int>(now_pos.focus * max), min, max);
    obs_data_set_int(settings, PTZ_FOCUS_ABSOLUTE, i_focus);
    obs_property_modified(focus_prop, settings);

    obs_source_update(src, settings);
}

void PTZUSBCam::set_autofocus(bool enabled)
{
    now_pos.focusAuto = enabled;

    OBSSourceAutoRelease src = obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;
    OBSPropertiesAutoDestroy props = obs_source_properties(src);
    if (!props)
        return;
    OBSDataAutoRelease settings = obs_source_get_settings(src);

    obs_property_t *focus_prop = obs_properties_get(props, PTZ_FOCUS_AUTO);
    if (!focus_prop || obs_property_get_type(focus_prop) != OBS_PROPERTY_BOOL)
        return;
    obs_data_set_bool(settings, PTZ_FOCUS_AUTO, now_pos.focusAuto);
    obs_property_modified(focus_prop, settings);

    obs_source_update(src, settings);
}

void PTZUSBCam::memory_reset(int id)
{
    if (!presets.contains(id))
        return;
    presets.remove(id);
}

void PTZUSBCam::memory_set(int i)
{
    presets[i] = now_pos;
}

void PTZUSBCam::memory_recall(int id)
{
    if (!presets.contains(id))
        return;

    now_pos = presets[id];
    pantilt_abs(now_pos.pan, now_pos.tilt);
    zoom_abs(now_pos.zoom);
    set_autofocus(now_pos.focusAuto);
    if (!now_pos.focusAuto)
        focus_abs(now_pos.focus);
}

void PTZUSBCam::log_source_settings() {
    obs_source_t *src =	obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;
    obs_data_t* settings = obs_source_get_settings(src);
    blog(LOG_INFO, "PTZ: Settings for source '%s':", obs_source_get_name(src));

    // Iterar sobre todas as chaves no obs_data_t
    obs_data_item_t* item = obs_data_first(settings);
    for (; item != nullptr; obs_data_item_next(&item)) {
        const char* key = obs_data_item_get_name(item);
        switch (obs_data_item_gettype(item)) {
            case OBS_DATA_STRING:
                blog(LOG_INFO, "  %s: %s", key, obs_data_get_string(settings, key));
                break;
            case OBS_DATA_NUMBER:
                if (obs_data_item_numtype(item) == OBS_DATA_NUM_INT) {
                    blog(LOG_INFO, "  %s: %lld (int)", key, obs_data_get_int(settings, key));
                } else {
                    blog(LOG_INFO, "  %s: %f (double)", key, obs_data_get_double(settings, key));
                }
                break;
            case OBS_DATA_BOOLEAN:
                blog(LOG_INFO, "  %s: %s (bool)", key, obs_data_get_bool(settings, key) ? "true" : "false");
                break;
            case OBS_DATA_ARRAY:
                blog(LOG_INFO, "  %s: [array]", key);
                break;
            case OBS_DATA_OBJECT:
                blog(LOG_INFO, "  %s: [object]", key);
                break;
            default:
                blog(LOG_INFO, "  %s: [unknown type]", key);
                break;
        }
    }
    obs_data_release(settings);
    obs_source_release(src);
}
