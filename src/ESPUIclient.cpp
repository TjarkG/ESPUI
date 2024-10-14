#include "ESPUIclient.hpp"
#include "ESPUI.hpp"
#include "Widgets/ESPUIcontrol.hpp"

void WebsocketClient::NotifyClient()
{
	if (ClientState != ClientState_t::Idle)
		return;

	// Clear the type so that we capture any changes in type that happen
	// while we are processing the current request.
	const auto TypeToProcess = ClientUpdateType;
	ClientUpdateType = ClientUpdateType_t::Synchronized;

	// Start processing the current request.
	switch (TypeToProcess)
	{
		case ClientUpdateType_t::Synchronized:
		{
			return;
		}
		case ClientUpdateType_t::UpdateNeeded:
		{
			ClientState = ClientState_t::Sending;
			SendClientNotification(ClientUpdateType_t::UpdateNeeded);
			return;
		}
		case ClientUpdateType_t::RebuildNeeded:
		{
			ClientState = ClientState_t::Rebuilding;
			SendClientNotification(ClientUpdateType_t::RebuildNeeded);
			return;
		}
		case ClientUpdateType_t::ReloadNeeded:
		{
			ClientState = ClientState_t::Reloading;
			SendClientNotification(ClientUpdateType_t::ReloadNeeded);
			return;
		}
	}
}

void WebsocketClient::ProcessAck(const uint16_t id, const std::string &FragmentRequest)
{
	switch (ClientState)
	{
		case ClientState_t::Idle:
			if (!FragmentRequest.empty())
				SendControlsToClient(id, ClientUpdateType_t::UpdateNeeded, FragmentRequest);
			else
			{
				// This is an unexpected request for control data from the browser
				// treat it as if it was a rebuild operation
				NotifyClient(ClientUpdateType_t::RebuildNeeded);
			}
			break;
		case ClientState_t::Sending:
			if (SendControlsToClient(id, ClientUpdateType_t::UpdateNeeded, FragmentRequest))
			{
				// No more data to send. Go back to idle or start next request
				ClientState = ClientState_t::Idle;
				NotifyClient();
			}
			break;
		case ClientState_t::Rebuilding:
			if (SendControlsToClient(id, ClientUpdateType_t::RebuildNeeded, FragmentRequest))
			{
				// No more data to send. Go back to idle or start next request
				ClientState = ClientState_t::Idle;
				NotifyClient();
			}
			break;
		case ClientState_t::Reloading:
			if (!FragmentRequest.empty())
				SendControlsToClient(id, ClientUpdateType_t::UpdateNeeded, FragmentRequest);
			break;
	}
}

bool WebsocketClient::CanSend() const
{
	if (client)
		return client->canSend();
	return false;
}

void WebsocketClient::FillInHeader(JsonDocument &document) const
{
	document["type"] = ExtendGUI;
	document["sliderContinuous"] = sliderContinuous;
	document["startindex"] = 0;
	document["totalcontrols"] = ui.root->getChildCount();
	const JsonArray items = document["controls"].to<JsonArray>();
	const JsonObject titleItem = items.add<JsonObject>();
	titleItem["type"] = static_cast<int>(ControlType::Title);
	titleItem["label"] = ui.uiTitle;
}

bool WebsocketClient::IsSynchronized() const
{
	return ClientUpdateType_t::Synchronized == ClientUpdateType && ClientState == ClientState_t::Idle;
}

void WebsocketClient::SendClientNotification(const ClientUpdateType_t value) const
{
	if (!CanSend())
		return;

	JsonDocument document;
	FillInHeader(document);
	if (ClientUpdateType_t::ReloadNeeded == value)
		document["type"] = static_cast<int>(Reload);
	// dont send any controls
	SendJsonDocToWebSocket(document);
}

void WebsocketClient::NotifyClient(const ClientUpdateType_t value)
{
	SetState(value);
	NotifyClient();
}

// Handle Websockets Communication
bool WebsocketClient::onWsEvent(const AwsEventType type, void *, const uint8_t *data, const size_t len)
{
	bool Response = false;

	switch (type)
	{
		case WS_EVT_CONNECT:
		{
			NotifyClient(ClientUpdateType_t::RebuildNeeded);
			break;
		}

		case WS_EVT_DATA:
		{
			std::string msg;
			msg.reserve(len + 1);

			for (size_t i = 0; i < len; i++)
				msg += static_cast<char>(data[i]);
			const auto delim1 {msg.find(':')};
			const auto delim2 {msg.find_last_of(':')};

			const std::string cmd = msg.substr(0, delim1);
			const std::string value = msg.substr(delim1 + 1, delim2 - delim1 - 1);
			const uint16_t id = std::stoi(msg.substr(delim2 + 1));

			if (cmd == "uiok")
			{
				ProcessAck(id, "");
				break;
			}

			if (cmd == "uifragmentok")
			{
				if (!value.empty())
					ProcessAck(0xFFFF, value);
				else
					Serial.println(
						"ERROR:WebsocketClient::OnWsEvent:WS_EVT_DATA:uifragmentok:ProcessAck:Fragment Header is missing");
				break;
			}

			if (cmd == "uiuok")
				break;

			const auto control = ui.getControl(id);
			if (nullptr == control)
				break;
			control->onWsEvent(cmd, value, ui);
			// notify other clients of change
			Response = true;
			break;
		}

		default:
			break;
	} // end switch

	return Response;
}

