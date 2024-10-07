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
	//remove disconnected client
	if (type == WS_EVT_DISCONNECT)
		clients.erase(client->id());
	//handle connect and data events
	else
	{
		if (type == WS_EVT_CONNECT)
			ws->cleanupClients();

		//Add new client
		if (clients.end() == clients.find(client->id()))
			clients.emplace(client->id(), WebsocketClient {client, *this});

		if (clients.at(client->id()).onWsEvent(type, arg, data, len))
			NotifyClients(ClientUpdateType_t::UpdateNeeded);
	}
}

std::shared_ptr<Widget> ESPUIClass::getControl(const uint16_t id) const
{
	xSemaphoreTake(ControlsSemaphore, portMAX_DELAY);
	auto Response = root->find(id);
	xSemaphoreGive(ControlsSemaphore);
	return Response;
}

void ESPUIClass::updateControl(Widget &control)
{
	// tell the control it has been updated
	control.SetControlChangedId(GetNextControlChangeId());
	NotifyClients(ClientUpdateType_t::UpdateNeeded);
}

uint32_t ESPUIClass::GetNextControlChangeId()
{
	static uint32_t ControlChangeID = 0;

	// force a reload which resets the counters
	if (static_cast<uint32_t>(-1) == ControlChangeID)
		NotifyClients(ClientUpdateType_t::ReloadNeeded);

	return ++ControlChangeID;
}

void ESPUIClass::setPanelStyle(Widget &control, const std::string &style)
{
	control.panelStyle = style;
	updateControl(control);
}

void ESPUIClass::setElementStyle(Widget &control, const std::string &style)
{
	control.elementStyle = style;
	updateControl(control);
}

void ESPUIClass::setInputType(Widget &control, const std::string &type)
{
	control.inputType = type;
	updateControl(control);
}

void ESPUIClass::setPanelWide(Widget &control, const bool wide)
{
	control.wide = wide;
	updateControl(control);
}

void ESPUIClass::setEnabled(Widget &control, const bool enabled)
{
	control.enabled = enabled;
	updateControl(control);
}

void ESPUIClass::setVertical(Widget &control, const bool vert)
{
	control.vertical = vert;
	updateControl(control);
}

void ESPUIClass::updateControlValue(Widget &control, const std::string &value)
{
	control.value_l = value;
	updateControl(control);
}

void ESPUIClass::updateControlLabel(Widget &control, const std::string &value)
{
	control.label = value;
	updateControl(control);
}

void ESPUIClass::print(Widget &control, const std::string &value)
{
	updateControlValue(control, value);
}

void ESPUIClass::updateLabel(Widget &control, const std::string &value)
{
	updateControlValue(control, value);
}

void ESPUIClass::updateButton(Widget &control, const std::string &value)
{
	updateControlValue(control, value);
}

void ESPUIClass::updateSlider(Widget &control, const int nValue)
{
	updateControlValue(control, std::to_string(nValue));
}

void ESPUIClass::updateSwitcher(Widget &control, const bool nValue)
{
	updateControlValue(control, std::string(nValue ? "1" : "0"));
}

void ESPUIClass::updateNumber(Widget &control, const int number)
{
	updateControlValue(control, std::to_string(number));
}

void ESPUIClass::updateText(Widget &control, const std::string &nValue)
{
	updateControlValue(control, nValue);
}

void ESPUIClass::updateSelect(Widget &control, const std::string &nValue)
{
	updateControlValue(control, nValue);
}

void ESPUIClass::updateGauge(Widget &control, const int number)
{
	updateControlValue(control, std::to_string(number));
}

void ESPUIClass::updateTime(Widget &control)
{
	updateControl(control);
}

void ESPUIClass::clearGraph(const Widget &control) const
{
	JsonDocument document;
	const JsonObject root = document.to<JsonObject>();

	root[F("type")] = static_cast<int>(ControlType::Graph) + static_cast<int>(ControlType::UpdateOffset);
	root[F("value")] = 0;
	root[F("id")] = control.id;

	SendJsonDocToWebSocket(document);
}

void ESPUIClass::addGraphPoint(const Widget &control, const int nValue) const
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
	for (const auto &[id, client]: clients)
		(void) client.SendJsonDocToWebSocket(document);
}

// Tell all the clients that they need to ask for an upload of the control data.
void ESPUIClass::NotifyClients(const ClientUpdateType_t newState)
{
	for (auto &[id, client]: clients)
		client.NotifyClient(newState);
}

void ESPUIClass::begin(const char *_title, const uint16_t port)
{
	uiTitle = _title;

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
