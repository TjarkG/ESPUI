//
// Created by tjark on 14.10.24.
//
#include "ESPUI.hpp"

#include "Label.hpp"

Label::Label(std::string heading, std::string buttonLable, const ControlColor color_in):
	text(std::move(buttonLable))
{
	label = std::move(heading);
	color = color_in;
	id = ++idCounter;
}

void Label::MarshalControl(const JsonObject &item, const bool refresh) const
{
	MarshalControlBasic(item, refresh);
	item["type"] = static_cast<uint32_t>(ControlType::Label) + (refresh ? 100 : 0);
	item["value"] = text;
}

void Label::onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui)
{
	SetControlChangedId(ui.GetNextControlChangeId());
}


void Label::set(const std::string &label) {
	text = label;
	notifyParent();
}
