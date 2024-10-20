//
// Created by tjark on 17.10.24.
//

#ifndef ENTRY_HPP
#define ENTRY_HPP

#include "ESPUIcontrol.hpp"

class Entry final : public Widget
{
	std::string value;
	std::string inputType;
	int maxLen {};
	std::function<void(Entry &)> callback;

public:
	explicit Entry(std::string heading, std::string startValue, ControlColor color_in = ControlColor::None,
	               std::function<void(Entry &)>  = nullptr);

	void MarshalControl(const JsonObject &item, bool refresh) const override;

	void onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui) override;

	void setInputType(const std::string &type);

	std::string getInputType() const { return inputType; }

	void setMaxLen(int len);

	int getMaxLen() const { return maxLen; }

	void set(const std::string &newValue);

	std::string get() const { return value; }
};


#endif //ENTRY_HPP
