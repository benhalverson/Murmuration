// Marks the inventory item that permits requesting a swarm.
class MUR_DroneControllerComponentClass : ScriptComponentClass {}
class MUR_DroneControllerComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		InventoryItemComponent item = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
		if (item && item.GetUIInfo())
			item.GetUIInfo().SetName("Drone controller");
	}
}
