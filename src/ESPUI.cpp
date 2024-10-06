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
void ESPUIClass::onWsEvent(AsyncWebSocketClient *client, const AwsEventType type, void *arg, const uint8_t *data,
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

std::shared_ptr<Control> ESPUIClass::add(const ControlType type,
                                         const std::string &label,
                                         const std::string &value,
                                         const ControlColor color,
                                         const std::function<void(Control *, UpdateType)> &callback)
{
	const Control control = {type, label, callback, value, color, true, nullptr, *this};

	return addControl(control);
}

void ESPUIClass::removeControl(const std::shared_ptr<Control> &control, const bool force_rebuild_ui)
{
	xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
	const auto it = std::find(controls.begin(), controls.end(), control);
	controls.erase(it);
	xSemaphoreGive(ControlsSemaphore);

	if (force_rebuild_ui)
		jsonReload();
	else
		NotifyClients(ClientUpdateType_t::RebuildNeeded);
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

void ESPUIClass::updateControl(Control &control) const
{
	// tell the control it has been updated
	control.SetControlChangedId(GetNextControlChangeId());
	NotifyClients(ClientUpdateType_t::UpdateNeeded);
}

uint32_t ESPUIClass::GetNextControlChangeId() const
{
	static uint32_t ControlChangeID = 0;

	// force a reload which resets the counters
	if (static_cast<uint32_t>(-1) == ControlChangeID)
		jsonReload();

	return ++ControlChangeID;
}

void ESPUIClass::setPanelStyle(Control &control, const std::string &style) const
{
	control.panelStyle = style;
	updateControl(control);
}

void ESPUIClass::setElementStyle(Control &control, const std::string &style) const
{
	control.elementStyle = style;
	updateControl(control);
}

void ESPUIClass::setInputType(Control &control, const std::string &type) const
{
	control.inputType = type;
	updateControl(control);
}

void ESPUIClass::setPanelWide(Control &control, const bool wide) const
{
	control.wide = wide;
	updateControl(control);
}

void ESPUIClass::setEnabled(Control &control, const bool enabled) const
{
	control.enabled = enabled;
	updateControl(control);
}

void ESPUIClass::setVertical(Control &control, const bool vert) const
{
	control.vertical = vert;
	updateControl(control);
}

void ESPUIClass::updateControlValue(Control &control, const std::string &value) const
{
	control.value = value;
	updateControl(control);
}

void ESPUIClass::updateControlLabel(Control &control, const std::string &value) const
{
	control.label = value;
	updateControl(control);
}

void ESPUIClass::updateVisibility(Control &control, const bool visibility) const
{
	control.visible = visibility;
	updateControl(control);
}

void ESPUIClass::print(Control &control, const std::string &value) const
{
	updateControlValue(control, value);
}

void ESPUIClass::updateLabel(Control &control, const std::string &value) const
{
	updateControlValue(control, value);
}

void ESPUIClass::updateButton(Control &control, const std::string &value) const
{
	updateControlValue(control, value);
}

void ESPUIClass::updateSlider(Control &control, const int nValue) const
{
	updateControlValue(control, std::to_string(nValue));
}

void ESPUIClass::updateSwitcher(Control &control, const bool nValue) const
{
	updateControlValue(control, std::string(nValue ? "1" : "0"));
}

void ESPUIClass::updateNumber(Control &control, const int number) const
{
	updateControlValue(control, std::to_string(number));
}

void ESPUIClass::updateText(Control &control, const std::string &nValue) const
{
	updateControlValue(control, nValue);
}

void ESPUIClass::updateSelect(Control &control, const std::string &nValue) const
{
	updateControlValue(control, nValue);
}

void ESPUIClass::updateGauge(Control &control, const int number) const
{
	updateControlValue(control, std::to_string(number));
}

void ESPUIClass::updateTime(Control &control) const
{
	updateControl(control);
}

void ESPUIClass::clearGraph(const Control &control) const
{
	JsonDocument document;
	const JsonObject root = document.to<JsonObject>();

	root[F("type")] = static_cast<int>(ControlType::Graph) + static_cast<int>(ControlType::UpdateOffset);
	root[F("value")] = 0;
	root[F("id")] = control.id;

	SendJsonDocToWebSocket(document);
}

void ESPUIClass::addGraphPoint(const Control &control, const int nValue) const
{
	JsonDocument document;
	const JsonObject root = document.to<JsonObject>();

	root[F("type")] = static_cast<int>(ControlType::GraphPoint);
	root[F("value")] = nValue;
	root[F("id")] = control.id;

	SendJsonDocToWebSocket(document);
}

void ESPUIClass::SendJsonDocToWebSocket(const JsonDocument &document) const
{
	for (const auto CurrentClient: MapOfClients)
		(void) CurrentClient.second->SendJsonDocToWebSocket(document);
}

// Tell all the clients that they need to ask for an upload of the control data.
void ESPUIClass::NotifyClients(const ClientUpdateType_t newState) const
{
	for (const auto &CurrentClient: MapOfClients)
		CurrentClient.second->NotifyClient(newState);
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

	ws->onEvent([this](AsyncWebSocket *, AsyncWebSocketClient *client, const AwsEventType type, void *arg,
	                   const uint8_t *data,
	                   const size_t len)
	{
		onWsEvent(client, type, arg, data, len);
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
