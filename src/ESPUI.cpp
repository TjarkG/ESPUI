#include "ESPUI.hpp"

#include <functional>

#include <ESPAsyncWebServer.h>

#include "data/ControlsJS.h"
#include "data/GraphJS.h"
#include "data/IndexHTML.h"
#include "data/NormalizeCSS.h"
#include "data/SliderJS.h"
#include "data/StyleCSS.h"
#include "data/TabbedcontentJS.h"
#include "data/ZeptoJS.h"

static std::string heapInfo(const __FlashStringHelper *mode)
{
	std::string result;

	result += std::to_string(ESP.getFreeHeap());
	result += ' ';

	result += std::to_string(reinterpret_cast<int>(mode));

	return result;
}

// Handle Websockets Communication
void ESPUIClass::onWsEvent(
	AsyncWebSocket *server, AsyncWebSocketClient *client, const AwsEventType type, void *arg, const uint8_t *data,
	const size_t len)
{
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
			NotifyClients(ClientUpdateType_t::UpdateNeeded);
	}
}

std::shared_ptr<Control> ESPUIClass::addControl(const Control &control)
{
	xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);

	controls.push_back(std::make_shared<Control>(control));

	xSemaphoreGive(ControlsSemaphore);

	NotifyClients(ClientUpdateType_t::RebuildNeeded);

	return controls.back();
}

std::shared_ptr<Control> ESPUIClass::addControl(
	const ControlType type, const std::string &label, const std::string &value, const ControlColor color,
	const std::shared_ptr<Control> &parentControl)
{
	const Control control = {type, label, nullptr, value, color, true, parentControl};

	return addControl(control);
}

std::shared_ptr<Control> ESPUIClass::addControl(const ControlType type, const std::string &label, const std::string &value,
                                                const ControlColor color,
                                                const std::shared_ptr<Control> &parentControl,
                                                const std::function<void(Control *, int)> &callback)
{
	auto control = addControl(type, label, value, color, parentControl);
	// set the original style callback
	control->callback = callback;
	return control;
}

void ESPUIClass::removeControl(const std::shared_ptr<Control> &control, const bool force_rebuild_ui)
{
	xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
	const auto it = std::find(controls.begin(), controls.end(), control);
	controls.erase(it);
	xSemaphoreGive(ControlsSemaphore);

	if (force_rebuild_ui)
	{
		jsonReload();
	} else
	{
		NotifyClients(ClientUpdateType_t::RebuildNeeded);
	}
}

std::shared_ptr<Control> ESPUIClass::getControl(const uint16_t id) const
{
	xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
	auto Response = getControlNoLock(id);
	xSemaphoreGive(ControlsSemaphore);
	return Response;
}

// WARNING: Anytime you walk the chain of controllers, the protection semaphore
//          MUST be locked. This function assumes that the semaphore is locked
//          at the time it is called. Make sure YOU locked it :)
std::shared_ptr<Control> ESPUIClass::getControlNoLock(const uint16_t id) const
{
	const auto it = std::find_if(controls.begin(), controls.end(),
	                             [id](const std::shared_ptr<Control> &i) { return i->id == id; });
	if (it == controls.end())
		return nullptr;

	return *it;
}

