[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_VioletPrefab_Spawn_HasFlightComponent : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		Resource prefab = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		if (!prefab || !prefab.IsValid())
		{
			SetFailure("VIOLET prefab could not be loaded.");
			return;
		}

		IEntity drone = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld());
		if (!drone)
		{
			SetFailure("VIOLET prefab could not be spawned.");
			return;
		}

		if (!MUR_DroneFlightComponent.Cast(drone.FindComponent(MUR_DroneFlightComponent)))
			SetFailure("Spawned VIOLET is missing its flight component.");
		SCR_EntityHelper.DeleteEntityAndChildren(drone);
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 25)]
class MUR_VioletFlight_ClearPath_ReturnsToLaunch : SCR_AutotestCaseBase
{
	protected IEntity m_Drone;
	protected MUR_DroneFlightComponent m_Flight;
	protected vector m_vLaunch = "100 100 100";
	protected bool m_bMoved;

	[TestStep(TestStage.Setup)]
	void Setup_Spawn()
	{
		Resource prefab = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		if (!prefab || !prefab.IsValid())
		{
			SetFailure("VIOLET prefab could not be loaded.");
			return;
		}
		m_Drone = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld());
		if (!m_Drone)
		{
			SetFailure("VIOLET prefab could not be spawned.");
			return;
		}
		m_Drone.SetOrigin(m_vLaunch);
		m_Flight = MUR_DroneFlightComponent.Cast(m_Drone.FindComponent(MUR_DroneFlightComponent));
		if (!m_Flight || !m_Flight.StartFlight(m_vLaunch + "2 0 0"))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_Drone);
			m_Drone = null;
			SetFailure("Flight could not start on the server.");
		}
	}

	[TestStep(TestStage.Main)]
	bool Execute_WaitForReturn()
	{
		if (vector.Distance(m_Drone.GetOrigin(), m_vLaunch) > 1)
			m_bMoved = true;
		return m_Flight.IsComplete();
	}

	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		AssertTrue(m_bMoved && vector.Distance(m_Drone.GetOrigin(), m_vLaunch) < 0.2, "Drone must move and return to launch.");
	}

	[TestStep(TestStage.TearDown)]
	void TearDown_DeleteDrone()
	{
		if (m_Drone)
			SCR_EntityHelper.DeleteEntityAndChildren(m_Drone);
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_DroneLaunch_ClearGround_SpawnsNearRequester : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		Resource prefab = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		if (!prefab || !prefab.IsValid())
		{
			SetFailure("Requester fixture could not be loaded.");
			return;
		}
		IEntity requester = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld());
		if (!requester)
		{
			SetFailure("Requester fixture could not be spawned.");
			return;
		}
		vector position = "100 0 100";
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
		requester.SetOrigin(position);
		IEntity drone = MUR_DroneLaunch.LaunchNear(requester, position + "20 0 0");
		if (!drone)
			SetFailure("A clear-ground launch should spawn a drone.");
		else
		{
			AssertTrue(vector.Distance(drone.GetOrigin(), position) < 3, "Drone should spawn near the requester.");
			RplComponent.DeleteRplEntity(drone, false);
		}
		RplComponent.DeleteRplEntity(requester, false);
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_DroneLaunch_OutOfRange_RejectsLaunch : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		Resource prefab = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		if (!prefab || !prefab.IsValid())
		{
			SetFailure("Requester fixture could not be loaded.");
			return;
		}
		IEntity requester = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld());
		if (!requester)
		{
			SetFailure("Requester fixture could not be spawned.");
			return;
		}
		vector position = "100 0 100";
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
		requester.SetOrigin(position);
		IEntity drone = MUR_DroneLaunch.LaunchNear(requester, position + "2005 0 0");
		if (drone)
		{
			RplComponent.DeleteRplEntity(drone, false);
			SetFailure("A scout point more than 2 km from launch must be rejected.");
		}
		RplComponent.DeleteRplEntity(requester, false);
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_DroneLaunch_ElevatedRequester_RejectsTerrainSpawn : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		Resource prefab = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		if (!prefab || !prefab.IsValid())
		{
			SetFailure("Requester fixture could not be loaded.");
			return;
		}
		IEntity requester = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld());
		if (!requester)
		{
			SetFailure("Requester fixture could not be spawned.");
			return;
		}
		vector position = "100 0 100";
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 5;
		requester.SetOrigin(position);
		IEntity drone = MUR_DroneLaunch.LaunchNear(requester, position + "20 0 0");
		if (drone)
		{
			RplComponent.DeleteRplEntity(drone, false);
			SetFailure("Do not spawn on terrain far below the requester.");
		}
		RplComponent.DeleteRplEntity(requester, false);
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_DroneLaunch_BlockedGround_UsesAnotherPosition : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		Resource requesterPrefab = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		Resource obstaclePrefab = Resource.Load("{9019FBEC2B39E7B5}Prefabs/Tests/MUR_Obstacle.et");
		if (!requesterPrefab || !requesterPrefab.IsValid() || !obstaclePrefab || !obstaclePrefab.IsValid())
		{
			SetFailure("Launch fixtures could not be loaded.");
			return;
		}
		IEntity requester = GetGame().SpawnEntityPrefab(requesterPrefab, GetGame().GetWorld());
		IEntity obstacle = GetGame().SpawnEntityPrefab(obstaclePrefab, GetGame().GetWorld());
		if (!requester || !obstacle)
		{
			if (requester) SCR_EntityHelper.DeleteEntityAndChildren(requester);
			if (obstacle) SCR_EntityHelper.DeleteEntityAndChildren(obstacle);
			SetFailure("Launch fixtures could not be spawned.");
			return;
		}
		vector position = "100 0 100";
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
		requester.SetOrigin(position);
		obstacle.SetOrigin(position + "2 0 0");
		obstacle.Update();
		GenericEntity.Cast(obstacle).OnTransformReset();
		IEntity drone = MUR_DroneLaunch.LaunchNear(requester, position + "20 0 0");
		if (!drone)
			SetFailure("A blocked first position should use another clear position.");
		else
		{
			AssertTrue(vector.Distance(drone.GetOrigin(), position + "2 0 0") > 1, "The drone should avoid the blocked position.");
			RplComponent.DeleteRplEntity(drone, false);
		}
		SCR_EntityHelper.DeleteEntityAndChildren(obstacle);
		RplComponent.DeleteRplEntity(requester, false);
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_DroneLaunch_BlockedAbove_UsesAnotherPosition : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		Resource requesterPrefab = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		Resource obstaclePrefab = Resource.Load("{9019FBEC2B39E7B5}Prefabs/Tests/MUR_Obstacle.et");
		if (!requesterPrefab || !requesterPrefab.IsValid() || !obstaclePrefab || !obstaclePrefab.IsValid())
		{
			SetFailure("Launch fixtures could not be loaded.");
			return;
		}
		IEntity requester = GetGame().SpawnEntityPrefab(requesterPrefab, GetGame().GetWorld());
		IEntity obstacle = GetGame().SpawnEntityPrefab(obstaclePrefab, GetGame().GetWorld());
		if (!requester || !obstacle)
		{
			if (requester) SCR_EntityHelper.DeleteEntityAndChildren(requester);
			if (obstacle) SCR_EntityHelper.DeleteEntityAndChildren(obstacle);
			SetFailure("Launch fixtures could not be spawned.");
			return;
		}
		vector position = "100 0 100";
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
		requester.SetOrigin(position);
		obstacle.SetOrigin(position + "2 2 0");
		obstacle.Update();
		GenericEntity.Cast(obstacle).OnTransformReset();
		IEntity drone = MUR_DroneLaunch.LaunchNear(requester, position + "20 0 0");
		if (!drone)
			SetFailure("An obstacle above the first position should use another clear position.");
		else
		{
			AssertTrue(vector.Distance(drone.GetOrigin(), position + "2 0 0") > 1, "The drone should avoid the blocked corridor.");
			RplComponent.DeleteRplEntity(drone, false);
		}
		SCR_EntityHelper.DeleteEntityAndChildren(obstacle);
		RplComponent.DeleteRplEntity(requester, false);
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 30)]
class MUR_VioletFlight_Obstacle_ClimbsAndReturns : SCR_AutotestCaseBase
{
	protected IEntity m_Obstacle;
	protected IEntity m_Drone;
	protected MUR_DroneFlightComponent m_Flight;
	protected vector m_vLaunch = "100 100 100";
	protected bool m_bClimbed;

