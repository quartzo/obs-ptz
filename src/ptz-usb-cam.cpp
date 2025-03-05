/* Pan Tilt Zoom VISCA over TCP implementation
	*
	* Copyright 2021 Grant Likely <grant.likely@secretlab.ca>
	*
	* SPDX-License-Identifier: GPLv2
	*/

#include "imported/qt-wrappers.hpp"
#include <obs-properties.h>
#include <obs.h>
#include <obs.hpp>
#include "ptz-usb-cam.hpp"

PTZUSBCam::PTZUSBCam(OBSData config) : PTZDevice(config)
{
	set_config(config);
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
        PtzUsbCamPreset p = PtzUsbCamPreset();
        p.pan = static_cast<int>(obs_data_get_int(preset, "pan"));
        p.tilt = static_cast<int>(obs_data_get_int(preset, "tilt"));
        p.zoom = static_cast<int>(obs_data_get_int(preset, "zoom"));
        p.focusAuto = obs_data_get_bool(preset, "focusauto");
        p.focus = static_cast<int>(obs_data_get_int(preset, "focus"));
        p.whitebalAuto = obs_data_get_bool(preset, "whitebalauto");
        p.temperature = static_cast<int>(obs_data_get_int(preset, "temperature"));
        presets[id] = p;
    }
}

