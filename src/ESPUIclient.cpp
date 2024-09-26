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
		size_t s = serializedSize(doc) + 10; // 10 is paranoid
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
	JsonArray items = AllocateJsonArray(document, F("controls"));
	JsonObject titleItem = AllocateJsonObject(items);
	titleItem[F("type")] = static_cast<int>(UI_TITLE);
	titleItem[F("label")] = ESPUI.ui_title;
}

bool ESPUIclient::IsSyncronized() const
{
	return ((ClientUpdateType_t::Synchronized == ClientUpdateType) &&
	        (&fsm_EspuiClient_state_Idle_imp == pCurrentFsmState));
}

bool ESPUIclient::SendClientNotification(const ClientUpdateType_t value) const
{
	if (!CanSend())
	{
		// Serial.println(F("ESPUIclient::SendClientNotification:CannotSend"));
		return false;
	}

	AllocateJsonDocument(document, ESPUI.jsonUpdateDocumentSize);
	FillInHeader(document);
	if (ClientUpdateType_t::ReloadNeeded == value)
	{
		// Serial.println(F("ESPUIclient::SendClientNotification:set type to reload"));
		document["type"] = static_cast<int>(UI_RELOAD);
	}
	// dont send any controls

	const bool Response = SendJsonDocToWebSocket(document);
	// Serial.println(String("ESPUIclient::SendClientNotification:NotificationSent:Response: ") + String(Response));
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
	// Serial.println(String("ESPUIclient::OnWsEvent: type: ") + String(type));

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
#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity)
            {
                Serial.println(F("ESPUIclient::OnWsEvent:WS_EVT_CONNECT"));
                Serial.println(client->id());
            }
#endif

			// Serial.println("ESPUIclient:onWsEvent:WS_EVT_CONNECT: Call NotifyClient: RebuildNeeded");
			NotifyClient(ClientUpdateType_t::RebuildNeeded);
			break;
		}

		case WS_EVT_DATA:
		{
			// Serial.println(F("ESPUIclient::OnWsEvent:WS_EVT_DATA"));
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
				// Serial.println(String(F("ESPUIclient::OnWsEvent:WS_EVT_DATA:uiok:ProcessAck:")) + pCurrentFsmState->GetStateName());
				pCurrentFsmState->ProcessAck(id, emptyString);
				break;
			}

			if (cmd.equals(F("uifragmentok")))
			{
				// Serial.println(String(F("ESPUIclient::OnWsEvent:WS_EVT_DATA:uiok:uifragmentok:")) + pCurrentFsmState->GetStateName() + ":ProcessAck");
				if (!emptyString.equals(value))
				{
					// Serial.println(String(F("ESPUIclient::OnWsEvent:WS_EVT_DATA:uiok:uifragmentok:")) + pCurrentFsmState->GetStateName() + ":ProcessAck:value:'" +  value + "'");
					pCurrentFsmState->ProcessAck(-1, value);
				} else
				{
					Serial.println(F(
						"ERROR:ESPUIclient::OnWsEvent:WS_EVT_DATA:uifragmentok:ProcessAck:Fragment Header is missing"));
				}
				break;
			}

			if (cmd.equals(F("uiuok")))
			{
				// Serial.println(F("WS_EVT_DATA: uiuok. Unlock new async notifications"));
				break;
			}

			// Serial.println(F("WS_EVT_DATA:Process Control"));
			Control *control = ESPUI.getControl(id);
			if (nullptr == control)
			{
#if defined(DEBUG_ESPUI)
                if (ESPUI.verbosity)
                {
                    Serial.println(String(F("No control found for ID ")) + String(id));
                }
#endif
				break;
			}
			control->onWsEvent(cmd, value);
			// notify other clients of change
			Response = true;
			break;
		}

		default:
		{
			// Serial.println(F("ESPUIclient::OnWsEvent:default"));
			break;
		}
	} // end switch

	return Response;
}

