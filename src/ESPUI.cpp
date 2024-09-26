#include "ESPUI.h"

#include <functional>

#include <ESPAsyncWebServer.h>

#include "dataControlsJS.h"
#include "dataGraphJS.h"
#include "dataIndexHTML.h"
#include "dataNormalizeCSS.h"
#include "dataSliderJS.h"
#include "dataStyleCSS.h"
#include "dataTabbedcontentJS.h"
#include "dataZeptoJS.h"

static String heapInfo(const __FlashStringHelper *mode)
{
	String result;

	result += ESP.getFreeHeap();
	result += ' ';

	result += mode;

	return result;
}

// ################# LITTLEFS functions
void listDir(const char *dirname, const uint8_t levels)
{
#if defined(DEBUG_ESPUI)
    if (ESPUI.verbosity)
    {
        Serial.printf_P(PSTR("Listing directory: %s\n"), dirname);
    }
#endif

	File root = ESPUI.EspuiLittleFS.open(dirname);
	if (!root)
	{
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("Failed to open directory"));
        }
#endif

		return;
	}

	if (!root.isDirectory())
	{
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("Not a directory"));
        }
#endif

		return;
	}

	File file = root.openNextFile();

	while (file)
	{
		if (file.isDirectory())
		{
#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.print(F("  DIR : "));
                Serial.println(file.name());
            }
#endif

			if (levels)
			{
				listDir(file.path(), levels - 1);
			}
		} else
		{
#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.print(F("  FILE: "));
                Serial.print(file.name());
                Serial.print(F("  SIZE: "));
                Serial.println(file.size());
            }
#endif
		}

		file = root.openNextFile();
	}
}

void ESPUIClass::list() const
{
	if (!EspuiLittleFS.begin())
	{
		Serial.println(F("Espui LittleFS Mount Failed"));
		return;
	}

	listDir("/", 1);

	Serial.print(F("Total KB: "));
	Serial.println(EspuiLittleFS.totalBytes() / 1024);
	Serial.print(F("Used KB: "));
	Serial.println(EspuiLittleFS.usedBytes() / 1024);
}

void deleteFile(const char *path)
{
	const bool exists = ESPUI.EspuiLittleFS.exists(path);
	if (!exists)
	{
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.printf_P(PSTR("File: %s does not exist, not deleting\n"), path);
        }
#endif

		return;
	}

#if defined(DEBUG_ESPUI)
    if (ESPUI.verbosity)
    {
        Serial.printf_P(PSTR("Deleting file: %s\n"), path);
    }

	bool didRemove = ESPUI.EspuiLittleFS.remove(path);
	if (didRemove)
	{
        if (ESPUI.verbosity)
        {
            Serial.println(F("File deleted"));
        }
	} else
	{
        if (ESPUI.verbosity)
        {
            Serial.println(F("Delete failed"));
        }
	}
#else
	ESPUI.EspuiLittleFS.remove(path);
#endif
}

void ESPUIClass::writeFile(const char *path, const char *data) const
{
#if defined(DEBUG_ESPUI)
    if (ESPUI.verbosity)
    {
        Serial.printf_P(PSTR("Writing file: %s\n"), path);
    }
#endif

	File file = EspuiLittleFS.open(path, FILE_WRITING);
	if (!file)
	{
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("Failed to open file for writing"));
        }
#endif

		return;
	}

	if (file.print(data))
	{
#if defined(DEBUG_ESPUI)
        if (ESPUI.verbosity)
        {
            Serial.println(F("File written"));
        }
    }
    else
    {
        if (ESPUI.verbosity)
        {
            Serial.println(F("Write failed"));
        }
#endif
	}
	file.close();
}

// end LITTLEFS functions

void ESPUIClass::prepareFileSystem(const bool format) const
{
	// this function should only be used once

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("About to prepare filesystem..."));
    }
#endif

	if (!EspuiLittleFS.begin(false)) // Test for an already formatted LittleFS by a mount failure
	{
		if (!EspuiLittleFS.begin(true)) // Attempt to format LittleFS
		{
#if defined(DEBUG_ESPUI)
            if (verbosity)
            {
                Serial.println(F("LittleFS Format Failed"));
            }
#endif
			return;
		}
	} else if (format)
	{
		EspuiLittleFS.format();

#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("LittleFS Formatted"));
        }
#endif
	}

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        listDir("/", 1);
        Serial.println(F("LittleFS Mount ESP32 Done"));
    }
