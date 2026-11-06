class MUR_DroneLaunchSite
{
	vector position;
	float touchdownDrop;
}

// Server-side launch-site selection and aircraft spawning.
class MUR_DroneLaunch
{
	protected static const float SPAWN_GROUND_CLEARANCE = 0.25;
	// The imported mesh extends approximately 0.008 m below its origin.
	protected static const float TOUCHDOWN_GROUND_CLEARANCE = 0.01;

	static IEntity LaunchNear(IEntity player, vector scoutPoint)
	{
		array<ref MUR_DroneLaunchSite> sites = {};
		if (!FindSites(player, scoutPoint, 1, sites))
			return null;
		return LaunchAt(sites[0], scoutPoint, null);
	}

	static bool FindSites(IEntity player, vector scoutPoint, int count, notnull array<ref MUR_DroneLaunchSite> sites)
	{
		sites.Clear();
		if (!Replication.IsServer() || !player || count < 1 || count > 3)
			return false;
		BaseWorld world = GetGame().GetWorld();
		vector playerPosition = player.GetOrigin();
		array<vector> offsets = {"2 0 0", "-2 0 0", "0 0 2", "0 0 -2", "4 0 0", "-4 0 0", "0 0 4", "0 0 -4"};
		foreach (vector offset : offsets)
		{
			vector candidate = playerPosition + offset;
			candidate[1] = world.GetSurfaceY(candidate[0], candidate[2]) + SPAWN_GROUND_CLEARANCE;
			if (Math.AbsFloat(candidate[1] - playerPosition[1]) > 2.0)
				continue;
			// Keep the level mesh above the highest terrain at its footprint corners.
			float highestGround = world.GetSurfaceY(candidate[0], candidate[2]);
			highestGround = Math.Max(highestGround, world.GetSurfaceY(candidate[0] - 0.15, candidate[2] - 0.135));
			highestGround = Math.Max(highestGround, world.GetSurfaceY(candidate[0] - 0.15, candidate[2] + 0.135));
			highestGround = Math.Max(highestGround, world.GetSurfaceY(candidate[0] + 0.15, candidate[2] - 0.135));
			highestGround = Math.Max(highestGround, world.GetSurfaceY(candidate[0] + 0.15, candidate[2] + 0.135));
			float touchdownHeight = highestGround + TOUCHDOWN_GROUND_CLEARANCE;
			if (touchdownHeight >= candidate[1])
				continue;
			vector displacement = scoutPoint - candidate;
			displacement[1] = 0;
			if (!(displacement.Length() <= 2000.0))
				continue;
			MUR_DroneLaunchSite site = new MUR_DroneLaunchSite();
			site.position = candidate;
			site.touchdownDrop = candidate[1] - touchdownHeight;
			if (!IsSiteClear(site, scoutPoint))
				continue;
			sites.Insert(site);
			if (sites.Count() == count)
				return true;
		}
		sites.Clear();
		return false;
	}

	static bool IsSiteClear(MUR_DroneLaunchSite site, vector scoutPoint)
	{
		if (!site) return false;
		TraceSphere clearance = new TraceSphere();
		clearance.Radius = 0.2;
		MUR_FlightRoute candidateRoute = new MUR_FlightRoute(site.position, scoutPoint);
		clearance.Start = candidateRoute.GetDestination();
		clearance.End = site.position;
		clearance.Flags = TraceFlags.DEFAULT | TraceFlags.ANY_CONTACT;
		return GetGame().GetWorld().TraceMove(clearance, null) >= 1.0;
	}

	static IEntity LaunchAt(MUR_DroneLaunchSite site, vector scoutPoint, array<vector> waypoints)
	{
		if (!Replication.IsServer() || !site || !IsSiteClear(site, scoutPoint))
			return null;
		BaseWorld world = GetGame().GetWorld();
		Resource prefab = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		if (!prefab || !prefab.IsValid())
		{
			Print("MUR launch rejected: drone prefab unavailable");
			return null;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = site.position;
		IEntity drone = GetGame().SpawnEntityPrefab(prefab, world, params);
		if (!drone)
		{
			Print("MUR launch rejected: drone could not spawn");
			return null;
		}

		MUR_DroneFlightComponent flight = MUR_DroneFlightComponent.Cast(drone.FindComponent(MUR_DroneFlightComponent));
		if (!flight || !flight.StartFlight(scoutPoint, site.touchdownDrop, waypoints))
		{
			Print("MUR launch rejected: flight component could not start");
			RplComponent.DeleteRplEntity(drone, false);
			return null;
		}
		return drone;
	}
}
