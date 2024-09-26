#include "ESPUI.h"
#include "ESPUIclient.h"

// FSM definitions
void fsm_EspuiClient_state::Init()
{
	Parent->pCurrentFsmState = this;
}

bool fsm_EspuiClient_state_Idle::NotifyClient()
{
	bool Response = false;

	const ESPUIclient::ClientUpdateType_t TypeToProcess = Parent->ClientUpdateType;
	// Clear the type so that we capture any changes in type that happen
	// while we are processing the current request.
	Parent->ClientUpdateType = ESPUIclient::ClientUpdateType_t::Synchronized;

	// Start processing the current request.
	switch (TypeToProcess)
	{
		case ESPUIclient::ClientUpdateType_t::Synchronized:
		{
			// Parent->fsm_EspuiClient_state_Idle_imp.Init();
			Response = true; // Parent->SendClientNotification(ESPUIclient::ClientUpdateType_t::UpdateNeeded);
			break;
		}
		case ESPUIclient::ClientUpdateType_t::UpdateNeeded:
		{
			Parent->fsm_EspuiClient_state_SendingUpdate_imp.Init();
			Response = Parent->SendClientNotification(ESPUIclient::ClientUpdateType_t::UpdateNeeded);
			break;
		}
		case ESPUIclient::ClientUpdateType_t::RebuildNeeded:
		{
			Parent->fsm_EspuiClient_state_Rebuilding_imp.Init();
			Response = Parent->SendClientNotification(ESPUIclient::ClientUpdateType_t::RebuildNeeded);
			break;
		}
		case ESPUIclient::ClientUpdateType_t::ReloadNeeded:
		{
			Parent->fsm_EspuiClient_state_Reloading_imp.Init();
			Response = Parent->SendClientNotification(ESPUIclient::ClientUpdateType_t::ReloadNeeded);
			break;
		}
	}
	return Response;
}

void fsm_EspuiClient_state_Idle::ProcessAck(const uint16_t ControlIndex, const String FragmentRequestString)
{
	if (!emptyString.equals(FragmentRequestString))
	{
		Parent->SendControlsToClient(ControlIndex, ESPUIclient::ClientUpdateType_t::UpdateNeeded,
		                             FragmentRequestString);
	} else
	{
		// This is an unexpected request for control data from the browser
		// treat it as if it was a rebuild operation
		Parent->NotifyClient(ESPUIclient::ClientUpdateType_t::RebuildNeeded);
	}
}

bool fsm_EspuiClient_state_SendingUpdate::NotifyClient()
{
	return true; /* Ignore request */
}

void fsm_EspuiClient_state_SendingUpdate::ProcessAck(const uint16_t ControlIndex, const String FragmentRequest)
{
	if (Parent->SendControlsToClient(ControlIndex, ESPUIclient::ClientUpdateType_t::UpdateNeeded, FragmentRequest))
	{
		// No more data to send. Go back to idle or start next request
		Parent->fsm_EspuiClient_state_Idle_imp.Init();
		Parent->fsm_EspuiClient_state_Idle_imp.NotifyClient();
	}
}

void fsm_EspuiClient_state_Rebuilding::Init()
{
	Parent->CurrentSyncID = 0;
	Parent->NextSyncID = 0;
	Parent->pCurrentFsmState = this;
}

bool fsm_EspuiClient_state_Rebuilding::NotifyClient()
{
	return true; /* Ignore request */
}

void fsm_EspuiClient_state_Rebuilding::ProcessAck(const uint16_t ControlIndex, const String FragmentRequest)
{
	if (Parent->SendControlsToClient(ControlIndex, ESPUIclient::ClientUpdateType_t::RebuildNeeded, FragmentRequest))
	{
		// No more data to send. Go back to idle or start next request
		Parent->fsm_EspuiClient_state_Idle_imp.Init();
		Parent->fsm_EspuiClient_state_Idle_imp.NotifyClient();
	}
}

void fsm_EspuiClient_state_Reloading::Init()
{
	Parent->CurrentSyncID = 0;
	Parent->NextSyncID = 0;
	Parent->pCurrentFsmState = this;
}

void fsm_EspuiClient_state_Reloading::ProcessAck(const uint16_t ControlIndex, const String FragmentRequestString)
{
	if (!emptyString.equals(FragmentRequestString))
	{
		Parent->SendControlsToClient(ControlIndex, ESPUIclient::ClientUpdateType_t::UpdateNeeded,
		                             FragmentRequestString);
	}
}

bool fsm_EspuiClient_state_Reloading::NotifyClient()
{
	return true; /* Ignore request */
}
