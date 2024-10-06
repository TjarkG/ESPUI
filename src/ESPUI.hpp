#pragma once

#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <list>
#include <map>

#include "ESPUIclient.hpp"
#include "ESPUIcontrol.hpp"

// Values
enum class UpdateType : uint8_t
{
	//Tab
	TabValue,
	//Button
	ButtonDown, ButtonUp,
	//Pad
	PadLeftDown, PadLeftUp, PadRightDown, PadRightUp, PadForwardDown, PadForwardUp,
	PadBackDown, PadBackUp, PadCenterDown, PadCenterUp,
	//Switch
	SwitchOn, SwitchOff,
	//Slider
	Slider,
	//Number
	Number,
	//Text
	Text,
	//Select
	SelectChanged,
	//Time
	Time
};

class ESPUIClass
{
protected:
	friend class esp_ui_client;
	friend class Control;

	SemaphoreHandle_t ControlsSemaphore;

	// Store UI Title
	const char *ui_title = "ESPUI";

	AsyncWebServer *server {};
	AsyncWebSocket *ws {};

	std::map<uint32_t, esp_ui_client *> MapOfClients;

	std::list<std::shared_ptr<Control> > controls;

	void NotifyClients(ClientUpdateType_t newState) const;

	void NotifyClient(uint32_t WsClientId, ClientUpdateType_t newState);

	void SendJsonDocToWebSocket(const JsonDocument &document) const;

	void onWsEvent(AsyncWebSocketClient *client, AwsEventType type, void *arg,
	               const uint8_t *data,
	               size_t len);

	std::shared_ptr<Control> getControl(uint16_t id) const;

	std::shared_ptr<Control> getControlNoLock(uint16_t id) const;

	void jsonReload() const;

	uint32_t GetNextControlChangeId() const;

public:
	ESPUIClass()
	{
		ControlsSemaphore = xSemaphoreCreateMutex();
		xSemaphoreGive(ControlsSemaphore);
	}

	void begin(const char *_title, uint16_t port = 80); // Setup server and page in Memory mode

	std::shared_ptr<Control> addControl(const Control &control);

	std::shared_ptr<Control> add(ControlType type, const std::string &label = "", const std::string &value = "",
	                             ControlColor color = ControlColor::None,
	                             const std::function<void(Control *, UpdateType)> &callback = nullptr);

	void removeControl(const std::shared_ptr<Control> &control, bool force_rebuild_ui = false);

	// Update Elements
	void updateControlValue(Control &control, const std::string &value) const;

	void updateControlLabel(Control &control, const std::string &value) const;

	void updateControl(Control &control) const;

	void print(Control &control, const std::string &value) const;

	void updateLabel(Control &control, const std::string &value) const;

	void updateButton(Control &control, const std::string &value) const;

	void updateSwitcher(Control &control, bool nValue) const;

	void updateSlider(Control &control, int nValue) const;

	void updateNumber(Control &control, int number) const;

	void updateText(Control &control, const std::string &nValue) const;

	void updateSelect(Control &control, const std::string &nValue) const;

	void updateGauge(Control &control, int number) const;

	void updateTime(Control &control) const;

	void clearGraph(const Control &control) const;

	void addGraphPoint(const Control &control, int nValue) const;

	void setPanelStyle(Control &control, const std::string &style) const;

	void setElementStyle(Control &control, const std::string &style) const;

	void setInputType(Control &control, const std::string &type) const;

	void setPanelWide(Control &control, bool wide) const;

	void setVertical(Control &control, bool vert = true) const;

	void setEnabled(Control &control, bool enabled = true) const;

	void updateVisibility(Control &control, bool visibility) const;
};