#endif

	deleteFile("/index.htm");

	deleteFile("/css/style.css");
	deleteFile("/css/normalize.css");

	deleteFile("/js/zepto.min.js");
	deleteFile("/js/controls.js");
	deleteFile("/js/slider.js");
	deleteFile("/js/graph.js");
	deleteFile("/js/tabbedcontent.js");

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("Cleanup done"));
    }
#endif

	// Now write
	writeFile("/index.htm", HTML_INDEX);
	EspuiLittleFS.mkdir("/css");
	writeFile("/css/style.css", CSS_STYLE);
	writeFile("/css/normalize.css", CSS_NORMALIZE);
	EspuiLittleFS.mkdir("/js");
	writeFile("/js/zepto.min.js", JS_ZEPTO);
	writeFile("/js/controls.js", JS_CONTROLS);
	writeFile("/js/slider.js", JS_SLIDER);
	writeFile("/js/graph.js", JS_GRAPH);

	writeFile("/js/tabbedcontent.js", JS_TABBEDCONTENT);

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("Done Initializing filesystem :-)"));
    }

    if (verbosity)
    {
        listDir("/", 1);
    }
#endif

	EspuiLittleFS.end();
}

// Handle Websockets Communication
void ESPUIClass::onWsEvent(
	AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsEventType type, void *arg, const uint8_t *data,
	const size_t len)
{
	// Serial.println(String("ESPUIClass::OnWsEvent: type: ") + String(type));
	RemoveToBeDeletedControls();

	if (WS_EVT_DISCONNECT == type)
	{
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("WS_EVT_DISCONNECT"));
        }
#endif

		if (MapOfClients.end() != MapOfClients.find(client->id()))
		{
			// Serial.println("Delete client.");
			delete MapOfClients[client->id()];
			MapOfClients.erase(client->id());
		}
	} else
	{
		if (type == WS_EVT_CONNECT)
		{
			ws->cleanupClients();
		}

		if (MapOfClients.end() == MapOfClients.find(client->id()))
		{
			// Serial.println("ESPUIClass::OnWsEvent:Create new client.");
			MapOfClients[client->id()] = new ESPUIclient(client);
		}

		if (MapOfClients[client->id()]->onWsEvent(type, arg, data, len))
		{
			// Serial.println("ESPUIClass::OnWsEvent:notify the clients that they need to be updated.");
			NotifyClients(ESPUIclient::UpdateNeeded);
		}
	}
}

uint16_t ESPUIClass::addControl(const ControlType type, const char *label)
{
	return addControl(type, label, String(""));
}

uint16_t ESPUIClass::addControl(const ControlType type, const char *label, const String &value)
{
	return addControl(type, label, value, ControlColor::Turquoise);
}

uint16_t ESPUIClass::addControl(const ControlType type, const char *label, const String &value,
                                const ControlColor color)
{
	return addControl(type, label, value, color, Control::noParent);
}

uint16_t ESPUIClass::addControl(
	const ControlType type, const char *label, const String &value, const ControlColor color,
	const uint16_t parentControl)
{
	return addControl(type, label, value, color, parentControl,
	                  new Control(type, label, nullptr, value, color, true, parentControl));
}

uint16_t ESPUIClass::addControl(const ControlType type, const char *label, const String &value,
                                const ControlColor color,
                                const uint16_t parentControl, const std::function<void(Control *, int)> &callback)
{
	const uint16_t id = addControl(type, label, value, color, parentControl);
	// set the original style callback
	getControl(id)->callback = callback;
	return id;
}

uint16_t ESPUIClass::addControl(
	ControlType type, const char *label, const String &value, ControlColor color, uint16_t parentControl,
	Control *control)
{
	xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);

	if (controls == nullptr)
	{
		controls = control;
	} else
	{
		Control *iterator = controls;

		while (iterator->next != nullptr)
		{
			iterator = iterator->next;
		}

		iterator->next = control;
	}

	controlCount++;

	xSemaphoreGive(ControlsSemaphore);

	NotifyClients(ESPUIclient::ClientUpdateType_t::RebuildNeeded);

	if (control)
		return control->id;

	return 0;
}

