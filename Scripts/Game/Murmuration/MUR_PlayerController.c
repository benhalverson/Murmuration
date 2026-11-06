class MUR_ControllerMissionListener : MUR_SwarmMissionListener
{
	protected SCR_PlayerController m_Controller;

	void MUR_ControllerMissionListener(SCR_PlayerController controller)
	{
		m_Controller = controller;
	}

	override void OnMissionStatus(string message, bool active)
	{
		if (m_Controller) m_Controller.MUR_SendStatus(message, active);
	}

	override bool IsRequesterValid(IEntity player)
	{
		return m_Controller && player && m_Controller.GetControlledEntity() == player;
	}
}

modded class SCR_PlayerController
{
	protected ref MUR_SwarmMission m_MURMission;
	protected ref MUR_ControllerMissionListener m_MURListener;
	protected bool m_bMURActive;
	protected string m_sMURStatus;
	static ref ScriptInvoker s_MURStatusChanged = new ScriptInvoker();

	static bool MUR_HasController(IEntity player)
	{
		if (!player) return false;
		SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.GetGadgetManager(player);
		if (gadgets)
		{
			IEntity held = gadgets.GetHeldGadget();
			if (held && held.FindComponent(MUR_DroneControllerComponent)) return true;
		}
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(player.FindComponent(InventoryStorageManagerComponent));
		if (!inventory) return false;
		array<typename> required = {MUR_DroneControllerComponent};
		return inventory.FindItemWithComponents(required, EStoragePurpose.PURPOSE_ANY) != null;
	}

	bool MUR_IsActive() { return m_bMURActive; }
	string MUR_GetStatus() { return m_sMURStatus; }

	void MUR_RequestLaunch(vector scoutPoint)
	{
		if (m_bMURActive) return;
		if (Replication.IsServer())
		{
			RpcAsk_MURLaunch(scoutPoint);
			return;
		}
		m_bMURActive = true;
		m_sMURStatus = "Requesting launch...";
		s_MURStatusChanged.Invoke(m_sMURStatus, m_bMURActive);
		Rpc(RpcAsk_MURLaunch, scoutPoint);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_MURLaunch(vector scoutPoint)
	{
		if (m_bMURActive)
		{
			MUR_SendStatus("A swarm mission is already active.", true);
			return;
		}
		IEntity player = GetControlledEntity();
		if (!MUR_HasController(player))
		{
			MUR_SendStatus("Equip a drone controller before launching.", false);
			return;
		}
		m_MURListener = new MUR_ControllerMissionListener(this);
		m_MURMission = new MUR_SwarmMission();
		m_MURMission.Start(player, scoutPoint, m_MURListener);
	}

	void MUR_SendStatus(string message, bool active)
	{
		if (!Replication.IsServer()) return;
		PrintFormat("MUR swarm: %1", message);
		m_sMURStatus = message;
		m_bMURActive = active;
		Rpc(RpcDo_MURStatus, message, active);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_MURStatus(string message, bool active)
	{
		m_sMURStatus = message;
		m_bMURActive = active;
		s_MURStatusChanged.Invoke(message, active);
	}
}
