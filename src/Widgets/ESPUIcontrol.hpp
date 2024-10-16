#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

extern uint16_t idCounter;

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

class Widget : public std::enable_shared_from_this<Widget>
{
protected:
	//weak pointer to make sure Controls with Children are deleted correctly
	std::weak_ptr<Widget> parentControl;
	std::vector<std::shared_ptr<Widget> > children;

	Widget() = default;

public:
	ControlType type_l = ControlType::Title;
	uint16_t id = 0;
	std::string label;
	std::function<void(Widget *, UpdateType)> callback_l;
	std::string value_l;
	ControlColor color = ControlColor::None;
	bool wide {false};
	bool enabled {true};
	std::string panelStyle;
	std::string elementStyle;
	std::string inputType;

	static constexpr uint16_t noParent = 0xffff;

	Widget(ControlType type, std::string label, std::string value = "",
	       ControlColor color = ControlColor::None,
	       std::function<void(Widget *, UpdateType)> callback = nullptr,
	       const std::shared_ptr<Widget> &parentControl = nullptr);

	Widget(const Widget &Control) = default;

	virtual ~Widget() = default;

	void SendCallback(const UpdateType CallbackType)
	{
		if (callback_l)
			callback_l(this, CallbackType);
	}

	void MarshalControlBasic(const JsonObject &item, bool refresh) const;

	virtual void MarshalControl(const JsonObject &item, bool refresh) const;

	virtual void onWsEvent(const std::string &cmd, const std::string &data, ESPUIClass &ui);

	bool NeedsSync(const uint32_t lastControlChangeID) const
	{
		return lastControlChangeID < ControlChangeID;
	}

	void SetControlChangedId(const uint32_t changeId)
	{
		ControlChangeID = changeId;
	}

	//Add Child Control TODO remove
	std::shared_ptr<Widget> add(ControlType type, const std::string &label = "", const std::string &value = "",
	                            ControlColor color = ControlColor::None,
	                            const std::function<void(Widget *, UpdateType)> &callback = nullptr);

	//Add Child Control
	template<class T>
	std::shared_ptr<T> add(const T &widget)
	{
		auto ptr = std::make_shared<T>(widget);
		ptr->parentControl = shared_from_this();
		children.push_back(ptr);
		notifyParent();
		return ptr;
	}

	//Remove this Control
	void remove() const;

	//find control with id, return pointer to it or nullptr
	std::shared_ptr<Widget> find(uint16_t id_in);

	// get number of children and grandchildren, excluding this node
	size_t getChildCount() const;

	//get a vector of shared pointers to all children and grandchildren
	std::vector<std::shared_ptr<Widget> > getChildren() const;

	//Common Edit functions

	void setLabel(const std::string &label_in);

	void setStyle(const std::string &style);

	void setPanelStyle(const std::string &style);

	void setWide(bool wide_in = true);

	void setEnabled(bool wide_in = true);

protected:
	// notify parent that a widget change has occurred
	virtual void notifyParent() const;

	uint32_t ControlChangeID = 1;

	// multiplier for converting a typical controller label or value to a Json object
	static constexpr auto JsonMarshalingRatio {3};
	// Marshaled Control overhead length
	static constexpr auto JsonMarshaledOverhead {64};
};

class RootWidget final : public Widget
{
	ESPUIClass &ui;

public:
	explicit RootWidget(ESPUIClass &ui):
		Widget(), ui(ui) {}

	// Root Node can't be deleted by user
	void remove() = delete;

	// notify ui that a widget change has occurred. will be called from all other NotifyParent Functions
	void notifyParent() const override;
};
