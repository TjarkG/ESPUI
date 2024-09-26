#include <utility>

#include "ESPUI.hpp"

static uint16_t idCounter = 0;
static const String ControlError = "*** ESPUI ERROR: Could not transfer control ***";

Control::Control(ControlType type, const char *label, std::function<void(Control *, int)> callback,
                 String value, ControlColor color, bool visible, uint16_t parentControl) :
	type(type),
	label(label),
	callback(std::move(callback)),
	value(std::move(value)),
	color(color),
	visible(visible),
	wide(false),
	vertical(false),
	enabled(true),
	parentControl(parentControl),
	next(nullptr)
{
	id = ++idCounter;
	ControlChangeID = 1;
}

Control::Control(const Control &Control) :
	type(Control.type),
	id(Control.id),
	label(Control.label),
	callback(Control.callback),
	value(Control.value),
	color(Control.color),
	visible(Control.visible),
	parentControl(Control.parentControl),
	next(Control.next),
	ControlChangeID(Control.ControlChangeID) {}

void Control::SendCallback(const int type)
{
	if (callback)
	{
		callback(this, type);
	}
}

void Control::DeleteControl()
{
	toDelete = true;
	callback = nullptr;
}

bool Control::MarshalControl(const JsonObject &_item,
                             const bool refresh,
                             const uint32_t StartingOffset,
                             const uint32_t AvailMarshaledLength,
                             uint32_t &EstimatedMarshaledLength,
                             const ESPUIClass &ui) const
{
	// this code assumes MaxMarshaledLength > JsonMarshalingRatio

	bool ControlIsFragmented = false;
	// create a new item in the response document
	const JsonObject &item = _item;

	// how much space do we expect to use?
	const uint32_t LabelMarshaledLength = strlen(label) * JsonMarshalingRatio;
	const uint32_t MinimumMarshaledLength = LabelMarshaledLength + JsonMarshaledOverhead;
	const uint32_t SpaceForMarshaledValue = AvailMarshaledLength - MinimumMarshaledLength;

	// will the item fit in the remaining space? Fragment if not
	if (AvailMarshaledLength < MinimumMarshaledLength)
	{
		EstimatedMarshaledLength = 0;
		return false;
	}

	const uint32_t MaxValueLength = SpaceForMarshaledValue / JsonMarshalingRatio;

	const uint32_t ValueLenToSend = min(value.length() - StartingOffset, MaxValueLength);

	const uint32_t AdjustedMarshaledLength = ValueLenToSend * JsonMarshalingRatio + MinimumMarshaledLength;

	const bool NeedToFragment = ValueLenToSend < value.length();

	if (AdjustedMarshaledLength > AvailMarshaledLength && 0 != ValueLenToSend)
	{
		EstimatedMarshaledLength = 0;
		return false;
	}

	EstimatedMarshaledLength = AdjustedMarshaledLength;

	// are we fragmenting?
	if (NeedToFragment || StartingOffset)
	{
		// indicate that no additional controls should be sent
		ControlIsFragmented = true;

		// fill in the fragment header
		_item[F("type")] = static_cast<uint32_t>(ControlType::Fragment);
		_item[F("id")] = id;

		_item[F("offset")] = StartingOffset;
		_item[F("length")] = ValueLenToSend;
		_item[F("total")] = value.length();
		AllocateNamedJsonObject(item, _item, F("control"));
	}

	item[F("id")] = id;
	ControlType TempType = (ControlType::Password == type) ? ControlType::Text : type;
	if (refresh)
	{
		item[F("type")] = static_cast<uint32_t>(TempType) + static_cast<uint32_t>(ControlType::UpdateOffset);
	} else
	{
		item[F("type")] = static_cast<uint32_t>(TempType);
	}

	item[F("label")] = label;
	item[F("value")] = (ControlType::Password == type)
		                   ? F("--------")
		                   : value.substring(StartingOffset, StartingOffset + ValueLenToSend);
	item[F("visible")] = visible;
	item[F("color")] = static_cast<int>(color);
	item[F("enabled")] = enabled;

	if (!panelStyle.isEmpty()) { item[F("panelStyle")] = panelStyle; }
	if (!elementStyle.isEmpty()) { item[F("elementStyle")] = elementStyle; }
	if (!inputType.isEmpty()) { item[F("inputType")] = inputType; }
	if (wide == true) { item[F("wide")] = true; }
	if (vertical == true) { item[F("vertical")] = true; }
	if (parentControl != noParent)
	{
		item[F("parentControl")] = String(parentControl);
	}

	// special case for selects: to preselect an option, you have to add
	// "selected" to <option>
	if (ControlType::Option == type)
	{
		const Control *ParentControl = ui.getControlNoLock(parentControl);
		if (nullptr == ParentControl)
		{
			item[F("selected")] = emptyString;
		} else if (ParentControl->value == value)
		{
			item[F("selected")] = F("selected");
		} else
		{
			item[F("selected")] = "";
		}
	}

	return ControlIsFragmented;
}