	[TestStep(TestStage.Setup)]
	void Setup_ObstacleAndDrone()
	{
		Resource obstacle = Resource.Load("{9019FBEC2B39E7B5}Prefabs/Tests/MUR_Obstacle.et");
		Resource drone = Resource.Load("{87C1F8C2CD14A28C}Prefabs/MUR_Violet.et");
		if (!obstacle || !obstacle.IsValid() || !drone || !drone.IsValid())
		{
			SetFailure("Obstacle or drone prefab could not be loaded.");
			return;
		}
		m_Obstacle = GetGame().SpawnEntityPrefab(obstacle, GetGame().GetWorld());
		m_Drone = GetGame().SpawnEntityPrefab(drone, GetGame().GetWorld());
		if (!m_Obstacle || !m_Drone)
		{
			DeleteFixtures();
			SetFailure("Obstacle or drone prefab could not be spawned.");
			return;
		}
		m_Obstacle.SetOrigin("104 110 100");
		m_Obstacle.Update();
		GenericEntity.Cast(m_Obstacle).OnTransformReset();
		m_Drone.SetOrigin(m_vLaunch);
		TraceSphere trace = new TraceSphere();
		trace.Radius = 0.2;
		trace.Start = "102 110.5 100";
		trace.End = "106 110.5 100";
		trace.Flags = TraceFlags.DEFAULT | TraceFlags.ANY_CONTACT;
		if (GetGame().GetWorld().TraceMove(trace, null) >= 1.0)
		{
			DeleteFixtures();
			SetFailure("The obstacle fixture has no traceable collision on the route.");
			return;
		}
		m_Flight = MUR_DroneFlightComponent.Cast(m_Drone.FindComponent(MUR_DroneFlightComponent));
		if (!m_Flight || !m_Flight.StartFlight(m_vLaunch + "8 0 0"))
		{
			DeleteFixtures();
			SetFailure("Flight could not start.");
		}
	}

