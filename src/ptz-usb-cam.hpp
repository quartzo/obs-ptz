/* Pan Tilt Zoom VISCA over UVC USB
	*
	* Copyright 2025 Fabio Ferrari <fabioferrari@gmail.com>
	*
	* SPDX-License-Identifier: GPLv2
	*/
#pragma once

#include <QObject>
#include <QTcpSocket>
#include "ptz-device.hpp"

struct PtzUsbCamPreset {
        int pan = 0;
        int tilt = 0;
        int zoom = 0;
        bool focusAuto = true;
        int focus = 0;
        bool whitebalAuto = true;
        int temperature = 0;
    };

class PTZUSBCam : public PTZDevice {
	Q_OBJECT

private:
    QString m_PTZAddress{""};
    QMap<int, PtzUsbCamPreset> presets;

protected:

public:
	PTZUSBCam(OBSData config);
	void save(obs_data_t* settings) const;
	virtual QString description();

	void set_config(OBSData ptz_data);
	OBSData get_config();
	obs_properties_t *get_obs_properties();

	void do_update();
	void pantilt_rel(double pan, double tilt);
	void pantilt_abs(double pan, double tilt);
	void pantilt_home();
	void zoom_abs(double pos);
	void memory_reset(int i);
	void memory_set(int i);
	void memory_recall(int i);

	void log_source_settings();
};
