#include <utility>

#include "ESPUI.hpp"

uint16_t idCounter = 0;

Widget::Widget(const ControlType type, std::string label, std::string value, const ControlColor color,
               std::function<void(Widget *, UpdateType)> callback, const std::shared_ptr<Widget> &parentControl):
	parentControl(parentControl),
	type_l(type),
	label(std::move(label)),
	callback_l(std::move(callback)),
	value_l(std::move(value)),
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

void Widget::setLabel(const std::string &label_in)
{
	label = label_in;
	notifyParent();
}

void Widget::setStyle(const std::string &style)
{
	elementStyle = style;
	notifyParent();
}

void Widget::setPanelStyle(const std::string &style)
{
	panelStyle = style;
	notifyParent();
}

void Widget::setWide(const bool wide_in)
{
	wide = wide_in;
	notifyParent();
}

void Widget::setEnabled(const bool wide_in)
{
	wide = wide_in;
	notifyParent();
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

void Widget::MarshalControlBasic(const JsonObject &item, const bool refresh) const
{
	// this code assumes MaxMarshaledLength > JsonMarshalingRatio
	if (refresh && type_l == ControlType::Tab)
		return;

	item["id"] = id;
	item["label"] = label;
	item["visible"] = true;
	item["color"] = static_cast<int>(color);
	item["enabled"] = enabled;

	if (!panelStyle.empty()) { item["panelStyle"] = panelStyle; }
	if (!elementStyle.empty()) { item["elementStyle"] = elementStyle; }
	if (!inputType.empty()) { item["inputType"] = inputType; }
	if (wide) { item["wide"] = true; }
	if (const std::shared_ptr<Widget> parent = parentControl.lock())
		item["parentControl"] = parent->id;
}

void Widget::MarshalControl(const JsonObject &item, const bool refresh) const
{
	ControlType TempType = ControlType::Password == type_l ? ControlType::Text : type_l;
	item["type"] = static_cast<uint32_t>(TempType) + (refresh ? 100 : 0);

	item["value"] = value_l;

	// special case for selects: to preselect an option, you have to add
	// "selected" to <option>
	if (ControlType::Option == type_l)
	{
		if (const std::shared_ptr<Widget> parent = parentControl.lock(); parent && parent->value_l == value_l)
		{
			item["selected"] = "selected";
		} else
		{
			item["selected"] = "";
		}
	}

	MarshalControlBasic(item, refresh);
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
		value_l = "1";
		SendCallback(UpdateType::SwitchOn);
	} else if (cmd == "switchInactive")
	{
		value_l = "0";
		SendCallback(UpdateType::SwitchOff);
	} else if (cmd == "sliderValue")
	{
		value_l = data;
		SendCallback(UpdateType::Slider);
	} else if (cmd == "numberValue")
	{
		value_l = data;
		SendCallback(UpdateType::Number);
	} else if (cmd == "textValue")
	{
		value_l = data;
		SendCallback(UpdateType::Text);
	} else if (cmd == "tabValue")
		SendCallback(UpdateType::TabValue);
	else if (cmd == "selectValue")
	{
		value_l = data;
		SendCallback(UpdateType::SelectChanged);
	} else if (cmd == "time")
	{
		value_l = data;
		SendCallback(UpdateType::Time);
	}
}
