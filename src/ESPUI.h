#pragma once

// comment out to turn off debug output
// #define DEBUG_ESPUI true
#define WS_AUTHENTICATION false

#include <Arduino.h>

#include <ArduinoJson.h>
#if ARDUINOJSON_VERSION_MAJOR > 6
#define AllocateJsonDocument(name, size)    JsonDocument name
#define AllocateJsonArray(doc, name)        doc[name].to<JsonArray>()
#define AllocateJsonObject(doc)             doc.add<JsonObject>()
#define AllocateNamedJsonObject(t, s, n)    t[n] = s
#else
    #define AllocateJsonDocument(name, size)    DynamicJsonDocument name(size)
    #define AllocateJsonArray(doc, name)        doc.createNestedArray(name)
    #define AllocateJsonObject(doc)             doc.createNestedObject()
    #define AllocateNamedJsonObject(t, s, n)    t = s.createNestedObject(n)
#endif

#if (ESP_IDF_VERSION_MAJOR == 4 && ESP_IDF_VERSION_MINOR >= 4) || ESP_IDF_VERSION_MAJOR > 4
#include <LittleFS.h>
#else
		#include <LITTLEFS.h>
#endif
#include <map>
#include <ESPAsyncWebServer.h>

#include "ESPUIcontrol.h"
#include "ESPUIclient.h"

#include <AsyncTCP.h>
#include "WiFi.h"

#define FILE_WRITING "w"

// Message Types (and control types)

enum MessageTypes : uint8_t
{
	InitialGui = 200,
	Reload = 201,
	ExtendGUI = 210,
	UpdateGui = 220,
	ExtendedUpdateGui = 230,
};

#define UI_INITIAL_GUI  MessageTypes::InitialGui
#define UI_EXTEND_GUI   MessageTypes::ExtendGUI
#define UI_RELOAD       MessageTypes::Reload

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

enum class Verbosity : uint8_t
{
	Quiet = 0,
	Verbose,
	VerboseJSON
};

class ESPUIClass
{
public:
	ESPUIClass()
	{
		ControlsSemaphore = xSemaphoreCreateMutex();
		xSemaphoreGive(ControlsSemaphore);
	}

	unsigned int jsonUpdateDocumentSize = 2000;
	unsigned int jsonInitialDocumentSize = 8000;
	unsigned int jsonChunkNumberMax = 0;
	bool sliderContinuous = false;

	void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg,
	               const uint8_t *data,
	               size_t len);

	bool captivePortal = true;

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

	uint16_t addControl(ControlType type, const char *label);

	uint16_t addControl(ControlType type, const char *label, const String &value);

	uint16_t addControl(ControlType type, const char *label, const String &value, ControlColor color);

	uint16_t addControl(ControlType type, const char *label, const String &value, ControlColor color,
	                    uint16_t parentControl);

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

	void print(uint16_t id, const String &value) ;

	void updateLabel(uint16_t id, const String &value) ;

	void updateButton(uint16_t id, const String &value) ;

	void updateSwitcher(uint16_t id, bool nValue, int clientId = -1) ;

	void updateSlider(uint16_t id, int nValue, int clientId = -1) ;

	void updateNumber(uint16_t id, int nValue, int clientId = -1) ;

	void updateText(uint16_t id, const String &nValue, int clientId = -1) ;

	void updateSelect(uint16_t id, const String &nValue, int clientId = -1) ;

	void updateGauge(uint16_t id, int number, int clientId) ;

	void updateTime(uint16_t id, int clientId = -1) ;

	void clearGraph(uint16_t id, int clientId = -1);

	void addGraphPoint(uint16_t id, int nValue, int clientId = -1);

	void setPanelStyle(uint16_t id, const String &style, int clientId = -1);

	void setElementStyle(uint16_t id, const String &style, int clientId = -1);

	void setInputType(uint16_t id, const String &type, int clientId = -1);

	void setPanelWide(uint16_t id, bool wide) const ;

	void setVertical(uint16_t id, bool vert = true) const ;

	void setEnabled(uint16_t id, bool enabled = true, int clientId = -1);

	void updateVisibility(uint16_t id, bool visibility, int clientId = -1);

	// Variables
	const char *ui_title = "ESPUI"; // Store UI Title and Header Name
	Control *controls = nullptr;

	void jsonReload() const;

	void jsonDom(uint16_t start_idx, AsyncWebSocketClient *client = nullptr, bool Updating = false) const;

	Verbosity verbosity = Verbosity::Quiet;

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

	fs::LittleFSFS &EspuiLittleFS = LittleFS;

protected:
	friend class esp_ui_client;

	SemaphoreHandle_t ControlsSemaphore = nullptr;

	void RemoveToBeDeletedControls();

	AsyncWebServer *server {};
	AsyncWebSocket *ws {};

	uint16_t controlCount = 0;

	uint16_t addControl(ControlType type, const char *label, const String &value, ControlColor color,
	                    uint16_t parentControl, Control *control);

	void NotifyClients(esp_ui_client::ClientUpdateType_t newState) const;

	void NotifyClient(uint32_t WsClientId, esp_ui_client::ClientUpdateType_t newState);

	bool SendJsonDocToWebSocket(const JsonDocument &document, uint16_t clientId);

	std::map<uint32_t, esp_ui_client *> MapOfClients;

	uint32_t ControlChangeID = 0;
};
