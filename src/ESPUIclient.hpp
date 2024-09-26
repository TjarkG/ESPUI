#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "ESPUIclientFsm.hpp"


class ESPUIClass;

class esp_ui_client final
{
	uint32_t CurrentSyncID = 0;
	uint32_t NextSyncID = 0;
	ESPUIClass &ui;

	static constexpr auto sliderContinuous {false};
	static constexpr auto jsonUpdateDocumentSize {2000};
	static constexpr auto jsonInitialDocumentSize {8000};

public:
	enum ClientUpdateType_t
	{
		// this is an ordered list. highest number is the highest priority
		Synchronized = 0,
		UpdateNeeded = 1,
		RebuildNeeded = 2,
		ReloadNeeded = 3,
	};

protected:
	// bool HasBeenNotified      = false;  // Set when a notification has been sent, and we are waiting for a reply
	// bool DelayedNotification  = false;  // set if a delayed notification is needed

	ClientUpdateType_t ClientUpdateType = ClientUpdateType_t::RebuildNeeded;

	AsyncWebSocketClient *client = nullptr;

	friend class fsm_EspuiClient_state_Idle;
	friend class fsm_EspuiClient_state_SendingUpdate;
	friend class fsm_EspuiClient_state_Rebuilding;
	friend class fsm_EspuiClient_state_WaitForAck;
	friend class fsm_EspuiClient_state_Reloading;
	friend class fsm_EspuiClient_state;

	fsm_EspuiClient_state_Idle fsm_EspuiClient_state_Idle_imp;
	fsm_EspuiClient_state_SendingUpdate fsm_EspuiClient_state_SendingUpdate_imp;
	fsm_EspuiClient_state_Rebuilding fsm_EspuiClient_state_Rebuilding_imp;
	fsm_EspuiClient_state_Reloading fsm_EspuiClient_state_Reloading_imp;
	fsm_EspuiClient_state *pCurrentFsmState = &fsm_EspuiClient_state_Idle_imp;

	time_t EspuiClientEndTime = 0;

	// bool        NeedsNotification() { return pCurrentFsmState != &fsm_EspuiClient_state_Idle_imp; }

	bool CanSend() const;

	void FillInHeader(JsonDocument &document) const;

	uint32_t prepareJSONChunk(uint16_t startindex, JsonDocument &rootDoc, bool InUpdateMode, const String &value) const;

	bool SendControlsToClient(uint16_t start_idx, ClientUpdateType_t TransferMode, const String &FragmentRequest);

	bool SendClientNotification(ClientUpdateType_t value) const;

public:
	esp_ui_client(AsyncWebSocketClient *client, ESPUIClass &ui);

	esp_ui_client(const esp_ui_client &source);

	~esp_ui_client();

	void NotifyClient(ClientUpdateType_t value);

	bool onWsEvent(AwsEventType type, void *arg, const uint8_t *data, size_t len);

	bool IsSynchronized() const;

	uint32_t id() const { return client->id(); }

	void SetState(ClientUpdateType_t value);

	bool SendJsonDocToWebSocket(const JsonDocument &document) const;
};
