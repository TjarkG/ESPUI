#include "ESPUI.hpp"

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

// LITTLEFS functions
void ESPUIClass::listDir(const char *dirname, const uint8_t levels)
{
	File root = EspuiLittleFS.open(dirname);
	if (!root)
		return;

	if (!root.isDirectory())
		return;

	File file = root.openNextFile();

	while (file)
	{
		if (file.isDirectory())
		{
			if (levels)
				listDir(file.path(), levels - 1);
		}

		file = root.openNextFile();
	}
}

void ESPUIClass::list()
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

void ESPUIClass::deleteFile(const char *path) const
{
	const bool exists = EspuiLittleFS.exists(path);
	if (!exists)
		return;

	EspuiLittleFS.remove(path);
}

void ESPUIClass::writeFile(const char *path, const char *data) const
{
	File file = EspuiLittleFS.open(path, FILE_WRITING);
	if (!file)
		return;

	file.print(data);
	file.close();
}

// end LITTLEFS functions

void ESPUIClass::prepareFileSystem(const bool format) const
{
	// this function should only be used once

	if (!EspuiLittleFS.begin(false)) // Test for an already formatted LittleFS by a mount failure
	{
		if (!EspuiLittleFS.begin(true)) // Attempt to format LittleFS
			return;
	} else if (format)
	{
		EspuiLittleFS.format();
	}

	deleteFile("/index.htm");

	deleteFile("/css/style.css");
	deleteFile("/css/normalize.css");

	deleteFile("/js/zepto.min.js");
	deleteFile("/js/controls.js");
	deleteFile("/js/slider.js");
	deleteFile("/js/graph.js");
	deleteFile("/js/tabbedcontent.js");

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

	EspuiLittleFS.end();
}

