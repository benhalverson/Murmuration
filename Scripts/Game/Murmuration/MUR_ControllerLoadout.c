// Give players the prototype controller as part of their initial loadout.
modded class SCR_BaseGameMode
{
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		super.OnPlayerSpawned(playerId, controlledEntity);
		if (Replication.IsServer() && controlledEntity)
			GetGame().GetCallqueue().CallLater(MUR_GivePlayerController, 1000, false, controlledEntity);
	}

	protected void MUR_GivePlayerController(IEntity player)
	{
		if (!player || SCR_PlayerController.MUR_HasController(player)) return;
		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(player.FindComponent(InventoryStorageManagerComponent));
		if (!inventory || !inventory.TrySpawnPrefabToStorage("{A39802E278DE0200}Prefabs/MUR_DroneController.et"))
			Print("MUR controller could not be added to the player's inventory", LogLevel.ERROR);
		else
			Print("MUR controller added to the player's inventory");
	}
}