bool ESPUIClass::removeControl(const uint16_t id, const bool force_rebuild_ui)
{
	bool Response = false;

	Control *control = getControl(id);
	if (control)
	{
		Response = true;
		control->DeleteControl();
		controlCount--;

		if (force_rebuild_ui)
		{
			jsonReload();
		} else
		{
			NotifyClients(ESPUIclient::ClientUpdateType_t::RebuildNeeded);
		}
	}
#ifdef DEBUG_ESPUI
    else
    {
        // Serial.println(String("Could not Remove Control ") + String(id));
    }
#endif // def DEBUG_ESPUI

	return Response;
}

void ESPUIClass::RemoveToBeDeletedControls()
{
	xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);

	Control *PreviousControl = nullptr;
	Control *CurrentControl = controls;

	while (nullptr != CurrentControl)
	{
		Control *NextControl = CurrentControl->next;
		if (CurrentControl->ToBeDeleted())
		{
			if (CurrentControl == controls)
			{
				// this is the root control
				controls = NextControl;
			} else if (PreviousControl != nullptr)
			{
				PreviousControl->next = NextControl;
			}
			delete CurrentControl;
			CurrentControl = NextControl;
		} else
		{
			PreviousControl = CurrentControl;
			CurrentControl = NextControl;
		}
	}

	xSemaphoreGive(ControlsSemaphore);
}

uint16_t ESPUIClass::label(const char *label, const ControlColor color, const String &value)
{
	return addControl(ControlType::Label, label, value, color);
}

uint16_t ESPUIClass::graph(const char *label, const ControlColor color)
{
	return addControl(ControlType::Graph, label, "", color);
}

uint16_t ESPUIClass::slider(
	const char *label, const std::function<void(Control *, int)> &callback, const ControlColor color, const int value,
	const int min, const int max)
{
	const uint16_t sliderId
			= addControl(ControlType::Slider, label, String(value), color, Control::noParent, callback);
	addControl(ControlType::Min, label, String(min), ControlColor::None, sliderId);
	addControl(ControlType::Max, label, String(max), ControlColor::None, sliderId);
	return sliderId;
}

uint16_t ESPUIClass::button(const char *label, const std::function<void(Control *, int)> &callback,
                            const ControlColor color,
                            const String &value)
{
	return addControl(ControlType::Button, label, value, color, Control::noParent, callback);
}

uint16_t ESPUIClass::switcher(const char *label, const std::function<void(Control *, int)> &callback,
                              const ControlColor color,
                              const bool startState)
{
	return addControl(ControlType::Switcher, label, startState ? "1" : "0", color, Control::noParent, callback);
}

uint16_t ESPUIClass::pad(const char *label, const std::function<void(Control *, int)> &callback,
                         const ControlColor color)
{
	return addControl(ControlType::Pad, label, "", color, Control::noParent, callback);
}

uint16_t ESPUIClass::padWithCenter(const char *label, const std::function<void(Control *, int)> &callback,
                                   const ControlColor color)
{
	return addControl(ControlType::PadWithCenter, label, "", color, Control::noParent, callback);
}

uint16_t ESPUIClass::number(
	const char *label, const std::function<void(Control *, int)> &callback, const ControlColor color, const int number,
	const int min, const int max)
{
	const uint16_t numberId = addControl(ControlType::Number, label, String(number), color, Control::noParent,
	                                     callback);
	addControl(ControlType::Min, label, String(min), ControlColor::None, numberId);
	addControl(ControlType::Max, label, String(max), ControlColor::None, numberId);
	return numberId;
}

uint16_t ESPUIClass::gauge(const char *label, const ControlColor color, const int number, const int min, const int max)
{
	const uint16_t numberId = addControl(ControlType::Gauge, label, String(number), color, Control::noParent);
	addControl(ControlType::Min, label, String(min), ControlColor::None, numberId);
	addControl(ControlType::Max, label, String(max), ControlColor::None, numberId);
	return numberId;
}

void ESPUIClass::separator(const char *label)
{
	addControl(ControlType::Separator, label, "", ControlColor::Alizarin, Control::noParent, nullptr);
}

uint16_t ESPUIClass::fileDisplay(const char *label, const ControlColor color, const String &filename)
{
	return addControl(ControlType::FileDisplay, label, filename, color, Control::noParent);
}

uint16_t ESPUIClass::accelerometer(const char *label, const std::function<void(Control *, int)> &callback,
                                   const ControlColor color)
{
	return addControl(ControlType::Accel, label, "", color, Control::noParent, callback);
}

uint16_t ESPUIClass::text(const char *label, const std::function<void(Control *, int)> &callback,
                          const ControlColor color,
                          const String &value)
{
	return addControl(ControlType::Text, label, value, color, Control::noParent, callback);
}

