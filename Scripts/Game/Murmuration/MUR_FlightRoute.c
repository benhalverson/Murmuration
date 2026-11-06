// Tracks the waypoints for a single out-and-back reconnaissance flight.
enum MUR_FlightPhase
{
	TAKEOFF,
	OUTBOUND,
	SEARCH,
	RETURNING,
	LANDING,
	COMPLETE,
	FAILED
}

class MUR_FlightRoute
{
	static const float MIN_TERRAIN_CLEARANCE = 36.0;
	protected vector m_vLaunchPoint;
	protected vector m_vScoutPoint;
	protected float m_fFlightAltitude;
	protected float m_fMaxFlightAltitude;
	protected MUR_FlightPhase m_ePhase;
	protected ref array<vector> m_SearchWaypoints = {};
	protected int m_iSearchWaypoint;

	void MUR_FlightRoute(vector launchPoint, vector scoutPoint)
	{
		m_vLaunchPoint = launchPoint;
		m_vScoutPoint = scoutPoint;
		m_fFlightAltitude = Math.Max(launchPoint[1], scoutPoint[1]) + MIN_TERRAIN_CLEARANCE;
		m_fMaxFlightAltitude = m_fFlightAltitude + 40.0;
		m_ePhase = MUR_FlightPhase.TAKEOFF;
	}

	MUR_FlightPhase GetPhase()
	{
		return m_ePhase;
	}

	void Fail()
	{
		m_ePhase = MUR_FlightPhase.FAILED;
	}

	void ConfigureSearch(array<vector> waypoints)
	{
		if (m_ePhase == MUR_FlightPhase.TAKEOFF && waypoints)
			m_SearchWaypoints.Copy(waypoints);
	}

	void ReturnHome()
	{
		if (m_ePhase != MUR_FlightPhase.COMPLETE && m_ePhase != MUR_FlightPhase.FAILED && m_ePhase != MUR_FlightPhase.LANDING)
			m_ePhase = MUR_FlightPhase.RETURNING;
	}

	bool RaiseCruiseAltitude(float altitude)
	{
		if ((m_ePhase != MUR_FlightPhase.OUTBOUND && m_ePhase != MUR_FlightPhase.SEARCH && m_ePhase != MUR_FlightPhase.RETURNING) || altitude > m_fMaxFlightAltitude)
			return false;
		m_fFlightAltitude = Math.Max(m_fFlightAltitude, altitude);
		return true;
	}

	void EnsureTerrainClearance(float terrainHeight)
	{
		if (m_ePhase != MUR_FlightPhase.OUTBOUND && m_ePhase != MUR_FlightPhase.SEARCH && m_ePhase != MUR_FlightPhase.RETURNING)
			return;
		float minimumAltitude = terrainHeight + MIN_TERRAIN_CLEARANCE;
		if (minimumAltitude > m_fFlightAltitude)
		{
			m_fFlightAltitude = minimumAltitude;
			m_fMaxFlightAltitude = Math.Max(m_fMaxFlightAltitude, minimumAltitude + 40.0);
		}
	}

	vector GetDestination()
	{
		vector destination;
		if (m_ePhase == MUR_FlightPhase.SEARCH && !m_SearchWaypoints.IsEmpty())
		{
			destination = m_SearchWaypoints[m_iSearchWaypoint];
			destination[1] = m_fFlightAltitude;
			return destination;
		}
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
		if (m_ePhase == MUR_FlightPhase.COMPLETE || m_ePhase == MUR_FlightPhase.FAILED || arrivalRadius < 0)
			return;

		if (vector.Distance(position, GetDestination()) > arrivalRadius)
			return;

		if (m_ePhase == MUR_FlightPhase.TAKEOFF)
			m_ePhase = MUR_FlightPhase.OUTBOUND;
		else if (m_ePhase == MUR_FlightPhase.OUTBOUND)
		{
			if (m_SearchWaypoints.IsEmpty())
				m_ePhase = MUR_FlightPhase.RETURNING;
			else
				m_ePhase = MUR_FlightPhase.SEARCH;
		}
		else if (m_ePhase == MUR_FlightPhase.SEARCH)
		{
			m_iSearchWaypoint++;
			if (m_iSearchWaypoint >= m_SearchWaypoints.Count())
				m_iSearchWaypoint = 0;
		}
		else if (m_ePhase == MUR_FlightPhase.RETURNING)
			m_ePhase = MUR_FlightPhase.LANDING;
		else
			m_ePhase = MUR_FlightPhase.COMPLETE;
	}
}