/*
Prepare a chunk of elements as a single JSON string. If the allowed number of elements is greater than the total
number this will represent the entire UI. More likely, it will represent a small section of the UI to be sent. The
client will acknowledge receipt by requesting the next chunk.
 */
uint32_t ESPUIclient::prepareJSONChunk(uint16_t startindex,
                                       JsonDocument &rootDoc,
                                       const bool InUpdateMode,
                                       const String &value) const
{
	xSemaphoreTake(ESPUI.ControlsSemaphore, portMAX_DELAY);

	// Serial.println(String("prepareJSONChunk: Start.          InUpdateMode: ") + String(InUpdateMode));
	// Serial.println(String("prepareJSONChunk: Start.            startindex: ") + String(startindex));
	// Serial.println(String("prepareJSONChunk: Start. FragmentRequestString: '") + FragmentRequestString + "'");
	int elementCount = 0;
	uint32_t MaxMarshaledJsonSize = (!InUpdateMode) ? ESPUI.jsonInitialDocumentSize : ESPUI.jsonUpdateDocumentSize;
	uint32_t EstimatedUsedMarshaledJsonSize = 0;

	do // once
	{
		// Follow the list until control points to the startindex'th node
		Control *control = ESPUI.controls;
		uint32_t currentIndex = 0;
		uint32_t DataOffset = 0;
		JsonArray items = rootDoc[F("controls")];
		bool SingleControl = false;

		if (!emptyString.equals(value))
		{
			// Serial.println(F("prepareJSONChunk:Fragmentation:Got Header (1)"));
			// Serial.println(String("prepareJSONChunk:startindex:                  ") + String(startindex));
			// Serial.println(String("prepareJSONChunk:currentIndex:                ") + String(currentIndex));
			// Serial.println(String("prepareJSONChunk:FragmentRequestString:      '") + FragmentRequestString + "'");

			// this is actually a fragment or directed update request
			// parse the string we got from the UI and try to update that specific
			// control.
			AllocateJsonDocument(FragmentRequest, FragmentRequestString.length() * 3);
			/*
			            ArduinoJson::detail::sizeofObject(N);
			            if(0 >= FragmentRequest.capacity())
			            {
			                Serial.println(F("ERROR:prepareJSONChunk:Fragmentation:Could not allocate memory for a fragmentation request. Skipping Response"));
			                break;
			            }
			*/
			size_t FragmentRequestStartOffset = value.indexOf("{");
			DeserializationError error = deserializeJson(FragmentRequest,
			                                             value.substring(FragmentRequestStartOffset));
			if (DeserializationError::Ok != error)
			{
				Serial.println(
					F("ERROR:prepareJSONChunk:Fragmentation:Could not extract json from the fragment request"));
				break;
			}

			if (!FragmentRequest["id"].is<String>())
			{
				Serial.println(F("ERROR:prepareJSONChunk:Fragmentation:Request does not contain a control ID"));
				break;
			}
			const auto ControlId = FragmentRequest[F("id")].as<uint16_t>();

			if (!FragmentRequest["offset"].is<String>())
			{
				Serial.println(F("ERROR:prepareJSONChunk:Fragmentation:Request does not contain a starting offset"));
				break;
			}
			DataOffset = FragmentRequest[F("offset")].as<uint32_t>();
			control = ESPUI.getControlNoLock(ControlId);
			if (nullptr == control)
			{
				Serial.println(
					String(F("ERROR:prepareJSONChunk:Fragmentation:Requested control: ")) + String(ControlId) + F(
						" does not exist"));
				break;
			}

			// Serial.println(F("prepareJSONChunk:Fragmentation:disable the control search operation"));
			currentIndex = 1;
			startindex = 0;
			SingleControl = true;
		}

		// find a control to send
		while ((startindex > currentIndex) && (nullptr != control))
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
			// Serial.println("prepareJSONChunk: No controls to process");
			break;
		}

		// keep track of the number of elements we have serialised into this
		// message. Overflow is detected and handled later in this loop
		// and needs an index to the last item added.
		while (nullptr != control)
		{
			// skip deleted controls or controls that have not been updated
			if (control->ToBeDeleted() && !SingleControl)
			{
				// Serial.println(String("prepareJSONChunk: Ignoring Deleted control: ") + String(control->id));
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

			// Serial.println(String(F("prepareJSONChunk: MaxMarshaledJsonSize: ")) + String(MaxMarshaledJsonSize));
			// Serial.println(String(F("prepareJSONChunk: Cur EstimatedUsedMarshaledJsonSize: ")) + String(EstimatedUsedMarshaledJsonSize));

			JsonObject item = AllocateJsonObject(items);
			elementCount++;
			uint32_t RemainingSpace = (MaxMarshaledJsonSize - EstimatedUsedMarshaledJsonSize) - 100;
			// Serial.println(String(F("prepareJSONChunk: RemainingSpace: ")) + String(RemainingSpace));
			uint32_t SpaceUsedByMarshaledControl = 0;
			bool ControlIsFragmented = control->MarshalControl(item,
			                                                   InUpdateMode,
			                                                   DataOffset,
			                                                   RemainingSpace,
			                                                   SpaceUsedByMarshaledControl);
			// Serial.println(String(F("prepareJSONChunk: SpaceUsedByMarshaledControl: ")) + String(SpaceUsedByMarshaledControl));
			EstimatedUsedMarshaledJsonSize += SpaceUsedByMarshaledControl;
			// Serial.println(String(F("prepareJSONChunk: New EstimatedUsedMarshaledJsonSize: ")) + String(EstimatedUsedMarshaledJsonSize));
			// Serial.println(String(F("prepareJSONChunk:                ControlIsFragmented: ")) + String(ControlIsFragmented));

			// did the control get added to the doc?
			if (0 == SpaceUsedByMarshaledControl ||
			    (ESPUI.jsonChunkNumberMax > 0 && (elementCount % ESPUI.jsonChunkNumberMax) == 0))
			{
				// Serial.println( String("prepareJSONChunk: too much data in the message. Remove the last entry"));
				if (1 == elementCount)
				{
					// Serial.println(String(F("prepareJSONChunk: Control ")) + String(control->id) + F(" is too large to be sent to the browser."));
					// Serial.println(String(F("ERROR: prepareJSONChunk: value: ")) + control->value);
					rootDoc.clear();
					item = AllocateJsonObject(items);
					control->MarshalErrorMessage(item);
					elementCount = 0;
				} else
				{
					// Serial.println(String("prepareJSONChunk: Deferring control: ") + String(control->id));
					// Serial.println(String("prepareJSONChunk: elementCount: ") + String(elementCount));

					items.remove(elementCount);
					--elementCount;
				}
				// exit the loop
				control = nullptr;
			} else if ((SingleControl) ||
			           (ControlIsFragmented) ||
			           (MaxMarshaledJsonSize < (EstimatedUsedMarshaledJsonSize + 100)))
			{
				// Serial.println("prepareJSONChunk: Doc is Full, Fragmented Control or Single Control. exit loop");
				control = nullptr;
			} else
			{
				// Serial.println("prepareJSONChunk: Next Control");
				control = control->next;
			}
		} // end while (control != nullptr)
	} while (false);

	xSemaphoreGive(ESPUI.ControlsSemaphore);
	// Serial.println(String("prepareJSONChunk: END: elementCount: ") + String(elementCount));
	return elementCount;
}

