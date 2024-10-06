#pragma once

#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
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
	friend class WebsocketClient;
	friend class Control;

	SemaphoreHandle_t ControlsSemaphore;

	// Store UI Title
	const char *uiTitle = "ESPUI";

	AsyncWebServer *server {};
	AsyncWebSocket *ws {};

	std::map<uint32_t, WebsocketClient> clients;

public:
	std::shared_ptr<Control> root {};

protected:
	void NotifyClients(ClientUpdateType_t newState);

	void NotifyClient(uint32_t WsClientId, ClientUpdateType_t newState);

	void SendJsonDocToWebSocket(const JsonDocument &document) const;

	void onWsEvent(AsyncWebSocketClient *client, AwsEventType type, void *arg,
	               const uint8_t *data,
	               size_t len);

	[[nodiscard]] std::shared_ptr<Control> getControl(uint16_t id) const;

	[[nodiscard]] uint32_t GetNextControlChangeId();

public:
	ESPUIClass()
	{
		root = std::make_shared<Control>(this);
		ControlsSemaphore = xSemaphoreCreateMutex();
		xSemaphoreGive(ControlsSemaphore);
	}

	void begin(const char *_title, uint16_t port = 80); // Setup server and page in Memory mode

	// Update Elements
	void updateControlValue(Control &control, const std::string &value);

	void updateControlLabel(Control &control, const std::string &value);

	void updateControl(Control &control);

	void print(Control &control, const std::string &value);

	void updateLabel(Control &control, const std::string &value);

	void updateButton(Control &control, const std::string &value);

	void updateSwitcher(Control &control, bool nValue);

	void updateSlider(Control &control, int nValue);

	void updateNumber(Control &control, int number);

	void updateText(Control &control, const std::string &nValue);

	void updateSelect(Control &control, const std::string &nValue);

	void updateGauge(Control &control, int number);

	void updateTime(Control &control);

	void clearGraph(const Control &control) const;

	void addGraphPoint(const Control &control, int nValue) const;

	void setPanelStyle(Control &control, const std::string &style);

	void setElementStyle(Control &control, const std::string &style);

	void setInputType(Control &control, const std::string &type);

	void setPanelWide(Control &control, bool wide);

	void setVertical(Control &control, bool vert = true);

	void setEnabled(Control &control, bool enabled = true);

	void updateVisibility(Control &control, bool visibility);
};
