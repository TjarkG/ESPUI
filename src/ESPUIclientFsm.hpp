#pragma once

#include <Arduino.h>

// forward declaration
class esp_ui_client;

class fsm_EspuiClient_state
{
public:
	fsm_EspuiClient_state() = default;

	virtual ~fsm_EspuiClient_state() = default;

	virtual void Init();

	virtual bool NotifyClient() = 0;

	virtual void ProcessAck(uint16_t id, String FragmentRequest) = 0;

	void SetParent(esp_ui_client *value) { Parent = value; }

protected:
	esp_ui_client *Parent = nullptr;
};

class fsm_EspuiClient_state_Idle final : public fsm_EspuiClient_state
{
public:
	fsm_EspuiClient_state_Idle() = default;

	~fsm_EspuiClient_state_Idle() override = default;

	bool NotifyClient() override;

	void ProcessAck(uint16_t id, String FragmentRequest) override;
};

class fsm_EspuiClient_state_SendingUpdate final : public fsm_EspuiClient_state
{
public:
	fsm_EspuiClient_state_SendingUpdate() = default;

	~fsm_EspuiClient_state_SendingUpdate() override = default;

	bool NotifyClient() override;

	void ProcessAck(uint16_t id, String FragmentRequest) override;
};

class fsm_EspuiClient_state_Rebuilding final : public fsm_EspuiClient_state
{
public:
	fsm_EspuiClient_state_Rebuilding() = default;

	~fsm_EspuiClient_state_Rebuilding() override = default;

	void Init() override;

	bool NotifyClient() override;

	void ProcessAck(uint16_t id, String FragmentRequest) override;
};

class fsm_EspuiClient_state_Reloading final : public fsm_EspuiClient_state
{
public:
	fsm_EspuiClient_state_Reloading() = default;

	~fsm_EspuiClient_state_Reloading() override = default;

	void Init() override;

	bool NotifyClient() override;

	void ProcessAck(uint16_t id, String FragmentRequest) override;
};
