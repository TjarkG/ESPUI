//
// Created by tjark on 17.10.24.
//

#include "ESPUI.hpp"

#include "Entry.hpp"

Entry::Entry(std::string heading, std::string startValue, const ControlColor color_in,
             std::function<void(Entry &)> callback):
	value(std::move(startValue)), callback(std::move(callback))
{
	label = std::move(heading);
	color = color_in;
	id = ++idCounter;
}

void Entry::MarshalControl(const JsonObject &item, const bool refresh) const
{
	MarshalControlBasic(item, refresh);
	item["type"] = static_cast<uint32_t>(ControlType::Text) + (refresh ? 100 : 0);
	item["value"] = value;
	if (maxLen != 0)
		item["max"] = maxLen;

	if (!inputType.empty())
		item["inputType"] = inputType;
}

void Entry::onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui)
{
	ControlChangeID = ESPUIClass::GetNextControlChangeId();

	if (cmd == "textValue" && maxLen != 0)
		value = data.substr(0, maxLen);
	else if (cmd == "textValue")
		value = data;

	if (callback)
		callback(*this);
}

void Entry::setInputType(const std::string &type)
{
	inputType = type;
	notifyParent();
}

void Entry::setMaxLen(const int len)
{
	maxLen = len;
	notifyParent();
}

void Entry::set(const std::string &newValue)
{
	value = newValue;
	notifyParent();
}
