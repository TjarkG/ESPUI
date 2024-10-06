#include <utility>

#include "ESPUI.hpp"

static const std::string ControlError = "*** ESPUI ERROR: Could not transfer control ***";

Control::Control(const ControlType type, std::string label, std::function<void(Control *, UpdateType)> callback,
                 std::string value, const ControlColor color, const bool visible,
                 const std::shared_ptr<Control> &parentControl, ESPUIClass *ui):
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

//Copy Constructor. No need to call Constructor of shared_from_this since it is empty
Control::Control(const Control &Control) : // NOLINT(*-copy-constructor-init)
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
                                      const ControlColor color,
                                      const std::function<void(Control *, UpdateType)> &callback,
                                      const bool visible)
{
	auto control = std::make_shared<Control>(type, label, callback, value, color, visible, shared_from_this(), ui);

	children.push_back(control);
	if (ui)
	{
		xSemaphoreTake(ui->ControlsSemaphore, portMAX_DELAY);
		ui->NotifyClients(ClientUpdateType_t::RebuildNeeded);
		xSemaphoreGive(ui->ControlsSemaphore);
	}
	return control;
}

void Control::remove(const bool force_rebuild_ui) const
{
	if (ui)
		xSemaphoreTake(ui->ControlsSemaphore, portMAX_DELAY);
	const auto it = std::find_if(parentControl->children.begin(), parentControl->children.end(),
	                             [this](const std::shared_ptr<Control> &i)
	                             {
		                             return i.get() == this;
	                             });

	parentControl->children.erase(it);

	if (ui)
	{
		xSemaphoreGive(ui->ControlsSemaphore);
		if (force_rebuild_ui)
			ui->NotifyClients(ClientUpdateType_t::ReloadNeeded);
		else
			ui->NotifyClients(ClientUpdateType_t::RebuildNeeded);
	}
}

std::shared_ptr<Control> Control::find(const uint16_t id_in)
{
	if (id_in == id)
		return shared_from_this();

	for (const auto &child: children)
		if (auto tmp = child->find(id_in); tmp != nullptr)
			return tmp;

	return nullptr;
}

size_t Control::getChildCount() const
{
	size_t i = children.size();
	for (const auto &child: children)
		i += child->getChildCount();

	return i;
}

std::vector<std::shared_ptr<Control> > Control::getChildren() const
{
	auto v = children;
	for (const auto &child: children)
	{
		auto cv = child->getChildren();
		v.insert(v.end(), cv.begin(), cv.end());
	}

	return v;
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

	if (cmd == "buttonDown")
		SendCallback(UpdateType::ButtonDown);
	else if (cmd == "buttonUP")
		SendCallback(UpdateType::ButtonUp);
	else if (cmd == "padForwardDown")
		SendCallback(UpdateType::PadForwardDown);
	else if (cmd == "padForwardUp")
		SendCallback(UpdateType::PadForwardUp);
	else if (cmd == "padLeftDown")
		SendCallback(UpdateType::PadLeftDown);
	else if (cmd == "padLeftUp")
		SendCallback(UpdateType::PadLeftUp);
	else if (cmd == "padRightDown")
		SendCallback(UpdateType::PadRightDown);
	else if (cmd == "padRightUp")
		SendCallback(UpdateType::PadRightUp);
	else if (cmd == "padBackDown")
		SendCallback(UpdateType::PadBackDown);
	else if (cmd == "padBackUp")
		SendCallback(UpdateType::PadBackUp);
	else if (cmd == "padCenterDown")
		SendCallback(UpdateType::PadCenterDown);
	else if (cmd == "padCenterUp")
		SendCallback(UpdateType::PadCenterUp);
	else if (cmd == "switchActive")
	{
		value = "1";
		SendCallback(UpdateType::SwitchOn);
	} else if (cmd == "switchInactive")
	{
		value = "0";
		SendCallback(UpdateType::SwitchOff);
	} else if (cmd == "sliderValue")
	{
		value = data;
		SendCallback(UpdateType::Slider);
	} else if (cmd == "numberValue")
	{
		value = data;
		SendCallback(UpdateType::Number);
	} else if (cmd == "textValue")
	{
		value = data;
		SendCallback(UpdateType::Text);
	} else if (cmd == "tabValue")
		SendCallback(UpdateType::TabValue);
	else if (cmd == "selectValue")
	{
		value = data;
		SendCallback(UpdateType::SelectChanged);
	} else if (cmd == "time")
	{
		value = data;
		SendCallback(UpdateType::Time);
	}
}
