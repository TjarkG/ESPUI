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
	Control *controls = nullptr;

	AsyncWebServer *server {};
	AsyncWebSocket *ws {};

	uint16_t controlCount = 0;

	std::map<uint32_t, esp_ui_client *> MapOfClients;

	uint32_t ControlChangeID = 0;

	Verbosity verbosity = Verbosity::Quiet;

	fs::LittleFSFS &EspuiLittleFS = LittleFS;

	uint16_t addControl(
		Control *control);

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

	uint16_t addControl(ControlType type, const char *label, const String &value = "",
	                    ControlColor color = ControlColor::Turquoise,
	                    uint16_t parentControl = Control::noParent);

	uint16_t addControl(ControlType type, const char *label, const String &value, ControlColor color,
	                    uint16_t parentControl, const std::function<void(Control *, int)> &callback);

	bool removeControl(uint16_t id, bool force_rebuild_ui = false);

	// create Elements
	// Create Event Button
	uint16_t button(const char *label, const std::function<void(Control *, int)> &callback, ControlColor color,
	                const String &value = "");

	uint16_t switcher(const char *label, const std::function<void(Control *, int)> &callback, ControlColor color,
	                  bool startState = false); // Create Toggle Button
	uint16_t pad(const char *label, const std::function<void(Control *, int)> &callback, ControlColor color);

	// Create Pad Control
	uint16_t padWithCenter(const char *label, const std::function<void(Control *, int)> &callback, ControlColor color);

	// Create Pad Control with Centerbutton
	uint16_t slider(const char *label, const std::function<void(Control *, int)> &callback, ControlColor color,
	                int value,
	                int min = 0, int max = 100); // Create Slider Control
	uint16_t number(const char *label, const std::function<void(Control *, int)> &callback, ControlColor color,
	                int value,
	                int min = 0, int max = 100); // Create a Number Input Control
	uint16_t text(const char *label, const std::function<void(Control *, int)> &callback, ControlColor color,
	              const String &value = ""); // Create a Text Input Control

	// Output only
	uint16_t label(const char *label, ControlColor color,
	               const String &value = ""); // Create Label
	uint16_t graph(const char *label, ControlColor color); // Create Graph display
	uint16_t gauge(const char *label, ControlColor color, int value, int min = 0,
	               int max = 100); // Create Gauge display
	void separator(const char *label); //Create separator
	uint16_t fileDisplay(const char *label, ControlColor color, const String &filename);

	// Input only
	uint16_t accelerometer(const char *label, const std::function<void(Control *, int)> &callback, ControlColor color);

	// Update Elements

	Control *getControl(uint16_t id) const;

	Control *getControlNoLock(uint16_t id) const;

	// Update Elements
	void updateControlValue(uint16_t id, const String &value, int clientId = -1);

	void updateControlValue(Control *control, const String &value, int clientId = -1);

	void updateControlLabel(uint16_t control, const char *value, int clientId = -1);

	void updateControlLabel(Control *control, const char *value, int clientId = -1);

	void updateControl(uint16_t id, int clientId = -1);

	void updateControl(Control *control, int clientId = -1);

	void print(uint16_t id, const String &value);

	void updateLabel(uint16_t id, const String &value);

	void updateButton(uint16_t id, const String &value);

	void updateSwitcher(uint16_t id, bool nValue, int clientId = -1);

	void updateSlider(uint16_t id, int nValue, int clientId = -1);

	void updateNumber(uint16_t id, int nValue, int clientId = -1);

	void updateText(uint16_t id, const String &nValue, int clientId = -1);

	void updateSelect(uint16_t id, const String &nValue, int clientId = -1);

	void updateGauge(uint16_t id, int number, int clientId);

	void updateTime(uint16_t id, int clientId = -1);

	void clearGraph(uint16_t id, int clientId = -1);

	void addGraphPoint(uint16_t id, int nValue, int clientId = -1);

	void setPanelStyle(uint16_t id, const String &style, int clientId = -1);

	void setElementStyle(uint16_t id, const String &style, int clientId = -1);

	void setInputType(uint16_t id, const String &type, int clientId = -1);

	void setPanelWide(uint16_t id, bool wide) const;

	void setVertical(uint16_t id, bool vert = true) const;

	void setEnabled(uint16_t id, bool enabled = true, int clientId = -1);

	void updateVisibility(uint16_t id, bool visibility, int clientId = -1);

	void jsonReload() const;

	void jsonDom(uint16_t start_idx, AsyncWebSocketClient *client = nullptr, bool Updating = false) const;


	uint32_t GetNextControlChangeId();

	uint16_t button(const char *label, const std::function<void(Control *, int, void *)> &callback,
	                const ControlColor color,
	                const String &value, void *userData)
	{
		return button(
			label, [callback, userData](Control *sender, const int type) { callback(sender, type, userData); },
			color, value);
	}

	uint16_t switcher(const char *label, const std::function<void(Control *, int, void *)> &callback,
	                  const ControlColor color,
	                  const bool startState, void *userData)
	{
		return switcher(label, [callback, userData](Control *sender, const int type)
		                {
			                callback(sender, type, userData);
		                },
		                color, startState);
	}

	uint16_t pad(const char *label, const std::function<void(Control *, int, void *)> &callback,
	             const ControlColor color,
	             void *userData)
	{
		return pad(label, [callback, userData](Control *sender, const int type) { callback(sender, type, userData); },
		           color);
	}

	uint16_t padWithCenter(const char *label, const std::function<void(Control *, int, void *)> &callback,
	                       const ControlColor color,
	                       void *userData)
	{
		return padWithCenter(label, [callback, userData](Control *sender, const int type)
		{
			callback(sender, type, userData);
		}, color);
	}

	uint16_t slider(const char *label, const std::function<void(Control *, int, void *)> &callback,
	                const ControlColor color,
	                const int value, const int min, const int max, void *userData)
	{
		return slider(
			label, [callback, userData](Control *sender, const int type) { callback(sender, type, userData); },
			color, value, min, max);
	}

	uint16_t number(const char *label, const std::function<void(Control *, int, void *)> &callback,
	                const ControlColor color,
	                const int value, const int min, const int max, void *userData)
	{
		return number(
			label, [callback, userData](Control *sender, const int type) { callback(sender, type, userData); },
			color, value, min, max);
	}

	uint16_t text(const char *label, const std::function<void(Control *, int, void *)> &callback,
	              const ControlColor color,
	              const String &value, void *userData)
	{
		return text(label, [callback, userData](Control *sender, const int type) { callback(sender, type, userData); },
		            color,
		            value);
	}

	uint16_t accelerometer(const char *label, const std::function<void(Control *, int, void *)> &callback,
	                       const ControlColor color,
	                       void *userData)
	{
		return accelerometer(label, [callback, userData](Control *sender, const int type)
		{
			callback(sender, type, userData);
		}, color);
	}

	AsyncWebServer *WebServer() const { return server; }

	AsyncWebSocket *WebSocket() const { return ws; }
};
