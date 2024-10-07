//
// Created by tjark on 07.10.24.
//

#include "ESPUI.hpp"

#include "Button.hpp"

Button::Button(std::string heading, std::string buttonLable, const ControlColor color_in,
               std::function<void(Button &)> callback):
	b_label(std::move(buttonLable)), callback(std::move(callback))
{
	label = std::move(heading);
	color = color_in;
	id = ++idCounter;
}

bool Button::MarshalControl(const JsonObject &item, const bool refresh, const uint32_t DataOffset,
                            const uint32_t MaxLength,
                            uint32_t &EstimatedUsedSpace) const
{
	MarshalControlBasic(item, refresh, DataOffset, MaxLength, EstimatedUsedSpace);
	item["type"] = static_cast<uint32_t>(ControlType::Button) + (refresh ? 100 : 0);
	item["value"] = b_label;
	return false;
}

void Button::onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui)
{
	SetControlChangedId(ui.GetNextControlChangeId());

	if (cmd == "buttonDown")
		state = true;
	else if (cmd == "buttonUP")
		state = false;

	if (callback)
		callback(*this);
}
