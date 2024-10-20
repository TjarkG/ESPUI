//
// Created by tjark on 17.10.24.
//
#include "ESPUI.hpp"

#include "Pad.hpp"

Pad::Pad(std::string heading, const ControlColor color_in,
         std::function<void(Pad &)> callback, const bool hasCenter):
	centerVisible(hasCenter), callback(std::move(callback))
{
	label = std::move(heading);
	color = color_in;
	id = ++idCounter;
}

void Pad::MarshalControl(const JsonObject &item, const bool refresh) const
{
	MarshalControlBasic(item, refresh);
	item["type"] = static_cast<uint32_t>(centerVisible ? ControlType::PadWithCenter : ControlType::Pad) + (
		               refresh ? 100 : 0);
}

void Pad::onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui)
{
	SetControlChangedId(ESPUIClass::GetNextControlChangeId());

	if (cmd == "padForwardDown")
		forward = true;
	else if (cmd == "padForwardUp")
		forward = false;
	else if (cmd == "padLeftDown")
		left = true;
	else if (cmd == "padLeftUp")
		left = false;
	else if (cmd == "padRightDown")
		right = true;
	else if (cmd == "padRightUp")
		right = false;
	else if (cmd == "padBackDown")
		back = true;
	else if (cmd == "padBackUp")
		back = false;
	else if (cmd == "padCenterDown")
		center = true;
	else if (cmd == "padCenterUp")
		center = false;

	if (callback)
		callback(*this);
}

void Pad::setHasCenter(const bool hasCenter)
{
	centerVisible = hasCenter;
	notifyParent();
}
