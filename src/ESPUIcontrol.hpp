#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

class ESPUIClass;

enum class UpdateType : uint8_t;

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

class Control : public std::enable_shared_from_this<Control>
{
protected:
	ESPUIClass *ui {};
	std::shared_ptr<Control> parentControl;
	std::vector<std::shared_ptr<Control> > children;

public:
	ControlType type = ControlType::Title;
	uint16_t id = 0;
	std::string label;
	std::function<void(Control *, UpdateType)> callback;
	std::string value;
	ControlColor color = ControlColor::None;
	bool visible {true};
	bool wide {false};
	bool vertical {false};
	bool enabled {true};
	std::string panelStyle;
	std::string elementStyle;
	std::string inputType;

	static constexpr uint16_t noParent = 0xffff;

	explicit Control(ESPUIClass *ui): ui(ui) {}

	Control(ControlType type, std::string label, std::function<void(Control *, UpdateType)> callback, std::string value,
	        ControlColor color, bool visible, const std::shared_ptr<Control> &parentControl, ESPUIClass *ui);

	Control(const Control &Control);

	void SendCallback(const UpdateType type)
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

	//Add Child Control
	std::shared_ptr<Control> add(ControlType type, const std::string &label = "", const std::string &value = "",
	                             ControlColor color = ControlColor::None,
	                             const std::function<void(Control *, UpdateType)> &callback = nullptr,
	                             bool visible = true);

	//Remove this Control
	void remove(bool force_rebuild_ui = false) const;

	//find control with id, return pointer to it or nullptr
	std::shared_ptr<Control> find(uint16_t id_in);

	// get number of children and grandchildren, excluding this node
	size_t getChildCount() const;

	//get a vector of shared pointers to all children and grandchildren
	std::vector<std::shared_ptr<Control> > getChildren() const;

private:
	uint32_t ControlChangeID = 0;
	std::string OldValue;

	// multiplier for converting a typical controller label or value to a Json object
	static constexpr auto JsonMarshalingRatio {3};
	// Marshaled Control overhead length
	static constexpr auto JsonMarshaledOverhead {64};
};
