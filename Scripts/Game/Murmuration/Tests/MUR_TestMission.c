// Opt-in dedicated-server fixture. The offset is supplied with -murTestScout.
modded class SCR_BaseGameMode
{
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		super.OnPlayerSpawned(playerId, controlledEntity);
		if (!Replication.IsServer() || !controlledEntity)
			return;

		string offsetText;
		if (!System.GetCLIParam("murTestScout", offsetText))
			return;

		// Give the client time to finish deploying before the short flight starts.
		GetGame().GetCallqueue().CallLater(MUR_LaunchTestDrone, 10000, false, playerId, controlledEntity, offsetText);
	}

	protected void MUR_LaunchTestDrone(int playerId, IEntity controlledEntity, string offsetText)
	{
		if (!controlledEntity)
			return;
		vector offset = offsetText.ToVector();
		if (offset.Length() < 1.0)
		{
			Print("MUR test launch rejected: invalid scout offset", LogLevel.ERROR);
			return;
		}

		IEntity drone = MUR_DroneLaunch.LaunchNear(controlledEntity, controlledEntity.GetOrigin() + offset);
		if (!drone)
		{
			Print("MUR test launch failed: check spawn clearance, scout range, and prefab availability", LogLevel.ERROR);
			return;
		}
		PrintFormat("MUR test drone spawned for player %1 at %2", playerId, drone.GetOrigin());
	}
}
