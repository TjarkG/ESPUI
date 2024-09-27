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

	Fragment = 98,
	Password = 99,
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
	const char *label;
	std::function<void(Control *, int)> callback;
	String value;
	ControlColor color;
	bool visible;
	bool wide {false};
	bool vertical {false};
	bool enabled {true};
	std::shared_ptr<Control>  parentControl;
	String panelStyle;
	String elementStyle;
	String inputType;

	static constexpr uint16_t noParent = 0xffff;

	Control(ControlType type,
	        const char *label,
	        std::function<void(Control *, int)> callback,
	        String value,
	        ControlColor color,
	        bool visible,
	        const std::shared_ptr<Control>& parentControl);

	Control(const Control &Control);

	void SendCallback(int type);

	bool HasCallback() const { return nullptr != callback; }

	bool MarshalControl(const JsonObject &item, bool refresh, uint32_t DataOffset, uint32_t MaxLength,
	                    uint32_t &EstimatedUsedSpace, const ESPUIClass &ui) const;

	void MarshalErrorMessage(const JsonObject &item) const;

	void onWsEvent(const String &cmd, const String &data, ESPUIClass &ui);

	bool NeedsSync(const uint32_t lastControlChangeID) const
	{
		return (lastControlChangeID < ControlChangeID);
	}

	void SetControlChangedId(const uint32_t value) { ControlChangeID = value; }

private:
	uint32_t ControlChangeID = 0;
	String OldValue = emptyString;

	// multiplier for converting a typical controller label or value to a Json object
	static constexpr auto JsonMarshalingRatio {3};
	// Marshaled Control overhead length
	static constexpr auto JsonMarshaledOverhead {64};
};