	[TestStep(TestStage.Main)]
	bool Execute_WaitForReturn()
	{
		vector position = m_Drone.GetOrigin();
		if (position[1] > 111)
			m_bClimbed = true;
		return m_Flight.IsComplete();
	}

	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		AssertTrue(m_bClimbed && vector.Distance(m_Drone.GetOrigin(), m_vLaunch) < 0.2, "Drone should climb over the obstacle and return.");
	}

	[TestStep(TestStage.TearDown)]
	void TearDown_DeleteFixtures()
	{
		DeleteFixtures();
	}

	protected void DeleteFixtures()
	{
		if (m_Drone)
			SCR_EntityHelper.DeleteEntityAndChildren(m_Drone);
		if (m_Obstacle)
			SCR_EntityHelper.DeleteEntityAndChildren(m_Obstacle);
		m_Drone = null;
		m_Obstacle = null;
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_FlightRoute_Obstruction_ClimbIsBounded : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		MUR_FlightRoute route = new MUR_FlightRoute("0 0 0", "100 0 0");
		AssertTrue(!route.RaiseCruiseAltitude(15), "Do not reroute while taking off.");
		route.Update("0 10 0", 0.1);
		AssertTrue(route.RaiseCruiseAltitude(15), "Cruise altitude should increase to clear an obstacle.");
		AssertTrue(vector.Distance(route.GetDestination(), "100 15 0") < 0.01, "Outbound destination should use the raised altitude.");
		AssertTrue(!route.RaiseCruiseAltitude(51), "A climb above the configured limit must be rejected.");
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_FlightRoute_OutAndBack_CompletesAtLaunch : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		vector launchPoint = "0 0 0";
		vector scoutPoint = "100 0 0";
		vector launchAltitude = "0 10 0";
		vector scoutAltitude = "100 10 0";
		MUR_FlightRoute route = new MUR_FlightRoute(launchPoint, scoutPoint);

		route.Update(launchPoint, 1);
		if (route.GetPhase() != MUR_FlightPhase.TAKEOFF || vector.Distance(route.GetDestination(), launchAltitude) > 0.01)
		{
			SetFailure("The route should initially climb above the launch point.");
			return;
		}

		route.Update(launchAltitude, 1);
		if (route.GetPhase() != MUR_FlightPhase.OUTBOUND || vector.Distance(route.GetDestination(), scoutAltitude) > 0.01)
		{
			SetFailure("Takeoff should lead to the scout point at flight altitude.");
			return;
		}

		route.Update(scoutAltitude, 1);
		if (route.GetPhase() != MUR_FlightPhase.RETURNING || vector.Distance(route.GetDestination(), launchAltitude) > 0.01)
		{
			SetFailure("Reaching the scout point should start the return.");
			return;
		}

		route.Update(launchAltitude, 1);
		if (route.GetPhase() != MUR_FlightPhase.LANDING || vector.Distance(route.GetDestination(), launchPoint) > 0.01)
		{
			SetFailure("Returning above the launch point should begin landing.");
			return;
		}

		route.Update(launchPoint, 1);
		if (route.GetPhase() != MUR_FlightPhase.COMPLETE)
		{
			SetFailure("Reaching the launch point should complete the route.");
			return;
		}
	}
}
