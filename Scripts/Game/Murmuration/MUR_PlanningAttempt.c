// Tracks a single initial planning request and ignores late responses.
class MUR_PlanningAttempt
{
	protected vector m_vScoutPoint;
	protected float m_fDeadline;
	protected bool m_bFailed;
	protected ref MUR_SwarmPlan m_Plan;

	void MUR_PlanningAttempt(vector scoutPoint, float now)
	{
		m_vScoutPoint = scoutPoint;
		m_fDeadline = now + 60.0;
	}

	void Tick(float now)
	{
		if (!m_Plan && now > m_fDeadline)
			m_bFailed = true;
	}

	bool Accept(MUR_SwarmPlan plan, float now)
	{
		Tick(now);
		if (m_bFailed || m_Plan)
			return false;
		if (!MUR_SwarmPlanValidator.IsValid(plan, m_vScoutPoint))
		{
			m_bFailed = true;
			return false;
		}
		m_Plan = plan;
		return true;
	}

	void Fail()
	{
		if (!m_Plan)
			m_bFailed = true;
	}

	bool HasFailed()
	{
		return m_bFailed;
	}

	MUR_SwarmPlan GetPlan()
	{
		return m_Plan;
	}
}
