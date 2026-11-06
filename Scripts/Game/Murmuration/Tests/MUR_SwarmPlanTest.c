[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_SwarmPlan_ValidDistinctRoutes_IsAccepted : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		MUR_SwarmPlan plan = new MUR_SwarmPlan();
		plan.AddRoute({"100 0 100", "150 0 100"});
		plan.AddRoute({"100 0 100", "100 0 150"});
		plan.AddRoute({"100 0 100", "50 0 100"});
		AssertTrue(MUR_SwarmPlanValidator.IsValid(plan, "100 0 100"), "Three distinct routes in the search area should be accepted.");
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_SwarmPlan_Json_RejectsMissingCoordinates : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		string missing = "{\"routes\":[{\"waypoints\":[{\"x\":100,\"y\":0,\"z\":0},{\"x\":150,\"y\":0,\"z\":0}]},{\"waypoints\":[{\"x\":0,\"y\":0,\"z\":100},{\"x\":0,\"y\":0,\"z\":150}]},{\"waypoints\":[{\"x\":-100,\"y\":0},{\"x\":-150,\"y\":0,\"z\":0}]}]}";
		AssertTrue(!MUR_SwarmPlanCodec.Decode(missing, "0 0 0"), "Missing coordinates must not produce a usable plan.");
		AssertTrue(!MUR_SwarmPlanCodec.Decode("not JSON", "0 0 0"), "Malformed JSON must not produce a usable plan.");
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_SwarmPlan_Json_ValidPlanIsAccepted : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		string json = "{\"routes\":[{\"waypoints\":[{\"x\":100,\"y\":0,\"z\":0},{\"x\":150,\"y\":0,\"z\":0}]},{\"waypoints\":[{\"x\":0,\"y\":0,\"z\":100},{\"x\":0,\"y\":0,\"z\":150}]},{\"waypoints\":[{\"x\":-100,\"y\":0,\"z\":0},{\"x\":-150,\"y\":0,\"z\":0}]}]}";
		AssertTrue(MUR_SwarmPlanCodec.Decode(json, "0 0 0") != null, "A valid structured plan should be decoded.");
		AssertTrue(!MUR_SwarmPlanCodec.Decode(json + " trailing", "0 0 0"), "Trailing malformed data must be rejected.");
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_SwarmPlan_InvalidRoutes_AreRejected : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		MUR_SwarmPlan tooFew = new MUR_SwarmPlan();
		tooFew.AddRoute({"0 0 0"});
		AssertTrue(!MUR_SwarmPlanValidator.IsValid(tooFew, "0 0 0"), "A plan must contain three routes.");

		MUR_SwarmPlan duplicate = new MUR_SwarmPlan();
		duplicate.AddRoute({"100 0 0", "150 0 0"});
		duplicate.AddRoute({"100 0 0", "150 0 0"});
		duplicate.AddRoute({"0 0 100", "0 0 150"});
		AssertTrue(!MUR_SwarmPlanValidator.IsValid(duplicate, "0 0 0"), "Routes must be distinct.");

		MUR_SwarmPlan outside = new MUR_SwarmPlan();
		outside.AddRoute({"501 0 0", "150 0 0"});
		outside.AddRoute({"0 0 100", "0 0 150"});
		outside.AddRoute({"-100 0 0", "-150 0 0"});
		AssertTrue(!MUR_SwarmPlanValidator.IsValid(outside, "0 0 0"), "Waypoints must stay inside the search area.");
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_SwarmPlan_StationaryRoute_IsRejected : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		MUR_SwarmPlan plan = new MUR_SwarmPlan();
		plan.AddRoute({"100 0 0"});
		plan.AddRoute({"0 0 100", "0 0 150"});
		plan.AddRoute({"-100 0 0", "-150 0 0"});
		AssertTrue(!MUR_SwarmPlanValidator.IsValid(plan, "0 0 0"), "A stationary route cannot search the area.");
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_PlanningAttempt_TimeoutRejectsLatePlan : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		MUR_PlanningAttempt attempt = new MUR_PlanningAttempt("0 0 0", 10);
		MUR_SwarmPlan plan = new MUR_SwarmPlan();
		plan.AddRoute({"100 0 0", "150 0 0"});
		plan.AddRoute({"0 0 100", "0 0 150"});
		plan.AddRoute({"-100 0 0", "-150 0 0"});
		attempt.Tick(70.1);
		AssertTrue(attempt.HasFailed(), "Planning should fail after the 60-second deadline.");
		AssertTrue(!attempt.Accept(plan, 70.1), "A late plan must not launch a swarm.");
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_PlanningAttempt_InvalidPlanFailsWithoutFallback : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		MUR_PlanningAttempt attempt = new MUR_PlanningAttempt("0 0 0", 10);
		AssertTrue(!attempt.Accept(null, 11), "An invalid plan cannot be accepted.");
		AssertTrue(attempt.HasFailed() && !attempt.GetPlan(), "A failed initial plan must not produce a fallback route.");
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_FlightRoute_SearchCyclesUntilReturn : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		MUR_FlightRoute route = new MUR_FlightRoute("0 0 0", "100 0 0");
		route.ConfigureSearch({"110 0 0", "100 0 10"});
		route.Update("0 10 0", 0.1);
		route.Update("100 10 0", 0.1);
		AssertTrue(vector.Distance(route.GetDestination(), "110 10 0") < 0.01, "Arrival should start the first search waypoint.");
		route.Update("110 10 0", 0.1);
		AssertTrue(vector.Distance(route.GetDestination(), "100 10 10") < 0.01, "Search should advance through the route.");
		route.Update("100 10 10", 0.1);
		AssertTrue(vector.Distance(route.GetDestination(), "110 10 0") < 0.01, "Search should cycle until recalled.");
		route.ReturnHome();
		AssertTrue(vector.Distance(route.GetDestination(), "0 10 0") < 0.01, "Return should direct the drone home.");
	}
}

