class MUR_OllamaMissionInput : JsonApiStruct
{
	ref MUR_JsonWaypoint scout_point;
	ref array<ref MUR_JsonWaypoint> drone_positions = {};

	void MUR_OllamaMissionInput()
	{
		scout_point = new MUR_JsonWaypoint();
		RegV("scout_point");
		RegV("drone_positions");
	}

	static MUR_JsonWaypoint FromPosition(vector position)
	{
		MUR_JsonWaypoint point = new MUR_JsonWaypoint();
		point.x = Math.Round(position[0]);
		point.y = Math.Round(position[1]);
		point.z = Math.Round(position[2]);
		return point;
	}
}

class MUR_OllamaRequestBody : JsonApiStruct
{
	string model = "qwen3.5:latest";
	string prompt;

	void MUR_OllamaRequestBody()
	{
		RegV("model");
		RegV("prompt");
	}
}

class MUR_OllamaResponse : JsonApiStruct
{
	string response;
	bool done;
	bool m_bParsed;

	void MUR_OllamaResponse()
	{
		RegV("response");
		RegV("done");
	}

	override void OnSuccess(int errorCode) { m_bParsed = true; }
	override void OnError(int errorCode) { m_bParsed = false; }
}

class MUR_OllamaListener
{
	void OnPlanningFinished(MUR_SwarmPlan plan, string error) {}
}

// One request is owned by the mission while it is waiting for Ollama.
class MUR_OllamaPlanner : RestCallback
{
	protected ref MUR_PlanningAttempt m_Attempt;
	protected ref MUR_OllamaListener m_Listener;
	protected bool m_bFinished;
	protected vector m_vScoutPoint;

	bool Start(string endpoint, vector scoutPoint, array<vector> dronePositions, MUR_OllamaListener listener)
	{
		if (m_Attempt || !listener || !dronePositions || dronePositions.Count() != 3)
			return false;
		RestApi api = GetGame().GetRestApi();
		if (!api || endpoint.IsEmpty())
			return false;
		RestContext context = api.GetContext(endpoint);
		if (!context)
			return false;

		MUR_OllamaMissionInput input = new MUR_OllamaMissionInput();
		input.scout_point = MUR_OllamaMissionInput.FromPosition(scoutPoint);
		foreach (vector position : dronePositions)
			input.drone_positions.Insert(MUR_OllamaMissionInput.FromPosition(position));
		input.Pack();
		string missionData = input.AsString();
		missionData = missionData.Substring(0, missionData.Length() - 1) + ",\"observed_targets\":[]}";
		MUR_OllamaRequestBody body = new MUR_OllamaRequestBody();
		body.prompt = "Plan three different drone search routes. Return only JSON with a routes array of exactly three objects, each with a waypoints array of two to sixteen distinct objects with integer x, y, z world coordinates. Every waypoint must be within 500 meters of scout_point. Mission data: " + missionData;
		body.Pack();
		int centerX = input.scout_point.x;
		int centerY = input.scout_point.y;
		int centerZ = input.scout_point.z;
		string xSchema = string.Format("{\"type\":\"integer\",\"minimum\":%1,\"maximum\":%2}", centerX - 350, centerX + 350);
		string ySchema = string.Format("{\"type\":\"integer\",\"minimum\":%1,\"maximum\":%1}", centerY);
		string zSchema = string.Format("{\"type\":\"integer\",\"minimum\":%1,\"maximum\":%2}", centerZ - 350, centerZ + 350);
		string pointSchema = "{\"type\":\"object\",\"properties\":{\"x\":" + xSchema + ",\"y\":" + ySchema + ",\"z\":" + zSchema + "},\"required\":[\"x\",\"y\",\"z\"],\"additionalProperties\":false}";
		string routeSchema = "{\"type\":\"object\",\"properties\":{\"waypoints\":{\"type\":\"array\",\"minItems\":2,\"maxItems\":16,\"uniqueItems\":true,\"items\":" + pointSchema + "}},\"required\":[\"waypoints\"],\"additionalProperties\":false}";
		string schema = "{\"type\":\"object\",\"properties\":{\"routes\":{\"type\":\"array\",\"minItems\":3,\"maxItems\":3,\"items\":" + routeSchema + "}},\"required\":[\"routes\"],\"additionalProperties\":false}";
		// JsonApiStruct encodes bools as 0/1; Ollama requires JSON booleans.
		string payload = body.AsString();
		payload = payload.Substring(0, payload.Length() - 1) + ",\"format\":" + schema + ",\"stream\":false,\"think\":false}";

		m_vScoutPoint = scoutPoint;
		m_Attempt = new MUR_PlanningAttempt(scoutPoint, Now());
		m_Listener = listener;
		context.SetTimeout(60);
		GetGame().GetCallqueue().CallLater(OnDeadline, 60000, false);
		context.POST(this, "api/generate", payload);
		return true;
	}

	protected float Now() { return System.GetTickCount() * 0.001; }

	protected void Finish(MUR_SwarmPlan plan, string error)
	{
		if (m_bFinished) return;
		m_bFinished = true;
		GetGame().GetCallqueue().Remove(OnDeadline);
		if (m_Listener) m_Listener.OnPlanningFinished(plan, error);
		m_Listener = null;
	}

	protected void OnDeadline()
	{
		if (!m_Attempt || m_bFinished) return;
		m_Attempt.Fail();
		Finish(null, "Planning timed out after 60 seconds.");
	}

	override void OnError(int errorCode)
	{
		if (m_Attempt) m_Attempt.Fail();
		Finish(null, "Ollama request failed.");
	}

	override void OnTimeout() { OnDeadline(); }

	override void OnSuccess(string data, int dataSize)
	{
		if (m_bFinished || !m_Attempt) return;
		m_Attempt.Tick(Now());
		if (m_Attempt.HasFailed())
		{
			Finish(null, "Planning timed out after 60 seconds.");
			return;
		}
		if (dataSize <= 0 || !MUR_SwarmPlanCodec.HasSingleObject(data))
		{
			m_Attempt.Fail();
			Finish(null, "Ollama returned an invalid response.");
			return;
		}
		MUR_OllamaResponse response = new MUR_OllamaResponse();
		response.ExpandFromRAW(data);
		MUR_SwarmPlan plan;
		if (response.m_bParsed && response.done)
			plan = MUR_SwarmPlanCodec.Decode(response.response, m_vScoutPoint);
		if (!m_Attempt.Accept(plan, Now()))
		{
			Finish(null, "Ollama returned an invalid plan.");
			return;
		}
		Finish(plan, string.Empty);
	}
}