void ESPUIClass::updateControl(Control &control, int)
{
	// tell the control it has been updated
	control.SetControlChangedId(GetNextControlChangeId());
	NotifyClients(ClientUpdateType_t::UpdateNeeded);
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

void ESPUIClass::setPanelStyle(Control &control, const std::string &style, const int clientId)
{
	control.panelStyle = style;
	updateControl(control, clientId);
}

void ESPUIClass::setElementStyle(Control &control, const std::string &style, const int clientId)
{
	control.elementStyle = style;
	updateControl(control, clientId);
}

void ESPUIClass::setInputType(Control &control, const std::string &type, const int clientId)
{
	control.inputType = type;
	updateControl(control, clientId);
}

void ESPUIClass::setPanelWide(Control &control, const bool wide, const int clientId)
{
	control.wide = wide;
	updateControl(control, clientId);
}

void ESPUIClass::setEnabled(Control &control, const bool enabled, const int clientId)
{
	control.enabled = enabled;
	updateControl(control, clientId);
}

void ESPUIClass::setVertical(Control &control, const bool vert, const int clientId)
{
	control.vertical = vert;
	updateControl(control, clientId);
}

void ESPUIClass::updateControlValue(Control &control, const std::string &value, const int clientId)
{
	control.value = value;
	updateControl(control, clientId);
}

void ESPUIClass::updateControlLabel(Control &control, const std::string &value, const int clientId)
{
	control.label = value;
	updateControl(control, clientId);
}

void ESPUIClass::updateVisibility(Control &control, const bool visibility, const int clientId)
{
	control.visible = visibility;
	updateControl(control, clientId);
}

void ESPUIClass::print(Control &control, const std::string &value)
{
	updateControlValue(control, value);
}

void ESPUIClass::updateLabel(Control &control, const std::string &value)
{
	updateControlValue(control, value);
}

void ESPUIClass::updateButton(Control &control, const std::string &value)
{
	updateControlValue(control, value);
}

void ESPUIClass::updateSlider(Control &control, const int nValue, const int clientId)
{
	updateControlValue(control, std::to_string(nValue), clientId);
}

void ESPUIClass::updateSwitcher(Control &control, const bool nValue, const int clientId)
{
	updateControlValue(control, std::string(nValue ? "1" : "0"), clientId);
}

void ESPUIClass::updateNumber(Control &control, const int number, const int clientId)
{
	updateControlValue(control, std::to_string(number), clientId);
}

void ESPUIClass::updateText(Control &control, const std::string &nValue, const int clientId)
{
	updateControlValue(control, nValue, clientId);
}

void ESPUIClass::updateSelect(Control &control, const std::string &nValue, const int clientId)
{
	updateControlValue(control, nValue, clientId);
}

void ESPUIClass::updateGauge(Control &control, const int number, const int clientId)
{
	updateControlValue(control, std::to_string(number), clientId);
}

void ESPUIClass::updateTime(Control &control, const int clientId)
{
	updateControl(control, clientId);
}

void ESPUIClass::clearGraph(const Control &control, const int clientId)
{
	JsonDocument document;
	const JsonObject root = document.to<JsonObject>();

	root[F("type")] = static_cast<int>(ControlType::Graph) + static_cast<int>(ControlType::UpdateOffset);
	root[F("value")] = 0;
	root[F("id")] = control.id;

	SendJsonDocToWebSocket(document, clientId);
}

void ESPUIClass::addGraphPoint(const Control &control, const int nValue, const int clientId)
{
	JsonDocument document;
	const JsonObject root = document.to<JsonObject>();

	root[F("type")] = static_cast<int>(ControlType::GraphPoint);
	root[F("value")] = nValue;
	root[F("id")] = control.id;

	SendJsonDocToWebSocket(document, clientId);
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
	NotifyClients(ClientUpdateType_t::RebuildNeeded);
}

// Tell all the clients that they need to ask for an upload of the control data.
void ESPUIClass::NotifyClients(const ClientUpdateType_t newState) const
{
	for (const auto &CurrentClient: MapOfClients)
	{
		CurrentClient.second->NotifyClient(newState);
	}
}

void ESPUIClass::jsonReload() const
{
	for (const auto &CurrentClient: MapOfClients)
		CurrentClient.second->NotifyClient(ClientUpdateType_t::ReloadNeeded);
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
		request->send(200, "text/plain", heapInfo(F("In Memory Mode")).c_str());
	});

	server->onNotFound([this](AsyncWebServerRequest *request)
	{
		AsyncResponseStream *response = request->beginResponseStream("text/html");
		std::string responseText;
		responseText.reserve(1024);
		responseText += "<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>";
		responseText += ("<p>If site does not re-direct click here <a href='http://" + WiFi.softAPIP().toString() +
		                 "'>this link</a></p>").c_str();
		responseText += (R"(</body></html><head><meta http-equiv="Refresh" content="0; URL='http://)" + WiFi.softAPIP().
		                 toString() + "'\" /></head>").c_str();
		response->write(responseText.c_str(), responseText.length());
		request->send(response);
		yield();
	});

	server->begin();
}
