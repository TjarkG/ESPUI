#include "ESPUI.h"
#include "ESPUIclient.h"
#include "ESPUIcontrol.h"

// JSONSlave:
// helper to process exact JSON serialization size
// it takes ~2ms on esp8266 and avoid large String reallocation which is really worth the cost
class JSONSlave : public Print
{
public:
	size_t write(uint8_t c) override
	{
		counter++;
		return 1;
	}

	size_t write(const uint8_t *buf, const size_t count) override
	{
		counter += count;
		return count;
	}

	size_t get_counter() const { return counter; }

	static size_t serializedSize(const JsonDocument &doc)
	{
		JSONSlave counter;
		serializeJson(doc, counter);
		return counter.get_counter();
	}

	static size_t serialize(const JsonDocument &doc, String &str)
	{
		const size_t s = serializedSize(doc) + 10; // 10 is paranoid
		str.reserve(s);
		serializeJson(doc, str);
		return s;
	}

	static String toString(const JsonDocument &doc)
	{
		String str;
		serialize(doc, str);
		return str;
	}

protected:
	size_t counter = 0;
};

ESPUIclient::ESPUIclient(AsyncWebSocketClient *_client):
	client(_client)
{
	fsm_EspuiClient_state_Idle_imp.SetParent(this);
	fsm_EspuiClient_state_SendingUpdate_imp.SetParent(this);
	fsm_EspuiClient_state_Rebuilding_imp.SetParent(this);
	fsm_EspuiClient_state_Reloading_imp.SetParent(this);

	fsm_EspuiClient_state_Idle_imp.Init();
}

ESPUIclient::ESPUIclient(const ESPUIclient &source):
	client(source.client)
{
	fsm_EspuiClient_state_Idle_imp.SetParent(this);
	fsm_EspuiClient_state_SendingUpdate_imp.SetParent(this);
	fsm_EspuiClient_state_Rebuilding_imp.SetParent(this);
	fsm_EspuiClient_state_Reloading_imp.SetParent(this);

	fsm_EspuiClient_state_Idle_imp.Init();
}

ESPUIclient::~ESPUIclient() = default;

bool ESPUIclient::CanSend() const
{
	bool Response = false;
	if (nullptr != client)
	{
		Response = client->canSend();
	}
	return Response;
}

void ESPUIclient::FillInHeader(JsonDocument &document)
{
	document[F("type")] = UI_EXTEND_GUI;
	document[F("sliderContinuous")] = ESPUI.sliderContinuous;
	document[F("startindex")] = 0;
	document[F("totalcontrols")] = ESPUI.controlCount;
	const JsonArray items = AllocateJsonArray(document, F("controls"));
	const JsonObject titleItem = AllocateJsonObject(items);
	titleItem[F("type")] = static_cast<int>(UI_TITLE);
	titleItem[F("label")] = ESPUI.ui_title;
}

bool ESPUIclient::IsSyncronized() const
{
	return Synchronized == ClientUpdateType &&
	       &fsm_EspuiClient_state_Idle_imp == pCurrentFsmState;
}

bool ESPUIclient::SendClientNotification(const ClientUpdateType_t value) const
{
	if (!CanSend())
		return false;

	AllocateJsonDocument(document, ESPUI.jsonUpdateDocumentSize);
	FillInHeader(document);
	if (ReloadNeeded == value)
		document["type"] = static_cast<int>(UI_RELOAD);
	// dont send any controls

	const bool Response = SendJsonDocToWebSocket(document);
	return Response;
}

void ESPUIclient::NotifyClient(const ClientUpdateType_t newState)
{
	SetState(newState);
	pCurrentFsmState->NotifyClient();
}

