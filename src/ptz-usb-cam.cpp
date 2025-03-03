/* Pan Tilt Zoom VISCA over TCP implementation
	*
	* Copyright 2021 Grant Likely <grant.likely@secretlab.ca>
	*
	* SPDX-License-Identifier: GPLv2
	*/

#include "imported/qt-wrappers.hpp"
#include <obs-properties.h>
#include <obs.hpp>
#include "ptz-usb-cam.hpp"

PTZUSBCam::PTZUSBCam(OBSData config) : PTZDevice(config)
{
	set_config(config);
	obs_data_array_t* presetArray = obs_data_get_array(settings, "presets");
    size_t count = obs_data_array_count(presetArray);
    for (size_t i = 0; i < count; ++i) {
        obs_data_t* preset = obs_data_array_item(presetArray, i);
        int id = static_cast<int>(obs_data_get_int(preset, "preset_id"));
        PtzUsbCamPreset& p = presets[id];
        p.pan = static_cast<int>(obs_data_get_int(preset, "pan"));
        p.tilt = static_cast<int>(obs_data_get_int(preset, "tilt"));
        p.zoom = static_cast<int>(obs_data_get_int(preset, "zoom"));
        p.focusAuto = obs_data_get_bool(preset, "focusauto");
        p.focus = static_cast<int>(obs_data_get_int(preset, "focus"));
        p.whitebalAuto = obs_data_get_bool(preset, "whitebalauto");
        p.temperature = static_cast<int>(obs_data_get_int(preset, "temperature"));
        obs_data_release(preset);
    }
    obs_data_array_release(presetArray);
}

QString PTZUSBCam::description()
{
	return QString("USB CAM (UVC)");
}

void PTZUSBCam::set_config(OBSData config)
{
	PTZDevice::set_config(config);
}

OBSData PTZUSBCam::get_config()
{
	OBSData config = PTZDevice::get_config();
	return config;
}

obs_properties_t *PTZUSBCam::get_obs_properties()
{
	obs_properties_t *ptz_props = PTZDevice::get_obs_properties();
	obs_properties_remove_by_name(ptz_props, "interface");
	log_source_settings();
	return ptz_props;
}


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

void PTZUSBCam::pantilt_abs(double pan, double tilt)
{
    /*
	OnvifPTZService c;
	c.AbsoluteMove(m_PTZAddress, username, password, m_selectedMedia.token,
		       pan, tilt, 0.0);
									*/
}

void PTZUSBCam::pantilt_rel(double pan, double tilt)
{
    /*
	OnvifPTZService c;
	c.RelativeMove(m_PTZAddress, username, password, m_selectedMedia.token,
		       pan, tilt, 0.0);
									*/
}

void PTZUSBCam::pantilt_home()
{
    /*
	OnvifPTZService c;
	c.GoToHomePosition(m_PTZAddress, username, password,
			   m_selectedMedia.token);
						*/
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

#define PTZ_PAN_ABSOLUTE "Pan Absolute"
#define PTZ_TILT_ABSOLUTE "Tilt Absolute"
#define PTZ_ZOOM_ABSOLUTE "Zoom Absolute"
#define PTZ_FOCUS_AUTO "Focus Auto"
#define PTZ_FOCUS_ABSOLUTE "Focus Absolute"
#define PTZ_WHITE_BALANCE_TEMP_AUTO "White Balance Temperature Auto"
#define PTZ_WHITE_BALANCE_TEMP "White Balance Temperature"

void PTZUSBCam::memory_set(int i)
{
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
