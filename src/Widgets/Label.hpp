//
// Created by tjark on 14.10.24.
//

#ifndef LABEL_HPP
#define LABEL_HPP

#include "ESPUIcontrol.hpp"

class Label final : public Widget
{
	std::string text;

public:
	explicit Label(std::string heading, std::string text, ControlColor color_in = ControlColor::None);

	void MarshalControl(const JsonObject &item, bool refresh) const override;

	void onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui) override;

	// set contend
	void set(const std::string &label);

	// get current contend
	std::string get() { return text; }
};


#endif //LABEL_HPP