class MUR_TestOllamaListener : MUR_OllamaListener
{
	bool m_bFinished;
	ref MUR_SwarmPlan m_Plan;
	string m_sError;

	override void OnPlanningFinished(MUR_SwarmPlan plan, string error)
	{
		m_Plan = plan;
		m_sError = error;
		m_bFinished = true;
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 10)]
class MUR_OllamaPlanner_UnavailableEndpoint_FailsWithoutPlan : SCR_AutotestCaseBase
{
	protected ref MUR_OllamaPlanner m_Planner;
	protected ref MUR_TestOllamaListener m_Listener;

	[TestStep(TestStage.Setup)]
	void Setup_Request()
	{
		m_Planner = new MUR_OllamaPlanner();
		m_Listener = new MUR_TestOllamaListener();
		array<vector> positions = {"0 0 0", "2 0 0", "-2 0 0"};
		if (!m_Planner.Start("http://127.0.0.1:9/", "100 0 0", positions, m_Listener))
			SetFailure("Request could not be started.");
	}

	[TestStep(TestStage.Main)]
	bool Execute_WaitForFailure()
	{
		return m_Listener.m_bFinished;
	}

	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		AssertTrue(!m_Listener.m_Plan && !m_Listener.m_sError.IsEmpty(), "An unavailable endpoint must fail without a plan.");
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_DroneLaunch_ThreeSites_AreDistinct : SCR_AutotestCaseBase
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
			SetFailure("Requester fixture could not spawn.");
			return;
		}
		vector point = "100 0 100";
		point[1] = GetGame().GetWorld().GetSurfaceY(point[0], point[2]);
		requester.SetOrigin(point);
		array<ref MUR_DroneLaunchSite> sites = {};
		if (!MUR_DroneLaunch.FindSites(requester, point + "20 0 0", 3, sites))
			SetFailure("Three clear launch sites should be available.");
		else
			AssertTrue(vector.Distance(sites[0].position, sites[1].position) > 1 && vector.Distance(sites[1].position, sites[2].position) > 1 && vector.Distance(sites[0].position, sites[2].position) > 1, "The drones need distinct launch sites.");
		RplComponent.DeleteRplEntity(requester, false);
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_DroneController_Prefab_IsInventoryItem : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		Resource prefab = Resource.Load("{A39802E278DE0200}Prefabs/MUR_DroneController.et");
		if (!prefab || !prefab.IsValid())
		{
			SetFailure("Drone controller prefab could not be loaded.");
			return;
		}
		IEntity item = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld());
		if (!item)
		{
			SetFailure("Drone controller prefab could not be spawned.");
			return;
		}
		AssertTrue(item.FindComponent(MUR_DroneControllerComponent) && item.FindComponent(InventoryItemComponent), "The controller must be an identifiable inventory item.");
		RplComponent.DeleteRplEntity(item, false);
	}
}

[Test(suite: MUR_FlightRouteSuite, timeoutS: 5)]
class MUR_DroneController_Name_IsIndependentOfRadio : SCR_AutotestCaseBase
{
	[TestStep(TestStage.Main)]
	void Execute_Assert()
	{
		Resource controller = Resource.Load("{A39802E278DE0200}Prefabs/MUR_DroneController.et");
		Resource radio = Resource.Load("{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et");
		if (!controller || !controller.IsValid() || !radio || !radio.IsValid())
		{
			SetFailure("Controller or radio prefab could not be loaded.");
			return;
		}
		IEntity item = GetGame().SpawnEntityPrefab(controller, GetGame().GetWorld());
		IEntity ordinaryRadio = GetGame().SpawnEntityPrefab(radio, GetGame().GetWorld());
		if (!item || !ordinaryRadio)
		{
			if (item) RplComponent.DeleteRplEntity(item, false);
			if (ordinaryRadio) RplComponent.DeleteRplEntity(ordinaryRadio, false);
			SetFailure("Controller or radio could not be spawned.");
			return;
		}
		InventoryItemComponent itemInfo = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		InventoryItemComponent radioInfo = InventoryItemComponent.Cast(ordinaryRadio.FindComponent(InventoryItemComponent));
		if (!itemInfo || !radioInfo || !itemInfo.GetUIInfo() || !radioInfo.GetUIInfo())
			SetFailure("Inventory UI information is unavailable.");
		else
		{
			AssertTrue(itemInfo.GetUIInfo().GetName() == "Drone controller", "The item must identify itself as a drone controller.");
			AssertTrue(radioInfo.GetUIInfo().GetName() != "Drone controller", "Ordinary radios must retain their name.");
		}
		RplComponent.DeleteRplEntity(item, false);
		RplComponent.DeleteRplEntity(ordinaryRadio, false);
	}
}
