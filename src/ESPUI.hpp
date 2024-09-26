#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>
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
	std::shared_ptr<Control> controls {};

	AsyncWebServer *server {};
	AsyncWebSocket *ws {};

	uint16_t controlCount = 0;

	std::map<uint32_t, esp_ui_client *> MapOfClients;

	uint32_t ControlChangeID = 0;

	Verbosity verbosity = Verbosity::Quiet;

	fs::LittleFSFS &EspuiLittleFS = LittleFS;

	void NotifyClients(esp_ui_client::ClientUpdateType_t newState) const;

	void NotifyClient(uint32_t WsClientId, esp_ui_client::ClientUpdateType_t newState);

	bool SendJsonDocToWebSocket(const JsonDocument &document, uint16_t clientId);

	void RemoveToBeDeletedControls();

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
	void beginLITTLEFS(const char *_title, uint16_t port = 80); // Setup server and page in LITTLEFS mode

	// Initially preps the filesystem and loads a lot of stuff into LITTLEFS
	void prepareFileSystem(bool format = true) const;

	// Lists LITTLEFS directory
	void list();

	void listDir(const char *dirname, uint8_t levels);

	void deleteFile(const char *path) const;

	void writeFile(const char *path, const char *data) const;

	std::shared_ptr<Control> addControl(ControlType type, const char *label, const String &value = "",
	                                    ControlColor color = ControlColor::Turquoise,
	                                    const std::shared_ptr<Control>& parentControl = nullptr);

	std::shared_ptr<Control> addControl(ControlType type, const char *label, const String &value, ControlColor color,
	                                    const std::shared_ptr<Control>& parentControl,
	                                    const std::function<void(Control *, int)> &callback);

	void removeControl(Control &control, bool force_rebuild_ui = false);

	std::shared_ptr<Control> getControl(uint16_t id) const;

	std::shared_ptr<Control> getControlNoLock(uint16_t id) const;

	// Update Elements
	void updateControlValue(Control &control, const String &value, int clientId = -1);

	void updateControlLabel(Control &control, const char *value, int clientId = -1);

	void updateControl(Control &control, int clientId = -1);

	void print(Control &control, const String &value);

	void updateLabel(Control &control, const String &value);

	void updateButton(Control &control, const String &value);

	void updateSwitcher(Control &control, bool nValue, int clientId = -1);

	void updateSlider(Control &control, int nValue, int clientId = -1);

	void updateNumber(Control &control, int nValue, int clientId = -1);

	void updateText(Control &control, const String &nValue, int clientId = -1);

	void updateSelect(Control &control, const String &nValue, int clientId = -1);

	void updateGauge(Control &control, int number, int clientId);

	void updateTime(Control &control, int clientId = -1);

	void clearGraph(const Control &control, int clientId = -1);

	void addGraphPoint(const Control &control, int nValue, int clientId = -1);

	void setPanelStyle(Control &control, const String &style, int clientId = -1);

	void setElementStyle(Control &control, const String &style, int clientId = -1);

	void setInputType(Control &control, const String &type, int clientId = -1);

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
