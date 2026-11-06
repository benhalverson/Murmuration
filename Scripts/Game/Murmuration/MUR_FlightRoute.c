// Tracks the waypoints for a single out-and-back reconnaissance flight.
enum MUR_FlightPhase
{
	TAKEOFF,
	OUTBOUND,
	RETURNING,
	LANDING,
	COMPLETE
}

class MUR_FlightRoute
{
	protected vector m_vLaunchPoint;
	protected vector m_vScoutPoint;
	protected float m_fFlightAltitude;
	protected float m_fMaxFlightAltitude;
	protected MUR_FlightPhase m_ePhase;

	void MUR_FlightRoute(vector launchPoint, vector scoutPoint)
	{
		m_vLaunchPoint = launchPoint;
		m_vScoutPoint = scoutPoint;
		m_fFlightAltitude = Math.Max(launchPoint[1], scoutPoint[1]) + 10.0;
		m_fMaxFlightAltitude = m_fFlightAltitude + 40.0;
		m_ePhase = MUR_FlightPhase.TAKEOFF;
	}

	MUR_FlightPhase GetPhase()
	{
		return m_ePhase;
	}

	bool RaiseCruiseAltitude(float altitude)
	{
		if ((m_ePhase != MUR_FlightPhase.OUTBOUND && m_ePhase != MUR_FlightPhase.RETURNING) || altitude > m_fMaxFlightAltitude)
			return false;
		m_fFlightAltitude = Math.Max(m_fFlightAltitude, altitude);
		return true;
	}

	vector GetDestination()
	{
		vector destination;
		if (m_ePhase == MUR_FlightPhase.TAKEOFF || m_ePhase == MUR_FlightPhase.RETURNING)
		{
			destination = m_vLaunchPoint;
			destination[1] = m_fFlightAltitude;
			return destination;
		}

		if (m_ePhase == MUR_FlightPhase.OUTBOUND)
		{
			destination = m_vScoutPoint;
			destination[1] = m_fFlightAltitude;
			return destination;
		}

		return m_vLaunchPoint;
	}

	// The caller supplies the actual aircraft position and arrival tolerance.
	void Update(vector position, float arrivalRadius)
	{
		if (m_ePhase == MUR_FlightPhase.COMPLETE || arrivalRadius < 0)
			return;

		if (vector.Distance(position, GetDestination()) > arrivalRadius)
			return;

		if (m_ePhase == MUR_FlightPhase.TAKEOFF)
			m_ePhase = MUR_FlightPhase.OUTBOUND;
		else if (m_ePhase == MUR_FlightPhase.OUTBOUND)
			m_ePhase = MUR_FlightPhase.RETURNING;
		else if (m_ePhase == MUR_FlightPhase.RETURNING)
			m_ePhase = MUR_FlightPhase.LANDING;
		else
			m_ePhase = MUR_FlightPhase.COMPLETE;
	}
}