// Handle Websockets Communication
void ESPUIClass::onWsEvent(
	AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsEventType type, void *arg, const uint8_t *data,
	const size_t len)
{
	RemoveToBeDeletedControls();

	if (WS_EVT_DISCONNECT == type)
	{
		if (MapOfClients.end() != MapOfClients.find(client->id()))
		{
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
			MapOfClients[client->id()] = new esp_ui_client(client, *this);

		if (MapOfClients[client->id()]->onWsEvent(type, arg, data, len))
			NotifyClients(esp_ui_client::UpdateNeeded);
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

	NotifyClients(esp_ui_client::ClientUpdateType_t::RebuildNeeded);

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
			NotifyClients(esp_ui_client::ClientUpdateType_t::RebuildNeeded);
		}
	}

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
	addControl(ControlType::Separator, label, "", ControlColor::Red, Control::noParent, nullptr);
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

void ESPUIClass::updateControl(Control *control, int)
{
	if (!control)
	{
		return;
	}
	// tell the control it has been updated
	control->SetControlChangedId(GetNextControlChangeId());
	NotifyClients(esp_ui_client::ClientUpdateType_t::UpdateNeeded);
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

void ESPUIClass::setPanelStyle(const uint16_t id, const String &style, const int clientId)
{
	Control *control = getControl(id);
	if (control)
	{
		control->panelStyle = style;
		updateControl(control, clientId);
	}
}

void ESPUIClass::setElementStyle(const uint16_t id, const String &style, const int clientId)
{
	Control *control = getControl(id);
	if (control)
	{
		control->elementStyle = style;
		updateControl(control, clientId);
	}
}

void ESPUIClass::setInputType(const uint16_t id, const String &type, const int clientId)
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

void ESPUIClass::setEnabled(const uint16_t id, const bool enabled, const int clientId)
{
	Control *control = getControl(id);
	if (control)
	{
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

void ESPUIClass::updateControl(uint16_t id, const int clientId)
{
	Control *control = getControl(id);

	if (!control)
		return;

	updateControl(control, clientId);
}

void ESPUIClass::updateControlValue(Control *control, const String &value, const int clientId)
{
	if (!control)
		return;

	control->value = value;
	updateControl(control, clientId);
}

void ESPUIClass::updateControlValue(uint16_t id, const String &value, const int clientId)
{
	Control *control = getControl(id);

	if (!control)
		return;

	updateControlValue(control, value, clientId);
}

void ESPUIClass::updateControlLabel(const uint16_t id, const char *value, const int clientId)
{
	updateControlLabel(getControl(id), value, clientId);
}

void ESPUIClass::updateControlLabel(Control *control, const char *value, const int clientId)
{
	if (!control)
		return;
	control->label = value;
	updateControl(control, clientId);
}

void ESPUIClass::updateVisibility(const uint16_t id, const bool visibility, const int clientId)
{
	Control *control = getControl(id);
	if (control)
	{
		control->visible = visibility;
		updateControl(control, clientId);
	}
}

void ESPUIClass::print(const uint16_t id, const String &value)
{
	updateControlValue(id, value);
}

void ESPUIClass::updateLabel(const uint16_t id, const String &value)
{
	updateControlValue(id, value);
}

void ESPUIClass::updateButton(const uint16_t id, const String &value)
{
	updateControlValue(id, value);
}

void ESPUIClass::updateSlider(const uint16_t id, const int nValue, const int clientId)
{
	updateControlValue(id, String(nValue), clientId);
}

void ESPUIClass::updateSwitcher(const uint16_t id, const bool nValue, const int clientId)
{
	updateControlValue(id, String(nValue ? "1" : "0"), clientId);
}

void ESPUIClass::updateNumber(const uint16_t id, const int number, const int clientId)
{
	updateControlValue(id, String(number), clientId);
}

void ESPUIClass::updateText(const uint16_t id, const String &text, const int clientId)
{
	updateControlValue(id, text, clientId);
}

void ESPUIClass::updateSelect(const uint16_t id, const String &text, const int clientId)
{
	updateControlValue(id, text, clientId);
}

void ESPUIClass::updateGauge(const uint16_t id, const int number, const int clientId)
{
	updateControlValue(id, String(number), clientId);
}

void ESPUIClass::updateTime(const uint16_t id, const int clientId)
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
	NotifyClients(esp_ui_client::ClientUpdateType_t::RebuildNeeded);
}

// Tell all the clients that they need to ask for an upload of the control data.
void ESPUIClass::NotifyClients(const esp_ui_client::ClientUpdateType_t newState) const
{
	for (const auto &CurrentClient: MapOfClients)
	{
		CurrentClient.second->NotifyClient(newState);
	}
}

void ESPUIClass::jsonReload() const
{
	for (const auto &CurrentClient: MapOfClients)
		CurrentClient.second->NotifyClient(esp_ui_client::ClientUpdateType_t::ReloadNeeded);
}

void ESPUIClass::beginLITTLEFS(const char *_title, const uint16_t port)
{
	ui_title = _title;

	server = new AsyncWebServer(port);
	ws = new AsyncWebSocket("/ws");

	const bool fsBegin = EspuiLittleFS.begin();
	if (!fsBegin)
		return;


	const bool indexExists = EspuiLittleFS.exists("/index.htm");
	if (!indexExists)
		return;

	ws->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsEventType type, void *arg,
	                   const uint8_t *data,
	                   const size_t len)
	{
		onWsEvent(server, client, type, arg, data, len);
	});
	server->addHandler(ws);

	server->serveStatic("/", EspuiLittleFS, "/").setDefaultFile("index.htm");

	// Heap for general Servertest
	server->on("/heap", HTTP_GET, [](AsyncWebServerRequest *request)
	{
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
}

void ESPUIClass::begin(const char *_title, const uint16_t port)
{
	ui_title = _title;

	server = new AsyncWebServer(port);
	ws = new AsyncWebSocket("/ws");

	ws->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsEventType type, void *arg,
	                   const uint8_t *data,
	                   const size_t len)
	{
		onWsEvent(server, client, type, arg, data, len);
	});

	server->addHandler(ws);

	server->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_INDEX);
		request->send(response);
	});

	// Javascript files

	server->on("/js/zepto.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "application/javascript", JS_ZEPTO_GZIP, sizeof(JS_ZEPTO_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/js/controls.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "application/javascript", JS_CONTROLS_GZIP, sizeof(JS_CONTROLS_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/js/slider.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "application/javascript", JS_SLIDER_GZIP, sizeof(JS_SLIDER_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/js/graph.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "application/javascript", JS_GRAPH_GZIP, sizeof(JS_GRAPH_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/js/tabbedcontent.js", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		AsyncWebServerResponse *response = request->beginResponse_P(
			200, "application/javascript", JS_TABBEDCONTENT_GZIP, sizeof(JS_TABBEDCONTENT_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	// Stylesheets

	server->on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "text/css", CSS_STYLE_GZIP, sizeof(CSS_STYLE_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	server->on("/css/normalize.css", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		AsyncWebServerResponse *response
				= request->beginResponse_P(200, "text/css", CSS_NORMALIZE_GZIP, sizeof(CSS_NORMALIZE_GZIP));
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});

	// Heap for general Servertest
	server->on("/heap", HTTP_GET, [](AsyncWebServerRequest *request)
	{
		request->send(200, "text/plain", heapInfo(F("In Memory Mode")));
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
			request->send(404);
		yield();
	});

	server->begin();
}
