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
		string obstacleOption;
		if (System.GetCLIParam("murTestObstacle", obstacleOption) && obstacleOption == "1")
		{
			ChimeraWorld world = GetGame().GetWorld();
			TimeAndWeatherManagerEntity timeManager = world.GetTimeAndWeatherManager();
			if (!timeManager || !timeManager.SetTime(new TimeContainer(12, 0, 0)))
				Print("MUR obstacle test: could not set daylight", LogLevel.ERROR);
		}

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

		vector scoutPoint = controlledEntity.GetOrigin() + offset;
		IEntity drone = MUR_DroneLaunch.LaunchNear(controlledEntity, scoutPoint);
		if (!drone)
		{
			Print("MUR test launch failed: check spawn clearance, scout range, and prefab availability", LogLevel.ERROR);
			return;
		}
		PrintFormat("MUR test drone spawned for player %1 at %2", playerId, drone.GetOrigin());

		string obstacleOption;
		if (!System.GetCLIParam("murTestObstacle", obstacleOption) || obstacleOption != "1")
			return;

		vector launchPoint = drone.GetOrigin();
		vector obstaclePosition = vector.Lerp(launchPoint, scoutPoint, 0.5);
		obstaclePosition[1] = Math.Max(launchPoint[1], scoutPoint[1]) + 10.0;
		Resource obstaclePrefab = Resource.Load("{9019FBEC2B39E7B5}Prefabs/Tests/MUR_Obstacle.et");
		if (!obstaclePrefab || !obstaclePrefab.IsValid())
		{
			Print("MUR obstacle test failed: obstacle prefab unavailable", LogLevel.ERROR);
			return;
		}
		EntitySpawnParams obstacleParams = new EntitySpawnParams();
		obstacleParams.TransformMode = ETransformMode.WORLD;
		obstacleParams.Transform[3] = obstaclePosition;
		IEntity obstacle = GetGame().SpawnEntityPrefab(obstaclePrefab, GetGame().GetWorld(), obstacleParams);
		if (!obstacle)
		{
			Print("MUR obstacle test failed: obstacle could not spawn", LogLevel.ERROR);
			return;
		}
		obstacle.Update();
		GenericEntity.Cast(obstacle).OnTransformReset();
		TraceSphere trace = new TraceSphere();
		trace.Radius = 0.2;
		trace.Start = launchPoint;
		trace.Start[1] = obstaclePosition[1];
		trace.End = scoutPoint;
		trace.End[1] = obstaclePosition[1];
		trace.Exclude = drone;
		trace.Flags = TraceFlags.DEFAULT;
		if (GetGame().GetWorld().TraceMove(trace, null) >= 1.0 || trace.TraceEnt != obstacle)
		{
			Print("MUR obstacle test invalid: crate is not the first obstacle on the route", LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(obstacle);
			RplComponent.DeleteRplEntity(drone, false);
		}
		else
		{
			PrintFormat("MUR obstacle test fixture placed at %1", obstaclePosition);
			GetGame().GetCallqueue().CallLater(MUR_CheckObstacleFlight, 1000, false, drone, obstaclePosition[1], launchPoint[1], 0);
		}
	}

	protected void MUR_CheckObstacleFlight(IEntity drone, float routeAltitude, float maxAltitude, int checks)
	{
		if (!drone)
			return;
		MUR_DroneFlightComponent flight = MUR_DroneFlightComponent.Cast(drone.FindComponent(MUR_DroneFlightComponent));
		if (!flight)
			return;
		vector position = drone.GetOrigin();
		maxAltitude = Math.Max(maxAltitude, position[1]);
		if (flight.IsComplete())
		{
			PrintFormat("MUR obstacle test flight completed: maximum altitude %1, route altitude %2", maxAltitude, routeAltitude);
			return;
		}
		if (flight.IsFailed() || checks >= 60)
		{
			Print("MUR obstacle test flight failed or timed out", LogLevel.ERROR);
			return;
		}
		GetGame().GetCallqueue().CallLater(MUR_CheckObstacleFlight, 1000, false, drone, routeAltitude, maxAltitude, checks + 1);
	}
}
