// A search route contains world-space waypoints for one drone.
class MUR_SwarmSearchRoute
{
	ref array<vector> m_Waypoints = {};
}

class MUR_SwarmPlan
{
	ref array<ref MUR_SwarmSearchRoute> m_Routes = {};

	void AddRoute(array<vector> waypoints)
	{
		MUR_SwarmSearchRoute route = new MUR_SwarmSearchRoute();
		if (waypoints)
			route.m_Waypoints.Copy(waypoints);
		m_Routes.Insert(route);
	}
}

class MUR_SwarmPlanValidator
{
	static bool IsValid(MUR_SwarmPlan plan, vector scoutPoint)
	{
		if (!plan || !plan.m_Routes || plan.m_Routes.Count() != 3)
			return false;
		foreach (MUR_SwarmSearchRoute route : plan.m_Routes)
		{
			if (!route || !route.m_Waypoints || route.m_Waypoints.Count() < 2 || route.m_Waypoints.Count() > 16)
				return false;
			bool moves;
			foreach (vector waypoint : route.m_Waypoints)
			{
				// Negated comparison also rejects non-finite distances.
				if (!(vector.Distance(waypoint, scoutPoint) <= 500.0))
					return false;
				if (vector.Distance(waypoint, route.m_Waypoints[0]) > 1.0)
					moves = true;
			}
			if (!moves) return false;
		}
		for (int i = 0; i < 3; i++)
		{
			for (int j = i + 1; j < 3; j++)
			{
				array<vector> a = plan.m_Routes[i].m_Waypoints;
				array<vector> b = plan.m_Routes[j].m_Waypoints;
				if (a.Count() != b.Count())
					continue;
				bool same = true;
				for (int k = 0; k < a.Count(); k++)
				{
					if (vector.Distance(a[k], b[k]) > 0.1)
					{
						same = false;
						break;
					}
				}
				if (same)
					return false;
			}
		}
		return true;
	}
}