/*
Convert & Transfer Arduino elements to JSON elements. This function sends a chunk of
JSON describing the controls of the UI, starting from the control at index startidx.
If startidx is 0 then a UI_INITIAL_GUI message will be sent, else a UI_EXTEND_GUI.
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
bool ESPUIclient::SendControlsToClient(const uint16_t startidx, const ClientUpdateType_t TransferMode,
                                       const String &FragmentRequest)
{
	bool Response = false;
	// Serial.println(String("ESPUIclient:SendControlsToClient:startidx: ") + String(startidx));
	do // once
	{
		if (!CanSend())
		{
			// Serial.println("ESPUIclient:SendControlsToClient: Cannot Send to clients.");
			break;
		} else if ((startidx >= ESPUI.controlCount) && (emptyString.equals(FragmentRequest)))
		{
			// Serial.println(F("ERROR:ESPUIclient:SendControlsToClient: No more controls to send."));
			Response = true;
			break;
		}

		AllocateJsonDocument(document, ESPUI.jsonInitialDocumentSize);
		FillInHeader(document);
		document[F("startindex")] = startidx;
		document[F("totalcontrols")] = -1; // ESPUI.controlCount;

		if (0 == startidx)
		{
			// Serial.println("ESPUIclient:SendControlsToClient: Tell client we are starting a transfer of controls.");
			document["type"] = (ClientUpdateType_t::RebuildNeeded == TransferMode) ? UI_INITIAL_GUI : UI_EXTEND_GUI;
			CurrentSyncID = NextSyncID;
			NextSyncID = ESPUI.GetNextControlChangeId();
		}
		// Serial.println(String("ESPUIclient:SendControlsToClient:type: ") + String((uint32_t)document["type"]));

		// Serial.println("ESPUIclient:SendControlsToClient: Build Controls.");
		if (prepareJSONChunk(startidx, document, ClientUpdateType_t::UpdateNeeded == TransferMode, FragmentRequest))
		{
#if defined(DEBUG_ESPUI)
                if (ESPUI.verbosity >= Verbosity::VerboseJSON)
                {
                    Serial.println(F("ESPUIclient:SendControlsToClient: Sending elements --------->"));
                    serializeJson(document, Serial);
                    Serial.println();
                }


			Serial.println("ESPUIclient:SendControlsToClient: Send message.");
			if (SendJsonDocToWebSocket(document))
			{
				Serial.println("ESPUIclient:SendControlsToClient: Sent.");
			} else
			{
				Serial.println("ESPUIclient:SendControlsToClient: Send failed.");
			}
#else
			SendJsonDocToWebSocket(document);
#endif
		} else
		{
			// Serial.println("ESPUIclient:SendControlsToClient: No elements to send.");
			Response = true;
		}
	} while (false);

	// Serial.println(String("ESPUIclient:SendControlsToClient:Response: ") + String(Response));
	return Response;
}

bool ESPUIclient::SendJsonDocToWebSocket(const JsonDocument &document) const
{
	bool Response = true;

	do // once
	{
		if (!CanSend())
		{
#if defined(DEBUG_ESPUI)
                if (ESPUI.verbosity >= Verbosity::VerboseJSON)
                {
                    Serial.println(F("ESPUIclient::SendJsonDocToWebSocket: Cannot Send to client. Not sending websocket message"));
                }
#endif
			// Serial.println("ESPUIclient::SendJsonDocToWebSocket: Cannot Send to client. Not sending websocket message");
			Response = false;
			break;
		}

		String json = JSONSlave::toString(document);

#if defined(DEBUG_ESPUI)
            if (ESPUI.verbosity >= Verbosity::VerboseJSON)
            {
                Serial.println(String(F("ESPUIclient::SendJsonDocToWebSocket: json: '")) + json + "'");
                Serial.println(F("ESPUIclient::SendJsonDocToWebSocket: client.text"));
            }
#endif
		// Serial.println(F("ESPUIclient::SendJsonDocToWebSocket: client.text"));
		client->text(json);
	} while (false);

	return Response;
}

void ESPUIclient::SetState(const ClientUpdateType_t value)
{
	// only a higher priority state request can replace the current state request
	if (ClientUpdateType < value)
	{
		ClientUpdateType = value;
	}
}
