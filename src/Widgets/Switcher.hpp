//
// Created by tjark on 14.10.24.
//

#ifndef SWITCHER_HPP
#define SWITCHER_HPP

#include "ESPUIcontrol.hpp"

class Switcher final : public Widget
{
	bool state {false};
	bool vertical {false};
	std::function<void(Switcher &)> callback;

public:
	explicit Switcher(std::string heading, ControlColor color_in = ControlColor::None,
		   std::function<void(Switcher &)>  = nullptr, bool startState = false);

	void MarshalControl(const JsonObject &item, bool refresh) const override;

	void onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui) override;

	//get Switcher State. true if button is pressed
	bool getState() const { return state; }

	// set Switcher State
	void set(bool state_in);

	void setVertical(bool ver = true);

	bool isVertical() const { return vertical; }
};


#endif //SWITCHER_HPP