/*
Prepare a chunk of elements as a single JSON string. If the allowed number of elements is greater than the total
number this will represent the entire UI. More likely, it will represent a small section of the UI to be sent. The
client will acknowledge receipt by requesting the next chunk.
 */
uint32_t WebsocketClient::prepareJSONChunk(JsonDocument &rootDoc, const bool InUpdateMode,
                                           const std::string &value) const
{
	xSemaphoreTake(ui.ControlsSemaphore, portMAX_DELAY);

	// Follow the list until control points to the startindex node
	const JsonArray items = rootDoc["controls"];

	if (!value.empty())
	{
		// this is actually a fragment or directed update request
		// parse the string we got from the UI and try to update that specific
		// control.
		JsonDocument FragmentRequest;
		const auto FragmentRequestStartOffset = value.find('{');
		const DeserializationError error =
				deserializeJson(FragmentRequest, value.substr(FragmentRequestStartOffset));
		if (DeserializationError::Ok != error)
		{
			Serial.println("ERROR:prepareJSONChunk:Fragmentation:Could not extract json from the fragment request");
			xSemaphoreGive(ui.ControlsSemaphore);
			return 0;
		}

		if (!FragmentRequest["id"].is<std::string>())
		{
			Serial.println("ERROR:prepareJSONChunk:Fragmentation:Request does not contain a control ID");
			xSemaphoreGive(ui.ControlsSemaphore);
			return 0;
		}
		const auto ControlId = FragmentRequest["id"].as<uint16_t>();

		const auto control = ui.root->find(ControlId);
		if (nullptr == control)
		{
			Serial.println((
				std::string("ERROR:prepareJSONChunk:Fragmentation:Requested control: ") + std::to_string(ControlId) +
				" does not exist").c_str());
			xSemaphoreGive(ui.ControlsSemaphore);
			return 0;
		}

		//Send Update for a Single Element
		const auto item = items.add<JsonObject>();
		control->MarshalControl(item, InUpdateMode);
		xSemaphoreGive(ui.ControlsSemaphore);
		return 1;
	}

	// keep track of the number of elements we have serialised into this
	// message. Overflow is detected and handled later in this loop
	// and needs an index to the last item added.
	int elementCount = 0;
	for (const auto &control: ui.root->getChildren())
	{
		// control has not been updated. Skip it
		if (InUpdateMode && !control->NeedsSync(CurrentSyncID))
			continue;

		auto item = items.add<JsonObject>();
		elementCount++;
		control->MarshalControl(item, InUpdateMode);
	}

	xSemaphoreGive(ui.ControlsSemaphore);
	return elementCount;
}

/*
Convert & Transfer Arduino elements to JSON elements. This function sends a chunk of
JSON describing the controls of the UI, starting from the control at index start_idx.
If start_idx is 0 then a UI_INITIAL_GUI message will be sent, else a UI_EXTEND_GUI.
Both message types contain a list of serialised UI elements. Only a portion of the UI
will be sent in order to avoid websocket buffer overflows. The client will acknowledge
receipt of a partial message by requesting the next chunk of UI.

The protocol is:
SERVER: SendControlsToClient(0):
    "UI_INITIAL_GUI: n serialised UI elements"
CLIENT: controls.js:handleEvent()
    "uiok:n"
SERVER: SendControlsToClient(n):
    "UI_EXTEND_GUI: n serialised UI elements"
CLIENT: controls.js:handleEvent()
    "uiok:2*n"
etc.
    Returns true if all controls have been sent (aka: Done)
*/
bool WebsocketClient::SendControlsToClient(const uint16_t start_idx, const ClientUpdateType_t TransferMode,
                                           const std::string &FragmentRequest)
{
	if (!CanSend())
		return false;

	JsonDocument document;
	FillInHeader(document);
	document["startindex"] = start_idx;
	document["totalcontrols"] = 0xFFFF;

	if (0 == start_idx)
	{
		document["type"] = ClientUpdateType_t::RebuildNeeded == TransferMode ? InitialGui : ExtendGUI;
		CurrentSyncID = NextSyncID;
		NextSyncID = ui.GetNextControlChangeId();

		if (prepareJSONChunk(document, ClientUpdateType_t::UpdateNeeded == TransferMode, FragmentRequest))
		{
			SendJsonDocToWebSocket(document);
			return false;
		}
	}
	return true;
}

void WebsocketClient::SendJsonDocToWebSocket(const JsonDocument &document) const
{
	if (!CanSend())
		return;

	std::string json {};
	serializeJson(document, json);

	client->text(json.c_str());
}

void WebsocketClient::SetState(const ClientUpdateType_t value)
{
	// only a higher priority state request can replace the current state request
	if (value > ClientUpdateType)
		ClientUpdateType = value;
}
