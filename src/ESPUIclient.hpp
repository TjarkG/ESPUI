#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>


class ESPUIClass;

enum class ClientUpdateType_t
{
	// this is an ordered list. highest number is the highest priority
	Synchronized = 0,
	UpdateNeeded = 1,
	RebuildNeeded = 2,
	ReloadNeeded = 3,
};

enum class ClientState_t
{
	// this is an ordered list. highest number is the highest priority
	Idle = 0,
	Sending = 1,
	Rebuilding = 2,
	Reloading = 3,
};

class esp_ui_client final
{
	uint32_t CurrentSyncID = 0;
	uint32_t NextSyncID = 0;
	ESPUIClass &ui;

	static constexpr auto sliderContinuous {false};
	static constexpr auto jsonUpdateDocumentSize {2000};
	static constexpr auto jsonInitialDocumentSize {8000};

	ClientUpdateType_t ClientUpdateType = ClientUpdateType_t::RebuildNeeded;

	AsyncWebSocketClient *client = nullptr;

	ClientState_t ClientState = ClientState_t::Idle;

	bool NotifyClient();

	void ProcessAck(uint16_t id, const String &FragmentRequest);

	time_t EspuiClientEndTime = 0;

	bool CanSend() const;

	void FillInHeader(JsonDocument &document) const;

	uint32_t prepareJSONChunk(JsonDocument &rootDoc, bool InUpdateMode, const String &value) const;

	bool SendControlsToClient(uint16_t start_idx, ClientUpdateType_t TransferMode, const String &FragmentRequest);

	bool SendClientNotification(ClientUpdateType_t value) const;

public:
	esp_ui_client(AsyncWebSocketClient *client, ESPUIClass &ui):
		ui(ui), client(client) {}

	esp_ui_client(const esp_ui_client &source):
		ui(source.ui), client(source.client) {}

	~esp_ui_client() = default;

	void NotifyClient(ClientUpdateType_t value);

	bool onWsEvent(AwsEventType type, void *arg, const uint8_t *data, size_t len);

	bool IsSynchronized() const;

	uint32_t id() const { return client->id(); }

	void SetState(ClientUpdateType_t value);

	bool SendJsonDocToWebSocket(const JsonDocument &document) const;
};