void Control::MarshalErrorMessage(const JsonObject &item) const
{
	item[F("id")] = id;
	item[F("type")] = static_cast<uint32_t>(ControlType::Label);
	item[F("label")] = ControlError.c_str();
	item[F("value")] = ControlError;
	item[F("visible")] = true;
	item[F("color")] = static_cast<int>(ControlColor::Orange);
	item[F("enabled")] = true;

	if (parentControl != noParent)
	{
		item[F("parentControl")] = String(parentControl);
	}
}

void Control::onWsEvent(const String &cmd, const String &data,ESPUIClass &ui)
{
	SetControlChangedId(ui.GetNextControlChangeId());
	if (!HasCallback())
		return;

	if (cmd.equals(F("bdown")))
	{
		SendCallback(B_DOWN);
		return;
	}

	if (cmd.equals(F("bup")))
	{
		SendCallback(B_UP);
		return;
	}

	if (cmd.equals(F("pfdown")))
	{
		SendCallback(P_FOR_DOWN);
		return;
	}

	if (cmd.equals(F("pfup")))
	{
		SendCallback(P_FOR_UP);
		return;
	}

	if (cmd.equals(F("pldown")))
	{
		SendCallback(P_LEFT_DOWN);
		return;
	} else if (cmd.equals(F("plup")))
	{
		SendCallback(P_LEFT_UP);
	} else if (cmd.equals(F("prdown")))
	{
		SendCallback(P_RIGHT_DOWN);
	} else if (cmd.equals(F("prup")))
	{
		SendCallback(P_RIGHT_UP);
	} else if (cmd.equals(F("pbdown")))
	{
		SendCallback(P_BACK_DOWN);
	} else if (cmd.equals(F("pbup")))
	{
		SendCallback(P_BACK_UP);
	} else if (cmd.equals(F("pcdown")))
	{
		SendCallback(P_CENTER_DOWN);
	} else if (cmd.equals(F("pcup")))
	{
		SendCallback(P_CENTER_UP);
	} else if (cmd.equals(F("sactive")))
	{
		value = "1";
		SendCallback(S_ACTIVE);
	} else if (cmd.equals(F("sinactive")))
	{
		value = "0";
		// updateControl(c, client->id());
		SendCallback(S_INACTIVE);
	} else if (cmd.equals(F("slvalue")))
	{
		value = data;
		// updateControl(c, client->id());
		SendCallback(SL_VALUE);
	} else if (cmd.equals(F("nvalue")))
	{
		value = data;
		// updateControl(c, client->id());
		SendCallback(N_VALUE);
	} else if (cmd.equals(F("tvalue")))
	{
		value = data;
		// updateControl(c, client->id());
		SendCallback(T_VALUE);
	} else if (cmd.equals(F("tabvalue")))
	{
		SendCallback(0);
	} else if (cmd.equals(F("svalue")))
	{
		value = data;
		// updateControl(c, client->id());
		SendCallback(S_VALUE);
	} else if (cmd.equals(F("time")))
	{
		value = data;
		// updateControl(c, client->id());
		SendCallback(TM_VALUE);
	}
}
