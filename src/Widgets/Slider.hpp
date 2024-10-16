//
// Created by tjark on 15.10.24.
//

#ifndef SLIDER_HPP
#define SLIDER_HPP

#include "ESPUIcontrol.hpp"

class Slider final : public Widget
{
	int position {};
    int min {0};
    int max {100};
	bool vertical {false};
	std::function<void(Slider &)> callback;

public:
	explicit Slider(std::string heading, ControlColor color_in = ControlColor::None,
		   std::function<void(Slider &)>  = nullptr, int min = 0, int max = 100, int startPos = 50);

	void MarshalControl(const JsonObject &item, bool refresh) const override;

	void onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui) override;

	//get Slider Position
	int getPosition() const { return position; }

	// set Slider Position
	void set(int pos);

	void setVertical(bool ver = true);

	bool isVertical() const { return vertical; }

	//get Minimum Slider Position
	int getMin() const { return position; }

	// set Minimum Slider Position
	void setMin(int pos);

	//get Maximum Slider Position
	int getMax() const { return position; }

	// set Maximum Slider Position
	void setMax(int pos);
};



#endif //SLIDER_HPP