Control *ESPUIClass::getControl(const uint16_t id) const
{
	xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
	Control *Response = getControlNoLock(id);
	xSemaphoreGive(ControlsSemaphore);
	return Response;
}

// WARNING: Anytime you walk the chain of controllers, the protection semaphore
//          MUST be locked. This function assumes that the semaphore is locked
//          at the time it is called. Make sure YOU locked it :)
Control *ESPUIClass::getControlNoLock(const uint16_t id) const
{
	Control *Response = nullptr;
	Control *control = controls;

	while (nullptr != control)
	{
		if (control->id == id)
		{
			if (!control->ToBeDeleted())
			{
				Response = control;
			}
			break;
		}
		control = control->next;
	}

	return Response;
}

void ESPUIClass::updateControl(Control *control, int) const
{
	if (!control)
	{
		return;
	}
	// tell the control it has been updated
	control->SetControlChangedId(ESPUI.GetNextControlChangeId());
	NotifyClients(ESPUIclient::ClientUpdateType_t::UpdateNeeded);
}

uint32_t ESPUIClass::GetNextControlChangeId()
{
	if (static_cast<uint32_t>(-1) == ControlChangeID)
	{
		// force a reload which resets the counters
		jsonReload();
	}
	return ++ControlChangeID;
}

void ESPUIClass::setPanelStyle(const uint16_t id, const String &style, const int clientId) const
{
	Control *control = getControl(id);
	if (control)
	{
		control->panelStyle = style;
		updateControl(control, clientId);
	}
}

void ESPUIClass::setElementStyle(const uint16_t id, const String &style, const int clientId) const
{
	Control *control = getControl(id);
	if (control)
	{
		control->elementStyle = style;
		updateControl(control, clientId);
	}
}

void ESPUIClass::setInputType(const uint16_t id, const String &type, const int clientId) const
{
	Control *control = getControl(id);
	if (control)
	{
		control->inputType = type;
		updateControl(control, clientId);
	}
}

void ESPUIClass::setPanelWide(const uint16_t id, const bool wide) const
{
	Control *control = getControl(id);
	if (control)
	{
		control->wide = wide;
	}
}

void ESPUIClass::setEnabled(const uint16_t id, const bool enabled, const int clientId) const
{
	Control *control = getControl(id);
	if (control)
	{
		// Serial.println(String("CreateAllowed: id: ") + String(clientId) + " State: " + String(enabled));
		control->enabled = enabled;
		updateControl(control, clientId);
	}
}

void ESPUIClass::setVertical(const uint16_t id, const bool vert) const
{
	Control *control = getControl(id);
	if (control)
	{
		control->vertical = vert;
	}
}

void ESPUIClass::updateControl(uint16_t id, const int clientId) const
{
	Control *control = getControl(id);

	if (!control)
	{
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.printf_P(PSTR("Error: Update Control: There is no control with ID %d\n"), id);
        }
#endif
		return;
	}

	updateControl(control, clientId);
}

void ESPUIClass::updateControlValue(Control *control, const String &value, const int clientId) const
{
	if (!control)
	{
		return;
	}

	control->value = value;
	updateControl(control, clientId);
}

void ESPUIClass::updateControlValue(uint16_t id, const String &value, const int clientId) const
{
	Control *control = getControl(id);

	if (!control)
	{
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.printf_P(PSTR("Error: updateControlValue Control: There is no control with ID %d\n"), id);
        }
#endif
		return;
	}

	updateControlValue(control, value, clientId);
}

void ESPUIClass::updateControlLabel(const uint16_t id, const char *value, const int clientId) const
{
	updateControlLabel(getControl(id), value, clientId);
}

void ESPUIClass::updateControlLabel(Control *control, const char *value, const int clientId) const
{
	if (!control)
	{
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.printf_P(PSTR("Error: updateControlLabel Control: There is no control with the requested ID \n"));
        }
#endif
		return;
	}
	control->label = value;
	updateControl(control, clientId);
}

void ESPUIClass::updateVisibility(const uint16_t id, const bool visibility, const int clientId) const
{
	Control *control = getControl(id);
	if (control)
	{
		control->visible = visibility;
		updateControl(control, clientId);
	}
}

void ESPUIClass::print(const uint16_t id, const String &value) const
{
	updateControlValue(id, value);
}

void ESPUIClass::updateLabel(const uint16_t id, const String &value) const
{
	updateControlValue(id, value);
}