// Handle Websockets Communication
bool ESPUIclient::onWsEvent(const AwsEventType type, void *arg, const uint8_t *data, const size_t len)
{
	bool Response = false;

	switch (type)
	{
#if defined(DEBUG_ESPUI)
		case WS_EVT_PONG:
		{
            if (ESPUI.verbosity)
            {
                Serial.println(F("ESPUIclient::OnWsEvent:WS_EVT_PONG"));
            }
			break;
		}

		case WS_EVT_ERROR:
		{
            if (ESPUI.verbosity)
            {
                Serial.println(F("ESPUIclient::OnWsEvent:WS_EVT_ERROR"));
            }
			break;
		}
#endif

		case WS_EVT_CONNECT:
		{
			NotifyClient(RebuildNeeded);
			break;
		}

		case WS_EVT_DATA:
		{
			String msg = "";
			msg.reserve(len + 1);

			for (size_t i = 0; i < len; i++)
			{
				msg += static_cast<char>(data[i]);
			}

			String cmd = msg.substring(0, msg.indexOf(":"));
			String value = msg.substring(cmd.length() + 1, msg.lastIndexOf(':'));
			uint16_t id = msg.substring(msg.lastIndexOf(':') + 1).toInt();

#if defined(DEBUG_ESPUI)
                if (ESPUI.verbosity >= Verbosity::VerboseJSON)
                {
                    Serial.println(String(F("  WS msg: ")) + msg);
                    Serial.println(String(F("  WS cmd: ")) + cmd);
                    Serial.println(String(F("   WS id: ")) + String(id));
                    Serial.println(String(F("WS value: ")) + String(value));
                }
#endif

			if (cmd.equals(F("uiok")))
			{
				pCurrentFsmState->ProcessAck(id, emptyString);
				break;
			}

			if (cmd.equals(F("uifragmentok")))
			{
				if (!emptyString.equals(value))
					pCurrentFsmState->ProcessAck(0xFFFF, value);
				else
				{
					Serial.println(F(
						"ERROR:ESPUIclient::OnWsEvent:WS_EVT_DATA:uifragmentok:ProcessAck:Fragment Header is missing"));
				}
				break;
			}

			if (cmd.equals(F("uiuok")))
				break;

			Control *control = ESPUI.getControl(id);
			if (nullptr == control)
				break;
			control->onWsEvent(cmd, value);
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
uint32_t ESPUIclient::prepareJSONChunk(uint16_t startindex, JsonDocument &rootDoc, const bool InUpdateMode,
                                       const String &value) const
{
	xSemaphoreTake(ESPUI.ControlsSemaphore, portMAX_DELAY);

	const uint32_t MaxMarshaledJsonSize = !InUpdateMode ? ESPUI.jsonInitialDocumentSize : ESPUI.jsonUpdateDocumentSize;
	uint32_t EstimatedUsedMarshaledJsonSize = 0;

	// Follow the list until control points to the startindex node
	const Control *control = ESPUI.controls;
	uint32_t currentIndex = 0;
	uint32_t DataOffset = 0;
	const JsonArray items = rootDoc[F("controls")];
	bool SingleControl = false;

	if (!emptyString.equals(value))
	{
		// this is actually a fragment or directed update request
		// parse the string we got from the UI and try to update that specific
		// control.
		AllocateJsonDocument(FragmentRequest, FragmentRequestString.length() * 3);
		const size_t FragmentRequestStartOffset = value.indexOf("{");
		const DeserializationError error = deserializeJson(FragmentRequest,
		                                                   value.substring(FragmentRequestStartOffset));
		if (DeserializationError::Ok != error)
		{
			Serial.println(
				F("ERROR:prepareJSONChunk:Fragmentation:Could not extract json from the fragment request"));
			xSemaphoreGive(ESPUI.ControlsSemaphore);
			return 0;
		}

		if (!FragmentRequest["id"].is<String>())
		{
			Serial.println(F("ERROR:prepareJSONChunk:Fragmentation:Request does not contain a control ID"));
			xSemaphoreGive(ESPUI.ControlsSemaphore);
			return 0;
		}
		const auto ControlId = FragmentRequest[F("id")].as<uint16_t>();

		if (!FragmentRequest["offset"].is<String>())
		{
			Serial.println(F("ERROR:prepareJSONChunk:Fragmentation:Request does not contain a starting offset"));
			xSemaphoreGive(ESPUI.ControlsSemaphore);
			return 0;
		}
		DataOffset = FragmentRequest[F("offset")].as<uint32_t>();
		control = ESPUI.getControlNoLock(ControlId);
		if (nullptr == control)
		{
			Serial.println(
				String(F("ERROR:prepareJSONChunk:Fragmentation:Requested control: ")) + String(ControlId) + F(
					" does not exist"));
			xSemaphoreGive(ESPUI.ControlsSemaphore);
			return 0;
		}

		currentIndex = 1;
		startindex = 0;
		SingleControl = true;
	}

	// find a control to send
	while (startindex > currentIndex && nullptr != control)
	{
		// only count active controls
		if (!control->ToBeDeleted())
		{
			if (InUpdateMode)
			{
				// In update mode we only count the controls that have been updated.
				if (control->NeedsSync(CurrentSyncID))
				{
					++currentIndex;
				}
			} else
			{
				// not in update mode. Count all active controls
				++currentIndex;
			}
		}
		control = control->next;
	}

	// any controls left to be processed?
	if (nullptr == control)
	{
		xSemaphoreGive(ESPUI.ControlsSemaphore);
		return 0;
	}

	// keep track of the number of elements we have serialised into this
	// message. Overflow is detected and handled later in this loop
	// and needs an index to the last item added.
	int elementCount = 0;
	while (nullptr != control)
	{
		// skip deleted controls or controls that have not been updated
		if (control->ToBeDeleted() && !SingleControl)
		{
			control = control->next;
			continue;
		}

		if (InUpdateMode && !SingleControl)
		{
			if (control->NeedsSync(CurrentSyncID))
			{
				// dont skip this control
			} else
			{
				// control has not been updated. Skip it
				control = control->next;
				continue;
			}
		}

		JsonObject item = AllocateJsonObject(items);
		elementCount++;
		const uint32_t RemainingSpace = MaxMarshaledJsonSize - EstimatedUsedMarshaledJsonSize - 100;
		uint32_t SpaceUsedByMarshaledControl = 0;
		const bool ControlIsFragmented = control->MarshalControl(item,
		                                                         InUpdateMode,
		                                                         DataOffset,
		                                                         RemainingSpace,
		                                                         SpaceUsedByMarshaledControl);
		EstimatedUsedMarshaledJsonSize += SpaceUsedByMarshaledControl;

		// did the control get added to the doc?
		if (0 == SpaceUsedByMarshaledControl ||
		    (ESPUI.jsonChunkNumberMax > 0 && elementCount % ESPUI.jsonChunkNumberMax == 0))
		{
			if (1 == elementCount)
			{
				rootDoc.clear();
				item = AllocateJsonObject(items);
				control->MarshalErrorMessage(item);
				elementCount = 0;
			} else
			{
				items.remove(elementCount);
				--elementCount;
			}
			// exit the loop
			control = nullptr;
		} else if (SingleControl || ControlIsFragmented || MaxMarshaledJsonSize < EstimatedUsedMarshaledJsonSize +
		           100)
			control = nullptr;
		else
			control = control->next;
	} // end while (control != nullptr)

	xSemaphoreGive(ESPUI.ControlsSemaphore);
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
bool ESPUIclient::SendControlsToClient(const uint16_t start_idx, const ClientUpdateType_t TransferMode,
                                       const String &FragmentRequest)
{
	if (!CanSend())
		return false;
	if (start_idx >= ESPUI.controlCount && emptyString.equals(FragmentRequest))
		return true;

	AllocateJsonDocument(document, ESPUI.jsonInitialDocumentSize);
	FillInHeader(document);
	document[F("startindex")] = start_idx;
	document[F("totalcontrols")] = 0xFFFF; // ESPUI.controlCount;

	if (0 == start_idx)
	{
		document["type"] = (RebuildNeeded == TransferMode) ? UI_INITIAL_GUI : UI_EXTEND_GUI;
		CurrentSyncID = NextSyncID;
		NextSyncID = ESPUI.GetNextControlChangeId();
	}
	if (prepareJSONChunk(start_idx, document, UpdateNeeded == TransferMode, FragmentRequest))
	{
		(void) SendJsonDocToWebSocket(document);
		return false;
	}
	return true;
}

bool ESPUIclient::SendJsonDocToWebSocket(const JsonDocument &document) const
{
	if (!CanSend())
		return false;

	const String json = JSONSlave::toString(document);

	client->text(json);
	return true;
}

void ESPUIclient::SetState(const ClientUpdateType_t value)
{
	// only a higher priority state request can replace the current state request
	if (value > ClientUpdateType)
		ClientUpdateType = value;
}
