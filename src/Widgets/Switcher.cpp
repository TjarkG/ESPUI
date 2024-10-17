//
// Created by tjark on 14.10.24.
//
#include "ESPUI.hpp"

#include "Switcher.hpp"

Switcher::Switcher(std::string heading, const ControlColor color_in,
                   std::function<void(Switcher &)> callback, bool startState):
	state(startState), callback(std::move(callback))
{
	label = std::move(heading);
	color = color_in;
	id = ++idCounter;
}

void Switcher::MarshalControl(const JsonObject &item, const bool refresh) const
{
	MarshalControlBasic(item, refresh);
	item["type"] = static_cast<uint32_t>(ControlType::Switcher) + (refresh ? 100 : 0);
	item["value"] = state ? 1 : 0;
	if (vertical)
		item["vertical"] = true;
}

void Switcher::onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui)
{
	SetControlChangedId(ESPUIClass::GetNextControlChangeId());

	if (cmd == "switchActive")
		state = true;
	else if (cmd == "switchInactive")
		state = false;

	if (callback)
		callback(*this);
}

void Switcher::set(const bool state_in)
{
	state = state_in;
	notifyParent();
}

void Switcher::setVertical(const bool ver)
{
	vertical = ver;
	notifyParent();
}