OBSData PTZUSBCam::get_config()
{
	OBSData config = PTZDevice::get_config();
	OBSDataArrayAutoRelease presetArray = obs_data_array_create();
	for (auto it = presets.constBegin(); it != presets.constEnd(); ++it) {
		const PtzUsbCamPreset &preset = it.value();
		OBSDataAutoRelease presetData = obs_data_create();
		obs_data_set_int(presetData, "preset_id", it.key());
		obs_data_set_int(presetData, "pan", preset.pan);
		obs_data_set_int(presetData, "tilt", preset.tilt);
		obs_data_set_int(presetData, "zoom", preset.zoom);
		obs_data_set_bool(presetData, "focusauto", preset.focusAuto);
		obs_data_set_int(presetData, "focus", preset.focus);
		obs_data_set_bool(presetData, "whitebalauto",
				  preset.whitebalAuto);
		obs_data_set_int(presetData, "temperature", preset.temperature);
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
    /*
	if (status &
	    (STATUS_PANTILT_SPEED_CHANGED | STATUS_ZOOM_SPEED_CHANGED)) {
		status &= ~(STATUS_PANTILT_SPEED_CHANGED |
			    STATUS_ZOOM_SPEED_CHANGED);
		OnvifPTZService c;
		if (pan_speed == 0.0 && tilt_speed == 0.0 &&
		    focus_speed == 0.0) {
			QThread::msleep(200);
			c.Stop(m_PTZAddress, username, password,
			       m_selectedMedia.token);
		} else {
			c.ContinuousMove(m_PTZAddress, username, password,
					 m_selectedMedia.token, pan_speed,
					 tilt_speed, focus_speed);
		}
	}
	*/
}

// Autorelease object definitions
inline void ___properties_dummy_addref(obs_properties_t *) {}
using OBSPropertiesAutoDestroy = OBSRef<obs_properties_t *, ___properties_dummy_addref, obs_properties_destroy>;

void clamp_set_field(obs_properties_t *props, obs_data_t *settings, const char* field_name,
            double value, bool absolute) {
    obs_property_t* prop = obs_properties_get(props, field_name);
    if (!prop || obs_property_get_type(prop) != OBS_PROPERTY_INT)
        return;
    int min = obs_property_int_min(prop);
    int max = obs_property_int_max(prop);
    int clamped_value = std::clamp(value, -1.0, 1.0) * max;
    if (!absolute) {
        int current_value = obs_data_get_int(settings, field_name);
        clamped_value += current_value;
    }
    int valuei = std::clamp(clamped_value, min, max);
    printf("Field: %s, Value: %i\n", field_name, valuei);
    obs_data_set_int(settings, field_name, valuei);
}

void PTZUSBCam::pantilt_abs(double pan, double tilt) {
    printf("Abs Pan: %f, Tilt: %f\n", pan, tilt);

    OBSSourceAutoRelease src = obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;
    OBSPropertiesAutoDestroy props = obs_source_properties(src);
    if (!props)
        return;
    OBSDataAutoRelease settings = obs_source_get_settings(src);

    clamp_set_field(props, settings, PTZ_PAN_ABSOLUTE, pan, true);
    clamp_set_field(props, settings, PTZ_TILT_ABSOLUTE, tilt, true);

    obs_source_update(src, settings);
}

void PTZUSBCam::pantilt_rel(double pan, double tilt)
{
    printf("Rel Pan: %f, Tilt: %f\n", pan, tilt);

    OBSSourceAutoRelease src = obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;
    OBSPropertiesAutoDestroy props = obs_source_properties(src);
    if (!props)
        return;
    OBSDataAutoRelease settings = obs_source_get_settings(src);

    clamp_set_field(props, settings, PTZ_PAN_ABSOLUTE, pan, false);
    clamp_set_field(props, settings, PTZ_TILT_ABSOLUTE, tilt, false);

    obs_source_update(src, settings);
}

void PTZUSBCam::pantilt_home()
{
    OBSSourceAutoRelease src = obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;
    OBSPropertiesAutoDestroy props = obs_source_properties(src);
    if (!props)
        return;
    OBSDataAutoRelease settings = obs_source_get_settings(src);

    log_source_settings();

    int pan = static_cast<int>(obs_data_get_int(settings, PTZ_PAN_ABSOLUTE));
    int tilt = static_cast<int>(obs_data_get_int(settings, PTZ_TILT_ABSOLUTE));
    printf("Current pan: %d, tilt: %d\n", pan, tilt);

    obs_data_set_int(settings, PTZ_PAN_ABSOLUTE, 0);
    obs_property_t *pan_prop = obs_properties_get(props, PTZ_PAN_ABSOLUTE);
    obs_property_modified(pan_prop, settings);

    obs_data_set_int(settings, PTZ_TILT_ABSOLUTE, 0);
    obs_property_t *tilt_prop = obs_properties_get(props, PTZ_TILT_ABSOLUTE);
    obs_property_modified(tilt_prop, settings);

    obs_source_update(src, settings);
}

void PTZUSBCam::zoom_abs(double pos)
{
    /*
	OnvifPTZService c;
	c.AbsoluteMove(m_PTZAddress, username, password, m_selectedMedia.token,
		       0.0, 0.0, pos);
									*/
}

void PTZUSBCam::memory_reset(int id)
{
    if (!presets.contains(id))
        return;

    presets.remove(id);
}

void PTZUSBCam::memory_set(int i)
{
    printf("Chamada memory_set para i: %i\n", i);
    OBSSourceAutoRelease src = obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;

    OBSDataAutoRelease settings = obs_source_get_settings(src);
    PtzUsbCamPreset p;
    p.pan = static_cast<int>(obs_data_get_int(settings, PTZ_PAN_ABSOLUTE));
    p.tilt = static_cast<int>(obs_data_get_int(settings, PTZ_TILT_ABSOLUTE));
    p.zoom = static_cast<int>(obs_data_get_int(settings, PTZ_ZOOM_ABSOLUTE));
    p.focusAuto = obs_data_get_bool(settings, PTZ_FOCUS_AUTO);
    p.focus = static_cast<int>(obs_data_get_int(settings, PTZ_FOCUS_ABSOLUTE));
    p.whitebalAuto = obs_data_get_bool(settings, PTZ_WHITE_BALANCE_TEMP_AUTO);
    p.temperature = static_cast<int>(obs_data_get_int(settings, PTZ_WHITE_BALANCE_TEMP));
    presets[i] = p;

    for (auto it = presets.constBegin(); it != presets.constEnd(); ++it) {
        printf("Preset id %i:\n", it.key());
        const PtzUsbCamPreset& p = it.value();
        printf("  pan: %i\n", p.pan);
        printf("  tilt: %i\n", p.tilt);
        printf("  zoom: %i\n", p.zoom);
        printf("  focusAuto: %i\n", p.focusAuto);
        printf("  focus: %i\n", p.focus);
        printf("  whitebalAuto: %i\n", p.whitebalAuto);
        printf("  temperature: %i\n", p.temperature);
    }
    /*
    OBSDataAutoRelease plugin_settings = obs_source_get_settings(pluginSource);
    save(plugin_settings);
    obs_source_update(pluginSource, plugin_settings);
    */
}

void PTZUSBCam::memory_recall(int id)
{
    if (!presets.contains(id))
        return;
    const PtzUsbCamPreset& p = presets[id];

    OBSSourceAutoRelease src = obs_get_source_by_name(QT_TO_UTF8(objectName()));
    if (!src)
        return;

    OBSDataAutoRelease settings = obs_source_get_settings(src);
    obs_data_set_int(settings, PTZ_PAN_ABSOLUTE, p.pan);
    obs_data_set_int(settings, PTZ_TILT_ABSOLUTE, p.tilt);
    obs_data_set_int(settings, PTZ_ZOOM_ABSOLUTE, p.zoom);
    obs_data_set_bool(settings, PTZ_FOCUS_AUTO, p.focusAuto);
    if (!p.focusAuto) obs_data_set_int(settings, PTZ_FOCUS_ABSOLUTE, p.focus);
    obs_data_set_bool(settings, PTZ_WHITE_BALANCE_TEMP_AUTO, p.whitebalAuto);
    if (!p.whitebalAuto) obs_data_set_int(settings, PTZ_WHITE_BALANCE_TEMP, p.temperature);
    obs_source_update(src, settings);
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
