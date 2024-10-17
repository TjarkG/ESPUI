//
// Created by tjark on 15.10.24.
//
#include "ESPUI.hpp"

#include "Slider.hpp"

Slider::Slider(std::string heading, const ControlColor color_in,
               std::function<void(Slider &)> callback, const int min, const int max, const int startPos):
	position(startPos), min(min), max(max), callback(std::move(callback))
{
	label = std::move(heading);
	color = color_in;
	id = ++idCounter;
}

void Slider::MarshalControl(const JsonObject &item, const bool refresh) const
{
	MarshalControlBasic(item, refresh);
	item["type"] = static_cast<uint32_t>(ControlType::Slider) + (refresh ? 100 : 0);
	item["value"] = position;
	item["min"] = min;
	item["max"] = max;
	if (vertical)
		item["vertical"] = true;
}

void Slider::onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui)
{
	ControlChangeID = ESPUIClass::GetNextControlChangeId();

	if (cmd == "sliderValue")
	{
		position = std::stoi(data);

		if (callback)
			callback(*this);
	}
}

void Slider::set(const int pos)
{
	position = pos;
	notifyParent();
}

void Slider::setVertical(const bool ver)
{
	vertical = ver;
	notifyParent();
}

void Slider::setMin(const int pos)
{
	min = pos;
	notifyParent();
}

void Slider::setMax(const int pos)
{
	max = pos;
	notifyParent();
}
