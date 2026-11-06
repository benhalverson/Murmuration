class MUR_DroneFlightComponentClass : ScriptComponentClass
{
}

// Flight is driven on the server. The entity must provide replicated movement.
class MUR_DroneFlightComponent : ScriptComponent
{
	protected ref MUR_FlightRoute m_Route;
	protected bool m_bObstructed;
	protected bool m_bClimbing;
	protected vector m_vClimbTarget;
	[RplProp(onRplName: "OnPositionChanged")]
	protected vector m_vReplicatedPosition;
	[RplProp(onRplName: "OnPositionChanged")]
	protected bool m_bHasPosition;
	protected static const float SPEED = 4.0;
	protected static const float ARRIVAL_RADIUS = 0.1;

	bool StartFlight(vector scoutPoint)
	{
		if (!Replication.IsServer() || m_Route)
			return false;

		m_Route = new MUR_FlightRoute(GetOwner().GetOrigin(), scoutPoint);
		m_vReplicatedPosition = GetOwner().GetOrigin();
		m_bHasPosition = true;
		Replication.BumpMe();
		SetEventMask(GetOwner(), EntityEvent.FRAME);
		return true;
	}

	bool IsObstructed()
	{
		return m_bObstructed;
	}

	bool IsComplete()
	{
		return m_Route && m_Route.GetPhase() == MUR_FlightPhase.COMPLETE;
	}

	protected void OnPositionChanged()
	{
		if (Replication.IsServer() || !m_bHasPosition)
			return;
		GetOwner().SetOrigin(m_vReplicatedPosition);
		GetOwner().Update();
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer() || !m_Route || timeSlice <= 0)
			return;

		vector position = owner.GetOrigin();
		if (m_bClimbing && vector.Distance(position, m_vClimbTarget) <= ARRIVAL_RADIUS)
			m_bClimbing = false;
		if (!m_bClimbing)
			m_Route.Update(position, ARRIVAL_RADIUS);
		if (m_Route.GetPhase() == MUR_FlightPhase.COMPLETE)
		{
			ClearEventMask(owner, EntityEvent.FRAME);
			return;
		}

		vector destination = m_Route.GetDestination();
		if (m_bClimbing)
			destination = m_vClimbTarget;
		float distance = vector.Distance(position, destination);
		if (distance <= ARRIVAL_RADIUS)
			return;
		float fraction = Math.Min(1.0, SPEED * timeSlice / distance);
		vector nextPosition = vector.Lerp(position, destination, fraction);

		TraceSphere trace = new TraceSphere();
		trace.Radius = 0.2;
		trace.Start = position;
		trace.End = nextPosition;
		trace.Exclude = owner;
		trace.Flags = TraceFlags.DEFAULT | TraceFlags.ANY_CONTACT;
		m_bObstructed = GetGame().GetWorld().TraceMove(trace, null) < 1.0;
		if (m_bObstructed)
		{
			if (!m_bClimbing && m_Route.RaiseCruiseAltitude(position[1] + 5.0))
			{
				m_vClimbTarget = position + "0 5 0";
				m_bClimbing = true;
			}
			return;
		}

		owner.SetOrigin(nextPosition);
		owner.Update();
		m_vReplicatedPosition = nextPosition;
		Replication.BumpMe();
		RplComponent replication = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (replication)
			replication.ForceNodeMovement(position);
	}
}
