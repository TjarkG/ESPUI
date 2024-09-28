#pragma once

#include <ArduinoJson.h>
#include <map>
#include <ESPAsyncWebServer.h>

#include "ESPUIcontrol.hpp"
#include "ESPUIclient.hpp"

#include <AsyncTCP.h>

// Message Types
enum MessageTypes : uint8_t
{
	InitialGui = 200,
	Reload = 201,
	ExtendGUI = 210,
	UpdateGui = 220,
	ExtendedUpdateGui = 230,
};

// Values
#define B_DOWN (-1)
#define B_UP 1

#define P_LEFT_DOWN (-2)
#define P_LEFT_UP 2
#define P_RIGHT_DOWN (-3)
#define P_RIGHT_UP 3
#define P_FOR_DOWN (-4)
#define P_FOR_UP 4
#define P_BACK_DOWN (-5)
#define P_BACK_UP 5
#define P_CENTER_DOWN (-6)
#define P_CENTER_UP 6

#define S_ACTIVE (-7)
#define S_INACTIVE 7

#define SL_VALUE 8
#define N_VALUE 9
#define T_VALUE 10
#define S_VALUE 11
#define TM_VALUE 12

class ESPUIClass
{
public:
	enum class Verbosity : uint8_t
	{
		Quiet = 0,
		Verbose,
		VerboseJSON
	};

protected:
	friend class esp_ui_client;

	SemaphoreHandle_t ControlsSemaphore;

	// Store UI Title and Header Name
	const char *ui_title = "ESPUI";

	AsyncWebServer *server {};
	AsyncWebSocket *ws {};

	std::map<uint32_t, esp_ui_client *> MapOfClients;

	std::vector<std::shared_ptr<Control>> controls;

	uint32_t ControlChangeID = 0;

	Verbosity verbosity = Verbosity::Quiet;

	void NotifyClients(ClientUpdateType_t newState) const;

	void NotifyClient(uint32_t WsClientId, ClientUpdateType_t newState);

	bool SendJsonDocToWebSocket(const JsonDocument &document, uint16_t clientId);

public:
	ESPUIClass()
	{
		ControlsSemaphore = xSemaphoreCreateMutex();
		xSemaphoreGive(ControlsSemaphore);
	}

	void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
	               const uint8_t *data,
	               size_t len);

	void setVerbosity(const Verbosity v) { verbosity = v; }

	void begin(const char *_title, uint16_t port = 80); // Setup server and page in Memory mode

	std::shared_ptr<Control> addControl(const Control& control);

	std::shared_ptr<Control> addControl(ControlType type, const std::string &label, const std::string &value = "",
	                                    ControlColor color = ControlColor::Turquoise,
	                                    const std::shared_ptr<Control>& parentControl = nullptr);

	std::shared_ptr<Control> addControl(ControlType type, const std::string &label, const std::string &value, ControlColor color,
	                                    const std::shared_ptr<Control>& parentControl,
	                                    const std::function<void(Control *, int)> &callback);

	void removeControl(const std::shared_ptr<Control>& control, bool force_rebuild_ui = false);

	std::shared_ptr<Control> getControl(uint16_t id) const;

	std::shared_ptr<Control> getControlNoLock(uint16_t id) const;

	// Update Elements
	void updateControlValue(Control &control, const std::string &value, int clientId = -1);

	void updateControlLabel(Control &control, const std::string &value, int clientId = -1);

	void updateControl(Control &control, int clientId = -1);

	void print(Control &control, const std::string &value);

	void updateLabel(Control &control, const std::string &value);

	void updateButton(Control &control, const std::string &value);

	void updateSwitcher(Control &control, bool nValue, int clientId = -1);

	void updateSlider(Control &control, int nValue, int clientId = -1);

	void updateNumber(Control &control, int number, int clientId = -1);

	void updateText(Control &control, const std::string &nValue, int clientId = -1);

	void updateSelect(Control &control, const std::string &nValue, int clientId = -1);

	void updateGauge(Control &control, int number, int clientId);

	void updateTime(Control &control, int clientId = -1);

	void clearGraph(const Control &control, int clientId = -1);

	void addGraphPoint(const Control &control, int nValue, int clientId = -1);

	void setPanelStyle(Control &control, const std::string &style, int clientId = -1);

	void setElementStyle(Control &control, const std::string &style, int clientId = -1);

	void setInputType(Control &control, const std::string &type, int clientId = -1);

	void setPanelWide(Control &control, bool wide, int clientId = -1);

	void setVertical(Control &control, bool vert = true, int clientId = -1);

	void setEnabled(Control &control, bool enabled = true, int clientId = -1);

	void updateVisibility(Control &control, bool visibility, int clientId = -1);

	void jsonReload() const;

	void jsonDom(uint16_t start_idx, AsyncWebSocketClient *client = nullptr, bool Updating = false) const;


	uint32_t GetNextControlChangeId();

	AsyncWebServer *WebServer() const { return server; }

	AsyncWebSocket *WebSocket() const { return ws; }
};
