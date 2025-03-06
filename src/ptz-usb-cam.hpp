/* Pan Tilt Zoom VISCA over UVC USB
	*
	* Copyright 2025 Fabio Ferrari <fabioferrari@gmail.com>
	*
	* SPDX-License-Identifier: GPLv2
	*/
#pragma once

#include <QObject>
#include <QTcpSocket>
#include "protocol-helpers.hpp"
#include "ptz-device.hpp"

struct PtzUsbCamPos {
        double pan = 0;
        double tilt = 0;
        double zoom = 0;
        bool focusAuto = true;
        double focus = 0;
        // bool whitebalAuto = true;
        //double temperature = 0;
    };

class PTZUSBCam : public PTZDevice {
	Q_OBJECT

private:
    QString m_PTZAddress{""};
    PtzUsbCamPos now_pos;
    QMap<int, PtzUsbCamPos> presets;
    double tick_elapsed = 0.0f;

protected:
    static void ptz_tick_callback(void *param, float seconds);
    void ptz_tick(float seconds);

public:
	PTZUSBCam(OBSData config);
	~PTZUSBCam();
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
	void focus_abs(double pos);
	void set_autofocus(bool enabled);
	void memory_reset(int i);
	void memory_set(int i);
	void memory_recall(int i);

	void log_source_settings();
};
