#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>


class ESPUIClass;


// Message Types
enum MessageTypes : uint8_t
{
	InitialGui        = 200,
	Reload            = 201,
	ExtendGUI         = 210,
	UpdateGui         = 220,
	ExtendedUpdateGui = 230,
};

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

class WebsocketClient final
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

	void NotifyClient();

	void ProcessAck(uint16_t id, const std::string &FragmentRequest);

	time_t EspuiClientEndTime = 0;

	[[nodiscard]] bool CanSend() const;

	void FillInHeader(JsonDocument &document) const;

	uint32_t prepareJSONChunk(JsonDocument &rootDoc, bool InUpdateMode, const std::string &value) const;

	bool SendControlsToClient(uint16_t start_idx, ClientUpdateType_t TransferMode, const std::string &FragmentRequest);

	void SendClientNotification(ClientUpdateType_t value) const;

public:
	WebsocketClient(AsyncWebSocketClient *client, ESPUIClass &ui):
		ui(ui), client(client) {}

	WebsocketClient(const WebsocketClient &source):
		ui(source.ui), client(source.client) {}

	~WebsocketClient() = default;

	void NotifyClient(ClientUpdateType_t value);

	bool onWsEvent(AwsEventType type, void *arg, const uint8_t *data, size_t len);

	[[nodiscard]] bool IsSynchronized() const;

	[[nodiscard]] uint32_t id() const { return client->id(); }

	void SetState(ClientUpdateType_t value);

	void SendJsonDocToWebSocket(const JsonDocument &document) const;
};
