class MUR_SwarmMissionListener
{
	void OnMissionStatus(string message, bool active) {}
	bool IsRequesterValid(IEntity player) { return player != null; }
}

// Keeps finished drones visible briefly, even if the player starts another mission.
class MUR_SwarmCleanup
{
	protected static ref array<ref MUR_SwarmCleanup> s_Pending = {};
	protected ref array<IEntity> m_Drones = {};

	static void Schedule(array<IEntity> drones)
	{
		if (!drones || drones.IsEmpty()) return;
		MUR_SwarmCleanup cleanup = new MUR_SwarmCleanup();
		cleanup.m_Drones.Copy(drones);
		s_Pending.Insert(cleanup);
		GetGame().GetCallqueue().CallLater(cleanup.RemoveDrones, 30000, false);
	}

	protected void RemoveDrones()
	{
		foreach (IEntity drone : m_Drones)
			if (drone) RplComponent.DeleteRplEntity(drone, false);
		m_Drones.Clear();
		s_Pending.RemoveItem(this);
	}
}

// A mission is owned by one player controller and runs on the authority.
class MUR_SwarmMission : MUR_OllamaListener
{
	protected IEntity m_Player;
	protected vector m_vScoutPoint;
	protected ref array<ref MUR_DroneLaunchSite> m_Sites = {};
	protected ref array<IEntity> m_Drones = {};
	protected ref MUR_OllamaPlanner m_Planner;
	protected ref MUR_SwarmMissionListener m_Listener;
	protected float m_fSearchStart = -1;
	protected bool m_bReturning;
	protected bool m_bFinished;

	bool Start(IEntity player, vector scoutPoint, MUR_SwarmMissionListener listener)
	{
		if (!Replication.IsServer() || !player || !listener || m_Player)
			return false;
		m_Player = player;
		m_Listener = listener;
		m_vScoutPoint = scoutPoint;
		m_vScoutPoint[1] = GetGame().GetWorld().GetSurfaceY(scoutPoint[0], scoutPoint[2]);
		if (!MUR_DroneLaunch.FindSites(player, m_vScoutPoint, 3, m_Sites))
		{
			Finish("Choose a point within 2 km and find clear ground for three drones.");
			return false;
		}
		array<vector> positions = {};
		foreach (MUR_DroneLaunchSite site : m_Sites)
			positions.Insert(site.position);
		string endpoint = "http://127.0.0.1:11434/";
		string configuredEndpoint;
		if (System.GetCLIParam("murOllamaUrl", configuredEndpoint) && !configuredEndpoint.IsEmpty())
			endpoint = configuredEndpoint;
		if (!endpoint.IsEmpty() && endpoint.Substring(endpoint.Length() - 1, 1) != "/")
			endpoint = endpoint + "/";
		m_Planner = new MUR_OllamaPlanner();
		m_Listener.OnMissionStatus("Planning with Ollama...", true);
		if (!m_Planner.Start(endpoint, m_vScoutPoint, positions, this))
		{
			Finish("Could not start Ollama planning.");
			return false;
		}
		return true;
	}

	override void OnPlanningFinished(MUR_SwarmPlan plan, string error)
	{
		if (m_bFinished) return;
		if (!plan)
		{
			Finish(error);
			return;
		}
		if (!m_Listener.IsRequesterValid(m_Player))
		{
			Finish("Requester is no longer available.");
			return;
		}
		vector playerPosition = m_Player.GetOrigin();
		foreach (MUR_DroneLaunchSite site : m_Sites)
		{
			vector offset = site.position - playerPosition;
			offset[1] = 0;
			if (offset.Length() > 6 || !MUR_DroneLaunch.IsSiteClear(site, m_vScoutPoint))
			{
				Finish("Launch area changed while planning. Choose clear ground and try again.");
				return;
			}
		}
		for (int i = 0; i < 3; i++)
		{
			IEntity drone = MUR_DroneLaunch.LaunchAt(m_Sites[i], m_vScoutPoint, plan.m_Routes[i].m_Waypoints);
			if (!drone)
			{
				foreach (IEntity spawned : m_Drones)
					if (spawned) RplComponent.DeleteRplEntity(spawned, false);
				m_Drones.Clear();
				Finish("Could not launch the swarm.");
				return;
			}
			m_Drones.Insert(drone);
		}
		m_Listener.OnMissionStatus("Swarm launched.", true);
		GetGame().GetCallqueue().CallLater(UpdateMission, 1000, false);
	}

	protected void UpdateMission()
	{
		if (m_bFinished) return;
		bool anyFlying;
		int returned;
		int failedOrLost;
		float now = System.GetTickCount() * 0.001;
		foreach (IEntity drone : m_Drones)
		{
			if (!drone) { failedOrLost++; continue; }
			MUR_DroneFlightComponent flight = MUR_DroneFlightComponent.Cast(drone.FindComponent(MUR_DroneFlightComponent));
			if (!flight || flight.IsFailed()) { failedOrLost++; continue; }
			if (flight.IsComplete()) { returned++; continue; }
			anyFlying = true;
			vector distance = drone.GetOrigin() - m_vScoutPoint;
			distance[1] = 0;
			if (m_fSearchStart < 0 && distance.Length() <= 500)
			{
				m_fSearchStart = now;
				Print("MUR swarm: first drone reached the search area; five-minute search started");
			}
		}
		if (!anyFlying)
		{
			if (failedOrLost > 0 && m_fSearchStart < 0)
				Finish(string.Format("Scouting failed: %1 returned, %2 failed or lost.", returned, failedOrLost));
			else if (failedOrLost > 0)
				Finish(string.Format("Swarm mission ended: %1 returned, %2 failed or lost.", returned, failedOrLost));
			else
				Finish(string.Format("Swarm mission complete: %1 drones returned.", returned));
			return;
		}
		if (!m_bReturning && m_fSearchStart >= 0 && now - m_fSearchStart >= 300)
		{
			m_bReturning = true;
			foreach (IEntity aircraft : m_Drones)
			{
				if (!aircraft) continue;
				MUR_DroneFlightComponent returning = MUR_DroneFlightComponent.Cast(aircraft.FindComponent(MUR_DroneFlightComponent));
				if (returning) returning.ReturnHome();
			}
			m_Listener.OnMissionStatus("Search complete. Swarm returning.", true);
		}
		GetGame().GetCallqueue().CallLater(UpdateMission, 1000, false);
	}

	protected void Finish(string message)
	{
		if (m_bFinished) return;
		m_bFinished = true;
		MUR_SwarmCleanup.Schedule(m_Drones);
		m_Drones.Clear();
		if (m_Listener) m_Listener.OnMissionStatus(message, false);
	}
}
