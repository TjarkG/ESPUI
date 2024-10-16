//
// Created by tjark on 07.10.24.
//

#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "ESPUIcontrol.hpp"

class Button final : public Widget
{
	bool state {false};
	std::string b_label;
	std::function<void(Button &)> callback;

public:
	explicit Button(std::string heading, std::string buttonLable, ControlColor color_in = ControlColor::None,
		   std::function<void(Button &)>  = nullptr);

	void MarshalControl(const JsonObject &item, bool refresh) const override;

	void onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui) override;

	//get Button State. true if button is pressed
	bool getState() const { return state; }

	// set Button label
	void setButtonLabel(const std::string &newLabel);
};

#endif //BUTTON_HPP