void ESPUIClass::updateButton(const uint16_t id, const String &value) const
{
	updateControlValue(id, value);
}

void ESPUIClass::updateSlider(const uint16_t id, const int nValue, const int clientId) const
{
	updateControlValue(id, String(nValue), clientId);
}

void ESPUIClass::updateSwitcher(const uint16_t id, const bool nValue, const int clientId) const
{
	updateControlValue(id, String(nValue ? "1" : "0"), clientId);
}

void ESPUIClass::updateNumber(const uint16_t id, const int number, const int clientId) const
{
	updateControlValue(id, String(number), clientId);
}

void ESPUIClass::updateText(const uint16_t id, const String &text, const int clientId) const
{
	updateControlValue(id, text, clientId);
}

void ESPUIClass::updateSelect(const uint16_t id, const String &text, const int clientId) const
{
	updateControlValue(id, text, clientId);
}

void ESPUIClass::updateGauge(const uint16_t id, const int number, const int clientId) const
{
	updateControlValue(id, String(number), clientId);
}

void ESPUIClass::updateTime(const uint16_t id, const int clientId) const
{
	updateControl(id, clientId);
}

void ESPUIClass::clearGraph(const uint16_t id, const int clientId)
{
	do // once
	{
		const Control *control = getControl(id);
		if (!control)
		{
			break;
		}

		AllocateJsonDocument(document, jsonUpdateDocumentSize);
		const JsonObject root = document.to<JsonObject>();

		root[F("type")] = static_cast<int>(ControlType::Graph) + static_cast<int>(ControlType::UpdateOffset);
		root[F("value")] = 0;
		root[F("id")] = control->id;

		SendJsonDocToWebSocket(document, clientId);
	} while (false);
}

void ESPUIClass::addGraphPoint(const uint16_t id, const int nValue, const int clientId)
{
	do // once
	{
		const Control *control = getControl(id);
		if (!control)
		{
			break;
		}

		AllocateJsonDocument(document, jsonUpdateDocumentSize);
		const JsonObject root = document.to<JsonObject>();

		root[F("type")] = static_cast<int>(ControlType::GraphPoint);
		root[F("value")] = nValue;
		root[F("id")] = control->id;

		SendJsonDocToWebSocket(document, clientId);
	} while (false);
}

bool ESPUIClass::SendJsonDocToWebSocket(const ArduinoJson::JsonDocument &document, const uint16_t clientId)
{
	bool Response = false;

	if (0 > clientId)
	{
		if (MapOfClients.end() != MapOfClients.find(clientId))
		{
			Response = MapOfClients[clientId]->SendJsonDocToWebSocket(document);
		}
	} else
	{
		for (const auto CurrentClient: MapOfClients)
		{
			Response |= CurrentClient.second->SendJsonDocToWebSocket(document);
		}
	}

	return Response;
}

void ESPUIClass::jsonDom(uint16_t, AsyncWebSocketClient *, bool) const
{
	NotifyClients(ESPUIclient::ClientUpdateType_t::RebuildNeeded);
}

// Tell all the clients that they need to ask for an upload of the control data.
void ESPUIClass::NotifyClients(const ESPUIclient::ClientUpdateType_t newState) const
{
	for (const auto &CurrentClient: MapOfClients)
	{
		CurrentClient.second->NotifyClient(newState);
	}
}

void ESPUIClass::jsonReload() const
{
	for (const auto &CurrentClient: MapOfClients)
	{
		// Serial.println("Requesting Reload");
		CurrentClient.second->NotifyClient(ESPUIclient::ClientUpdateType_t::ReloadNeeded);
	}
}

void ESPUIClass::beginSPIFFS(const char *_title, const char *username, const char *password, const uint16_t port)
{
	// Backwards compatibility wrapper
	beginLITTLEFS(_title, username, password, port);
}

void ESPUIClass::beginLITTLEFS(const char *_title, const char *username, const char *password, const uint16_t port)
{
	ui_title = _title;
	basicAuthUsername = username;
	basicAuthPassword = password;

	if (username == nullptr && password == nullptr)
	{
		basicAuth = false;
	} else
	{
		basicAuth = true;
	}

	server = new AsyncWebServer(port);
	ws = new AsyncWebSocket("/ws");

	const bool fsBegin = EspuiLittleFS.begin();
	if (!fsBegin)
	{
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("LITTLEFS Mount Failed, PLEASE CHECK THE README ON HOW TO "
                             "PREPARE YOUR ESP!!!!!!!"));
        }
