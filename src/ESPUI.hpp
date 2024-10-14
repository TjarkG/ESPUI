#pragma once

#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <map>

#include "ESPUIclient.hpp"
#include "Widgets/ESPUIcontrol.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/Switcher.hpp"

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
	friend class RootWidget;

	SemaphoreHandle_t ControlsSemaphore;

	// Store UI Title
	const char *uiTitle = "ESPUI";

	AsyncWebServer *server {};
	AsyncWebSocket *ws {};

	std::map<uint32_t, WebsocketClient> clients;

public:
	std::shared_ptr<RootWidget> root {};

protected:
	void NotifyClients(ClientUpdateType_t newState);

	void NotifyClient(uint32_t WsClientId, ClientUpdateType_t newState);

	void SendJsonDocToWebSocket(const JsonDocument &document) const;

	void onWsEvent(AsyncWebSocketClient *client, AwsEventType type, void *arg,
	               const uint8_t *data,
	               size_t len);

	[[nodiscard]] std::shared_ptr<Widget> getControl(uint16_t id) const;

public:
	ESPUIClass()
	{
		root = std::make_shared<RootWidget>(*this);
		ControlsSemaphore = xSemaphoreCreateMutex();
		xSemaphoreGive(ControlsSemaphore);
	}

	void begin(const char *_title, uint16_t port = 80); // Setup server and page in Memory mode

	[[nodiscard]] uint32_t GetNextControlChangeId();

	// Update Elements TODO remove all
	void updateControlValue(Widget &control, const std::string &value);

	void updateControlLabel(Widget &control, const std::string &value);

	void updateControl(Widget &control);

	void print(Widget &control, const std::string &value);

	void updateSlider(Widget &control, int nValue);

	void updateNumber(Widget &control, int number);

	void updateText(Widget &control, const std::string &nValue);

	void updateSelect(Widget &control, const std::string &nValue);

	void updateGauge(Widget &control, int number);

	void updateTime(Widget &control);

	void clearGraph(const Widget &control) const;

	void addGraphPoint(const Widget &control, int nValue) const;

	void setPanelStyle(Widget &control, const std::string &style);

	void setInputType(Widget &control, const std::string &type);

	void setPanelWide(Widget &control, bool wide);

	void setVertical(Widget &control, bool vert = true);

	void setEnabled(Widget &control, bool enabled = true);
};
