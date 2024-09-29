#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

class ESPUIClass;

enum class ControlType : uint8_t
{
	// fixed Controls
	Title = 0,

	// updatable Controls
	Pad,
	PadWithCenter,
	Button,
	Label,
	Switcher,
	Slider,
	Number,
	Text,
	Graph,
	GraphPoint,
	Tab,
	Select,
	Option,
	Min,
	Max,
	Step,
	Gauge,
	Accel,
	Separator,
	Time,
	FileDisplay,

	Fragment     = 98,
	Password     = 99,
	UpdateOffset = 100,
};

enum class ControlColor : uint8_t
{
	Turquoise,
	Green,
	Blue,
	Gray,
	Yellow,
	Orange,
	Red,
	Black,
	None = 0xFF
};

class Control
{
public:
	ControlType type;
	uint16_t id; // just mirroring the id here for practical reasons
	std::string label;
	std::function<void(Control *, int)> callback;
	std::string value;
	ControlColor color;
	bool visible;
	bool wide {false};
	bool vertical {false};
	bool enabled {true};
	std::shared_ptr<Control> parentControl;
	std::string panelStyle;
	std::string elementStyle;
	std::string inputType;

	static constexpr uint16_t noParent = 0xffff;

	Control(ControlType type,
	        std::string label,
	        std::function<void(Control *, int)> callback,
	        std::string value,
	        ControlColor color,
	        bool visible,
	        const std::shared_ptr<Control> &parentControl);

	Control(const Control &Control);

	void SendCallback(const int type)
	{
		if (callback)
			callback(this, type);
	}

	bool MarshalControl(const JsonObject &item, bool refresh, uint32_t DataOffset, uint32_t MaxLength,
	                    uint32_t &EstimatedUsedSpace) const;

	void MarshalErrorMessage(const JsonObject &item) const;

	void onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui);

	bool NeedsSync(const uint32_t lastControlChangeID) const
	{
		return lastControlChangeID < ControlChangeID;
	}

	void SetControlChangedId(const uint32_t value)
	{
		ControlChangeID = value;
	}

private:
	uint32_t ControlChangeID = 0;
	std::string OldValue;

	// multiplier for converting a typical controller label or value to a Json object
	static constexpr auto JsonMarshalingRatio {3};
	// Marshaled Control overhead length
	static constexpr auto JsonMarshaledOverhead {64};
};
