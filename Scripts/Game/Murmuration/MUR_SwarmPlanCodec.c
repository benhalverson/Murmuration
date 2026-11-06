// Missing numeric fields retain a sentinel and fail the search-area check.
class MUR_JsonWaypoint : JsonApiStruct
{
	int x = 1000000000;
	int y = 1000000000;
	int z = 1000000000;

	void MUR_JsonWaypoint()
	{
		RegV("x");
		RegV("y");
		RegV("z");
	}
}

class MUR_JsonRoute : JsonApiStruct
{
	ref array<ref MUR_JsonWaypoint> waypoints = {};

	void MUR_JsonRoute()
	{
		RegV("waypoints");
	}
}

class MUR_JsonPlan : JsonApiStruct
{
	ref array<ref MUR_JsonRoute> routes = {};
	bool m_bParsed;

	void MUR_JsonPlan()
	{
		RegV("routes");
	}

	override void OnSuccess(int errorCode)
	{
		m_bParsed = true;
	}

	override void OnError(int errorCode)
	{
		m_bParsed = false;
	}
}

class MUR_SwarmPlanCodec
{
	// JsonApiStruct accepts a valid prefix followed by junk, so require one
	// complete top-level object before handing the data to the engine parser.
	static bool HasSingleObject(string json)
	{
		json = json.Trim();
		if (json.IsEmpty() || json.Length() > 65536 || json.ToAscii(0) != 123)
			return false;
		array<int> closers = {125};
		bool quoted;
		bool escaped;
		for (int i = 1; i < json.Length(); i++)
		{
			int c = json.ToAscii(i);
			if (quoted)
			{
				if (c < 32) return false;
				if (escaped) escaped = false;
				else if (c == 92) escaped = true;
				else if (c == 34) quoted = false;
				continue;
			}
			if (c == 34) quoted = true;
			else if (c == 123) closers.Insert(125);
			else if (c == 91) closers.Insert(93);
			else if (c == 125 || c == 93)
			{
				if (closers.IsEmpty() || closers[closers.Count() - 1] != c)
					return false;
				closers.Remove(closers.Count() - 1);
				if (closers.IsEmpty())
					return i == json.Length() - 1;
			}
		}
		return false;
	}

	static MUR_SwarmPlan Decode(string json, vector scoutPoint)
	{
		if (!HasSingleObject(json))
			return null;
		MUR_JsonPlan decoded = new MUR_JsonPlan();
		decoded.ExpandFromRAW(json);
		if (!decoded.m_bParsed || !decoded.routes || decoded.routes.Count() != 3)
			return null;
		MUR_SwarmPlan plan = new MUR_SwarmPlan();
		foreach (MUR_JsonRoute route : decoded.routes)
		{
			if (!route || !route.waypoints)
				return null;
			array<vector> points = {};
			foreach (MUR_JsonWaypoint waypoint : route.waypoints)
			{
				if (!waypoint)
					return null;
				vector position;
				position[0] = waypoint.x;
				position[1] = waypoint.y;
				position[2] = waypoint.z;
				points.Insert(position);
			}
			plan.AddRoute(points);
		}
		if (!MUR_SwarmPlanValidator.IsValid(plan, scoutPoint))
			return null;
		return plan;
	}
}
