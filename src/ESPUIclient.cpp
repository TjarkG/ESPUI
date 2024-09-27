#include "ESPUIclient.hpp"
#include "ESPUI.hpp"
#include "ESPUIcontrol.hpp"

bool esp_ui_client::NotifyClient()
{
	if (ClientState != ClientState_t::Idle)
		return true;

	// Clear the type so that we capture any changes in type that happen
	// while we are processing the current request.
	const auto TypeToProcess = ClientUpdateType;
	ClientUpdateType = ClientUpdateType_t::Synchronized;

	// Start processing the current request.
	switch (TypeToProcess)
	{
		case ClientUpdateType_t::Synchronized:
		{
			return true;
		}
		case ClientUpdateType_t::UpdateNeeded:
		{
			ClientState = ClientState_t::Sending;
			return SendClientNotification(ClientUpdateType_t::UpdateNeeded);
		}
		case ClientUpdateType_t::RebuildNeeded:
		{
			ClientState = ClientState_t::Rebuilding;
			return SendClientNotification(ClientUpdateType_t::RebuildNeeded);
		}
		case ClientUpdateType_t::ReloadNeeded:
		{
			ClientState = ClientState_t::Reloading;
			return SendClientNotification(ClientUpdateType_t::ReloadNeeded);
		}
	}
	return false;
}

void esp_ui_client::ProcessAck(const uint16_t id, const String &FragmentRequest)
{
	switch (ClientState)
	{
		case ClientState_t::Idle:
			if (!emptyString.equals(FragmentRequest))
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
			if (!emptyString.equals(FragmentRequest))
				SendControlsToClient(id, ClientUpdateType_t::UpdateNeeded, FragmentRequest);
			break;
	}
}

bool esp_ui_client::CanSend() const
{
	if (client)
		return client->canSend();
	return false;
}

void esp_ui_client::FillInHeader(JsonDocument &document) const
{
	document[F("type")] = ExtendGUI;
	document[F("sliderContinuous")] = sliderContinuous;
	document[F("startindex")] = 0;
	document[F("totalcontrols")] = ui.controls.size();
	const JsonArray items = document[F("controls")].to<JsonArray>();
	const JsonObject titleItem = items.add<JsonObject>();
	titleItem[F("type")] = static_cast<int>(ControlType::Title);
	titleItem[F("label")] = ui.ui_title;
}

bool esp_ui_client::IsSynchronized() const
{
	return ClientUpdateType_t::Synchronized == ClientUpdateType && ClientState == ClientState_t::Idle;
}

bool esp_ui_client::SendClientNotification(const ClientUpdateType_t value) const
{
	if (!CanSend())
		return false;

	JsonDocument document;
	FillInHeader(document);
	if (ClientUpdateType_t::ReloadNeeded == value)
		document["type"] = static_cast<int>(Reload);
	// dont send any controls

	const bool Response = SendJsonDocToWebSocket(document);
	return Response;
}

void esp_ui_client::NotifyClient(const ClientUpdateType_t value)
{
	SetState(value);
	NotifyClient();
}

