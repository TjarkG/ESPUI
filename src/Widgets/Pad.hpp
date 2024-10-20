//
// Created by tjark on 17.10.24.
//

#ifndef PAD_HPP
#define PAD_HPP

#include "ESPUIcontrol.hpp"

class Pad final : public Widget
{
	bool centerVisible;
	std::function<void(Pad &)> callback;

	bool forward {false};
	bool back {false};
	bool left {false};
	bool right {false};
	bool center {false};

public:
	explicit Pad(std::string heading, ControlColor color_in = ControlColor::None,
	             std::function<void(Pad &)>  = nullptr, bool hasCenter = false);

	void MarshalControl(const JsonObject &item, bool refresh) const override;

	void onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui) override;

	void setHasCenter(bool hasCenter);

	bool hasCenter() const { return centerVisible; }

	bool getForward() const { return forward; }

	bool getBack() const { return back; }

	bool getLeft() const { return left; }

	bool getRight() const { return right; }

	bool getCenter() const { return center; }
};


#endif //PAD_HPP
