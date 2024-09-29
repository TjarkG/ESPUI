#include <utility>

#include "ESPUI.hpp"

static const std::string ControlError = "*** ESPUI ERROR: Could not transfer control ***";

Control::Control(const ControlType type, std::string label, std::function<void(Control *, int)> callback,
                 std::string value, const ControlColor color, const bool visible,
                 const std::shared_ptr<Control> &parentControl, ESPUIClass &ui):
	ui(ui),
	parentControl(parentControl),
	type(type),
	label(std::move(label)),
	callback(std::move(callback)),
	value(std::move(value)),
	color(color),
	visible(visible)
{
	static uint16_t idCounter = 0;
	id = ++idCounter;
	ControlChangeID = 1;
}

Control::Control(const Control &Control) :
	ui(Control.ui),
	parentControl(Control.parentControl),
	type(Control.type),
	id(Control.id),
	label(Control.label),
	callback(Control.callback),
	value(Control.value),
	color(Control.color), visible(Control.visible),
	ControlChangeID(Control.ControlChangeID) {}

std::shared_ptr<Control> Control::add(const ControlType type, const std::string &label, const std::string &value,
                                      const ControlColor color, const std::function<void(Control *, int)> &callback,
                                      const bool visible)
{
	const Control control = {type, label, callback, value, color, visible, std::shared_ptr<Control>(this), ui};

	return ui.addControl(control);
}

bool Control::MarshalControl(const JsonObject &item, const bool refresh, const uint32_t DataOffset,
                             const uint32_t MaxLength, uint32_t &EstimatedUsedSpace) const
{
	// this code assumes MaxMarshaledLength > JsonMarshalingRatio
	if (refresh && type == ControlType::Tab)
		return false;

	// how much space do we expect to use?
	const uint32_t LabelMarshaledLength = label.length() * JsonMarshalingRatio;
	const uint32_t MinimumMarshaledLength = LabelMarshaledLength + JsonMarshaledOverhead;
	const uint32_t SpaceForMarshaledValue = MaxLength - MinimumMarshaledLength;

	// will the item fit in the remaining space? Fragment if not
	if (MaxLength < MinimumMarshaledLength)
	{
		EstimatedUsedSpace = 0;
		return false;
	}

	const uint32_t MaxValueLength = SpaceForMarshaledValue / JsonMarshalingRatio;

	const uint32_t ValueLenToSend = min(value.length() - DataOffset, MaxValueLength);

	const uint32_t AdjustedMarshaledLength = ValueLenToSend * JsonMarshalingRatio + MinimumMarshaledLength;

	const bool NeedToFragment = ValueLenToSend < value.length();

	if (AdjustedMarshaledLength > MaxLength && 0 != ValueLenToSend)
	{
		EstimatedUsedSpace = 0;
		return false;
	}

	EstimatedUsedSpace = AdjustedMarshaledLength;

	// are we fragmenting?
	if (NeedToFragment || DataOffset)
	{
		// fill in the fragment header
		item[F("type")] = static_cast<uint32_t>(ControlType::Fragment);
		item[F("id")] = id;

		item[F("offset")] = DataOffset;
		item[F("length")] = ValueLenToSend;
		item[F("total")] = value.length();
		item[F("control")] = item;
	}

	item[F("id")] = id;
	ControlType TempType = ControlType::Password == type ? ControlType::Text : type;
	if (refresh)
	{
		item[F("type")] = static_cast<uint32_t>(TempType) + static_cast<uint32_t>(ControlType::UpdateOffset);
	} else
	{
		item[F("type")] = static_cast<uint32_t>(TempType);
	}

	item[F("label")] = label;
	item[F("value")] = ControlType::Password == type ? "--------" : value.substr(DataOffset, ValueLenToSend);
	item[F("visible")] = visible;
	item[F("color")] = static_cast<int>(color);
	item[F("enabled")] = enabled;

	if (!panelStyle.empty()) { item[F("panelStyle")] = panelStyle; }
	if (!elementStyle.empty()) { item[F("elementStyle")] = elementStyle; }
	if (!inputType.empty()) { item[F("inputType")] = inputType; }
	if (wide == true) { item[F("wide")] = true; }
	if (vertical == true) { item[F("vertical")] = true; }
	if (parentControl)
		item[F("parentControl")] = std::to_string(parentControl->id);

	// special case for selects: to preselect an option, you have to add
	// "selected" to <option>
	if (ControlType::Option == type)
	{
		if (!parentControl)
		{
			item[F("selected")] = emptyString;
		} else if (parentControl->value == value)
		{
			item[F("selected")] = F("selected");
		} else
		{
			item[F("selected")] = "";
		}
	}

	// indicate that no additional controls should be sent if fragmenting is on
	return NeedToFragment || DataOffset;
}

void Control::MarshalErrorMessage(const JsonObject &item) const
{
	item[F("id")] = id;
	item[F("type")] = static_cast<uint32_t>(ControlType::Label);
	item[F("label")] = ControlError;
	item[F("value")] = ControlError;
	item[F("visible")] = true;
	item[F("color")] = static_cast<int>(ControlColor::Orange);
	item[F("enabled")] = true;

	if (parentControl)
	{
		item[F("parentControl")] = std::to_string(parentControl->id);
	}
}

void Control::onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui)
{
	SetControlChangedId(ui.GetNextControlChangeId());

	if (cmd == "bdown")
		SendCallback(B_DOWN);
	else if (cmd == "bup")
		SendCallback(B_UP);
	else if (cmd == "pfdown")
		SendCallback(P_FOR_DOWN);
	else if (cmd == "pfup")
		SendCallback(P_FOR_UP);
	else if (cmd == "pldown")
		SendCallback(P_LEFT_DOWN);
	else if (cmd == "plup")
		SendCallback(P_LEFT_UP);
	else if (cmd == "prdown")
		SendCallback(P_RIGHT_DOWN);
	else if (cmd == "prup")
		SendCallback(P_RIGHT_UP);
	else if (cmd == "pbdown")
		SendCallback(P_BACK_DOWN);
	else if (cmd == "pbup")
		SendCallback(P_BACK_UP);
	else if (cmd == "pcdown")
		SendCallback(P_CENTER_DOWN);
	else if (cmd == "pcup")
		SendCallback(P_CENTER_UP);
	else if (cmd == "sactive")
	{
		value = "1";
		SendCallback(S_ACTIVE);
	} else if (cmd == "sinactive")
	{
		value = "0";
		SendCallback(S_INACTIVE);
	} else if (cmd == "slvalue")
	{
		value = data;
		SendCallback(SL_VALUE);
	} else if (cmd == "nvalue")
	{
		value = data;
		SendCallback(N_VALUE);
	} else if (cmd == "tvalue")
	{
		value = data;
		SendCallback(T_VALUE);
	} else if (cmd == "tabvalue")
		SendCallback(0);
	else if (cmd == "svalue")
	{
		value = data;
		SendCallback(S_VALUE);
	} else if (cmd == "time")
	{
		value = data;
		SendCallback(TM_VALUE);
	}
}