// Handle Websockets Communication
bool esp_ui_client::onWsEvent(const AwsEventType type, void *arg, const uint8_t *data, const size_t len)
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
			String msg = "";
			msg.reserve(len + 1);

			for (size_t i = 0; i < len; i++)
				msg += static_cast<char>(data[i]);

			const String cmd = msg.substring(0, msg.indexOf(":"));
			const String value = msg.substring(cmd.length() + 1, msg.lastIndexOf(':'));
			const uint16_t id = msg.substring(msg.lastIndexOf(':') + 1).toInt();

			if (cmd.equals(F("uiok")))
			{
				ProcessAck(id, emptyString);
				break;
			}

			if (cmd.equals(F("uifragmentok")))
			{
				if (!emptyString.equals(value))
					ProcessAck(0xFFFF, value);
				else
				{
					Serial.println(F(
						"ERROR:esp_ui_client::OnWsEvent:WS_EVT_DATA:uifragmentok:ProcessAck:Fragment Header is missing"));
				}
				break;
			}

			if (cmd.equals(F("uiuok")))
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
uint32_t esp_ui_client::prepareJSONChunk(JsonDocument &rootDoc, const bool InUpdateMode, const String &value) const
{
	xSemaphoreTake(ui.ControlsSemaphore, portMAX_DELAY);

	const uint32_t MaxMarshaledJsonSize = !InUpdateMode ? jsonInitialDocumentSize : jsonUpdateDocumentSize;
	uint32_t EstimatedUsedMarshaledJsonSize = 0;

	// Follow the list until control points to the startindex node
	const JsonArray items = rootDoc[F("controls")];

	if (!emptyString.equals(value))
	{
		// this is actually a fragment or directed update request
		// parse the string we got from the UI and try to update that specific
		// control.
		JsonDocument FragmentRequest;
		const size_t FragmentRequestStartOffset = value.indexOf("{");
		const DeserializationError error =
				deserializeJson(FragmentRequest, value.substring(FragmentRequestStartOffset));
		if (DeserializationError::Ok != error)
		{
			Serial.println(F("ERROR:prepareJSONChunk:Fragmentation:Could not extract json from the fragment request"));
			xSemaphoreGive(ui.ControlsSemaphore);
			return 0;
		}

		if (!FragmentRequest["id"].is<String>())
		{
			Serial.println(F("ERROR:prepareJSONChunk:Fragmentation:Request does not contain a control ID"));
			xSemaphoreGive(ui.ControlsSemaphore);
			return 0;
		}
		const auto ControlId = FragmentRequest[F("id")].as<uint16_t>();

		if (!FragmentRequest["offset"].is<String>())
		{
			Serial.println(F("ERROR:prepareJSONChunk:Fragmentation:Request does not contain a starting offset"));
			xSemaphoreGive(ui.ControlsSemaphore);
			return 0;
		}
		const auto DataOffset = FragmentRequest[F("offset")].as<uint32_t>();
		const auto control = ui.getControlNoLock(ControlId);
		if (nullptr == control)
		{
			Serial.println(
				String(F("ERROR:prepareJSONChunk:Fragmentation:Requested control: ")) + String(ControlId) + F(
					" does not exist"));
			xSemaphoreGive(ui.ControlsSemaphore);
			return 0;
		}

		//Send Update for a Single Element
		auto item = items.add<JsonObject>();
		const uint32_t RemainingSpace = MaxMarshaledJsonSize - 100;
		uint32_t SpaceUsedByMarshaledControl = 0;
		control->MarshalControl(item, InUpdateMode, DataOffset, RemainingSpace, SpaceUsedByMarshaledControl, ui);

		rootDoc.clear();
		item = items.add<JsonObject>();
		control->MarshalErrorMessage(item);
		xSemaphoreGive(ui.ControlsSemaphore);
		return 1;
	}

	// keep track of the number of elements we have serialised into this
	// message. Overflow is detected and handled later in this loop
	// and needs an index to the last item added.
	int elementCount = 0;
	for (const auto &control: ui.controls)
	{
		// control has not been updated. Skip it
		if (InUpdateMode && !control->NeedsSync(CurrentSyncID))
			continue;

		auto item = items.add<JsonObject>();
		elementCount++;
		const uint32_t RemainingSpace = MaxMarshaledJsonSize - EstimatedUsedMarshaledJsonSize - 100;
		uint32_t SpaceUsedByMarshaledControl = 0;
		const bool ControlIsFragmented = control->MarshalControl(item, InUpdateMode, 0, RemainingSpace,
		                                                         SpaceUsedByMarshaledControl, ui);
		EstimatedUsedMarshaledJsonSize += SpaceUsedByMarshaledControl;

		// did the control get added to the doc?
		if (SpaceUsedByMarshaledControl == 0 && elementCount == 1)
		{
			rootDoc.clear();
			item = items.add<JsonObject>();
			control->MarshalErrorMessage(item);
			elementCount = 0;
			break;
		}

		if (SpaceUsedByMarshaledControl == 0)
		{
			items.remove(elementCount);
			--elementCount;
			break;
		}

		if (ControlIsFragmented || MaxMarshaledJsonSize < EstimatedUsedMarshaledJsonSize + 100)
			break;
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
bool esp_ui_client::SendControlsToClient(const uint16_t start_idx, const ClientUpdateType_t TransferMode,
                                         const String &FragmentRequest)
{
	if (!CanSend())
		return false;
	if (start_idx >= ui.controls.size() && emptyString.equals(FragmentRequest))
		return true;

	JsonDocument document;
	FillInHeader(document);
	document[F("startindex")] = start_idx;
	document[F("totalcontrols")] = 0xFFFF; // ui.controlCount;

	if (0 == start_idx)
	{
		document["type"] = (ClientUpdateType_t::RebuildNeeded == TransferMode) ? InitialGui : ExtendGUI;
		CurrentSyncID = NextSyncID;
		NextSyncID = ui.GetNextControlChangeId();
	}
	if (prepareJSONChunk(document, ClientUpdateType_t::UpdateNeeded == TransferMode, FragmentRequest))
	{
		(void) SendJsonDocToWebSocket(document);
		return false;
	}
	return true;
}

bool esp_ui_client::SendJsonDocToWebSocket(const JsonDocument &document) const
{
	if (!CanSend())
		return false;

	String json {};
	serializeJson(document, json);

	client->text(json);
	return true;
}

void esp_ui_client::SetState(const ClientUpdateType_t value)
{
	// only a higher priority state request can replace the current state request
	if (value > ClientUpdateType)
		ClientUpdateType = value;
}
