// Server-side entry point for a single test mission.
class MUR_DroneLaunch
{
	static IEntity LaunchNear(IEntity player, vector scoutPoint)
	{
		if (!Replication.IsServer() || !player)
			return null;

		BaseWorld world = GetGame().GetWorld();
		vector playerPosition = player.GetOrigin();
		array<vector> offsets = {"2 0 0", "-2 0 0", "0 0 2", "0 0 -2", "4 0 0", "-4 0 0", "0 0 4", "0 0 -4"};
		vector spawnPoint;
		bool foundClearSpot;
		foreach (vector offset : offsets)
		{
			vector candidate = playerPosition + offset;
			candidate[1] = world.GetSurfaceY(candidate[0], candidate[2]) + 0.25;
			if (Math.AbsFloat(candidate[1] - playerPosition[1]) > 2.0)
				continue;
			vector displacement = scoutPoint - candidate;
			displacement[1] = 0;
			if (displacement.Length() > 2000.0)
				continue;
			TraceSphere clearance = new TraceSphere();
			clearance.Radius = 0.2;
			clearance.Start = candidate + "0 2 0";
			clearance.End = candidate;
			clearance.Flags = TraceFlags.DEFAULT | TraceFlags.ANY_CONTACT;
			if (world.TraceMove(clearance, null) < 1.0)
				continue;
			spawnPoint = candidate;
			foundClearSpot = true;
			break;
		}
		if (!foundClearSpot)
		{
			Print("MUR launch rejected: no nearby clear launch position in range");
			return null;
		}

		Resource prefab = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		if (!prefab || !prefab.IsValid())
		{
			Print("MUR launch rejected: drone prefab unavailable");
			return null;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = spawnPoint;
		IEntity drone = GetGame().SpawnEntityPrefab(prefab, world, params);
		if (!drone)
		{
			Print("MUR launch rejected: drone could not spawn");
			return null;
		}

		MUR_DroneFlightComponent flight = MUR_DroneFlightComponent.Cast(drone.FindComponent(MUR_DroneFlightComponent));
		if (!flight || !flight.StartFlight(scoutPoint))
		{
			Print("MUR launch rejected: flight component could not start");
			RplComponent.DeleteRplEntity(drone, false);
			return null;
		}
		return drone;
	}
}
