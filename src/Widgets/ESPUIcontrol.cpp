#include <utility>

#include "ESPUI.hpp"

static const std::string ControlError = "*** ESPUI ERROR: Could not transfer control ***";
uint16_t idCounter = 0;

Widget::Widget(const ControlType type, std::string label, std::string value, const ControlColor color,
               std::function<void(Widget *, UpdateType)> callback, const std::shared_ptr<Widget> &parentControl):
	parentControl(parentControl),
	type(type),
	label(std::move(label)),
	callback(std::move(callback)),
	value(std::move(value)),
	color(color)
{
	id = ++idCounter;
}

std::shared_ptr<Widget> Widget::add(const ControlType type, const std::string &label, const std::string &value,
                                    const ControlColor color,
                                    const std::function<void(Widget *, UpdateType)> &callback)
{
	auto control = std::make_shared<Widget>(type, label, value, color, callback, shared_from_this());

	children.push_back(control);
	notifyParent();
	return control;
}

void Widget::remove() const
{
	const std::shared_ptr<Widget> parent = parentControl.lock();
	const auto it = std::find_if(parent->children.begin(), parent->children.end(),
	                             [this](const std::shared_ptr<Widget> &i)
	                             {
		                             return i.get() == this;
	                             });

	parent->children.erase(it);
	notifyParent();
}

std::shared_ptr<Widget> Widget::find(const uint16_t id_in)
{
	if (id_in == id)
		return shared_from_this();

	for (const auto &child: children)
		if (auto tmp = child->find(id_in); tmp != nullptr)
			return tmp;

	return nullptr;
}

size_t Widget::getChildCount() const
{
	size_t i = children.size();
	for (const auto &child: children)
		i += child->getChildCount();

	return i;
}

std::vector<std::shared_ptr<Widget> > Widget::getChildren() const
{
	auto v = children;
	for (const auto &child: children)
	{
		auto cv = child->getChildren();
		v.insert(v.end(), cv.begin(), cv.end());
	}

	return v;
}

void Widget::notifyParent() const
{
	if (const std::shared_ptr<Widget> parent = parentControl.lock())
		parent->notifyParent();
}

void RootWidget::notifyParent() const
{
	xSemaphoreTake(ui.ControlsSemaphore, portMAX_DELAY);
	ui.NotifyClients(ClientUpdateType_t::RebuildNeeded);
	xSemaphoreGive(ui.ControlsSemaphore);
}

bool Widget::MarshalControl(const JsonObject &item, const bool refresh, const uint32_t DataOffset,
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
	item[F("visible")] = true;
	item[F("color")] = static_cast<int>(color);
	item[F("enabled")] = enabled;

	if (!panelStyle.empty()) { item[F("panelStyle")] = panelStyle; }
	if (!elementStyle.empty()) { item[F("elementStyle")] = elementStyle; }
	if (!inputType.empty()) { item[F("inputType")] = inputType; }
	if (wide == true) { item[F("wide")] = true; }
	if (vertical == true) { item[F("vertical")] = true; }
	const std::shared_ptr<Widget> parent = parentControl.lock();
	if (parent)
		item[F("parentControl")] = std::to_string(parent->id);

	// special case for selects: to preselect an option, you have to add
	// "selected" to <option>
	if (ControlType::Option == type)
	{
		if (!parent)
		{
			item[F("selected")] = emptyString;
		} else if (parent->value == value)
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

void Widget::MarshalErrorMessage(const JsonObject &item) const
{
	item[F("id")] = id;
	item[F("type")] = static_cast<uint32_t>(ControlType::Label);
	item[F("label")] = ControlError;
	item[F("value")] = ControlError;
	item[F("visible")] = true;
	item[F("color")] = static_cast<int>(ControlColor::Orange);
	item[F("enabled")] = true;

	if (const std::shared_ptr<Widget> parent = parentControl.lock())
	{
		item[F("parentControl")] = std::to_string(parent->id);
	}
}

void Widget::onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui)
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


Button::Button(std::string heading, std::string buttonLable, const ControlColor color_in,
               std::function<void(Button &)> callback):
	b_heading(std::move(heading)), b_label(std::move(buttonLable)), callback(std::move(callback))
{
	color = color_in;
	id = ++idCounter;
}

bool Button::MarshalControl(const JsonObject &item, const bool refresh, const uint32_t DataOffset, const uint32_t MaxLength,
	uint32_t &EstimatedUsedSpace) const
{
	// how much space do we expect to use?
	const uint32_t LabelMarshaledLength = b_heading.length() * JsonMarshalingRatio;
	const uint32_t MinimumMarshaledLength = LabelMarshaledLength + JsonMarshaledOverhead;
	const uint32_t SpaceForMarshaledValue = MaxLength - MinimumMarshaledLength;

	// will the item fit in the remaining space? Fragment if not
	if (MaxLength < MinimumMarshaledLength)
	{
		EstimatedUsedSpace = 0;
		return false;
	}

	const uint32_t MaxValueLength = SpaceForMarshaledValue / JsonMarshalingRatio;

	const uint32_t ValueLenToSend = min(b_label.length() - DataOffset, MaxValueLength);

	const uint32_t AdjustedMarshaledLength = ValueLenToSend * JsonMarshalingRatio + MinimumMarshaledLength;

	const bool NeedToFragment = ValueLenToSend < b_label.length();

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
		item[F("total")] = b_label.length();
		item[F("control")] = item;
	}

	item[F("id")] = id;
	if (refresh)
		item[F("type")] = static_cast<uint32_t>(ControlType::Button) + static_cast<uint32_t>(ControlType::UpdateOffset);
	else
		item[F("type")] = static_cast<uint32_t>(ControlType::Button);

	item[F("label")] = b_heading;
	item[F("value")] = b_label.substr(DataOffset, ValueLenToSend);
	item[F("visible")] = true;
	item[F("color")] = static_cast<int>(color);
	item[F("enabled")] = enabled;

	if (!panelStyle.empty()) { item[F("panelStyle")] = panelStyle; }
	if (!elementStyle.empty()) { item[F("elementStyle")] = elementStyle; }
	if (wide == true) { item[F("wide")] = true; }
	if (const std::shared_ptr<Widget> parent = parentControl.lock())
		item[F("parentControl")] = std::to_string(parent->id);

	// indicate that no additional controls should be sent if fragmenting is on
	return NeedToFragment || DataOffset;
}

void Button::onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui)
{
	SetControlChangedId(ui.GetNextControlChangeId());

	if (cmd == "buttonDown")
		state = true;
	else if (cmd == "buttonUP")
		state = false;

	if(callback)
		callback(*this);
}