#endif

		return;
	}

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        listDir("/", 1);
    }
#endif

	const bool indexExists = EspuiLittleFS.exists("/index.htm");
	if (!indexExists)
	{
#if defined(DEBUG_ESPUI)
        if (verbosity)
        {
            Serial.println(F("Please read the README!!!!!!!, Make sure to "
                             "prepareFileSystem() once in an empty sketch"));
        }
#endif

		return;
	}

	ws->onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsEventType type, void *arg,
	               const uint8_t *data,
	               const size_t len)
	{
		ESPUI.onWsEvent(server, client, type, arg, data, len);
	});
	server->addHandler(ws);

	if (basicAuth)
	{
#if (WS_AUTHENTICATION)
		ws->setAuthentication(basicAuthUsername, basicAuthPassword);
#endif
		server->serveStatic("/", EspuiLittleFS, "/").setDefaultFile("index.htm").setAuthentication(username, password);
	} else
	{
		server->serveStatic("/", EspuiLittleFS, "/").setDefaultFile("index.htm");
	}

	// Heap for general Servertest
	server->on("/heap", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		request->send(200, "text/plain", heapInfo(F("In LITTLEFS mode")));
	});

	server->onNotFound([this](AsyncWebServerRequest *request)
	{
		if (captivePortal)
		{
			request->redirect("/");
		} else
		{
			request->send(404);
		}
	});

	server->begin();

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("UI Initialized"));
    }
#endif
}

void ESPUIClass::begin(const char *_title, const char *username, const char *password, const uint16_t port)
{
	basicAuthUsername = username;
	basicAuthPassword = password;

	if (username != nullptr && password != nullptr)
	{
		basicAuth = true;
	} else
	{
		basicAuth = false;
	}

	ui_title = _title;

	server = new AsyncWebServer(port);
	ws = new AsyncWebSocket("/ws");

	ws->onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsEventType type, void *arg,
	               const uint8_t *data,
	               const size_t len)
	{
		ESPUI.onWsEvent(server, client, type, arg, data, len);
	});

	server->addHandler(ws);

#if (WS_AUTHENTICATION)
	if (basicAuth)
		ws->setAuthentication(username, password);
#endif

	server->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_INDEX);
		request->send(response);
	});

	// Javascript files

	server->on("/js/zepto.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "application/javascript", JS_ZEPTO_GZIP, sizeof(JS_ZEPTO_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/js/controls.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "application/javascript", JS_CONTROLS_GZIP, sizeof(JS_CONTROLS_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/js/slider.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "application/javascript", JS_SLIDER_GZIP, sizeof(JS_SLIDER_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/js/graph.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "application/javascript", JS_GRAPH_GZIP, sizeof(JS_GRAPH_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/js/tabbedcontent.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		AsyncWebServerResponse *response = request->beginResponse_P(
			200, "application/javascript", JS_TABBEDCONTENT_GZIP, sizeof(JS_TABBEDCONTENT_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	// Stylesheets

	server->on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "text/css", CSS_STYLE_GZIP, sizeof(CSS_STYLE_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/css/normalize.css", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "text/css", CSS_NORMALIZE_GZIP, sizeof(CSS_NORMALIZE_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	// Heap for general Servertest
	server->on("/heap", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
		{
			return request->requestAuthentication();
		}

		request->send(200, "text/plain", heapInfo(F("In Memorymode")));
	});

	server->onNotFound([this](AsyncWebServerRequest *request)
	{
		if (captivePortal)
		{
			AsyncResponseStream *response = request->beginResponseStream("text/html");
			String responseText;
			responseText.reserve(1024);
			responseText += F("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
			responseText += ("<p>If site does not re-direct click here <a href='http://" + WiFi.softAPIP().toString() +
			                 "'>this link</a></p>");
			responseText += (R"(</body></html><head><meta http-equiv="Refresh" content="0; URL='http://)" + WiFi.
			                 softAPIP().toString() + "'\" /></head>");
			response->write(responseText.c_str(), responseText.length());
			request->send(response);
		} else
		{
			request->send(404);
		}
		yield();
	});

	server->begin();

#if defined(DEBUG_ESPUI)
    if (verbosity)
    {
        Serial.println(F("UI Initialized"));
    }
#endif
}

void ESPUIClass::setVerbosity(const Verbosity v)
{
	verbosity = v;
}

ESPUIClass ESPUI;
