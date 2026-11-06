class MUR_SwarmMapUi : ScriptedWidgetEventHandler
{
	protected SCR_MapEntity m_Map;
	protected SCR_PlayerController m_Controller;
	protected ImageWidget m_ButtonBackground;
	protected ButtonWidget m_Button;
	protected TextWidget m_ButtonText;
	protected TextWidget m_StatusText;
	protected FrameWidget m_MarkerRoot;
	protected TextWidget m_Marker;
	protected vector m_vScoutPoint;
	protected bool m_bHasScoutPoint;

	void Open(SCR_MapEntity mapEntity)
	{
		m_Map = mapEntity;
		m_Controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!m_Map || !m_Controller) return;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace) return;
		m_ButtonBackground = ImageWidget.Cast(workspace.CreateWidgetInWorkspace(WidgetType.ImageWidgetTypeID, 24, 140, 220, 40, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR, null, 9999));
		m_Button = ButtonWidget.Cast(workspace.CreateWidgetInWorkspace(WidgetType.ButtonWidgetTypeID, 24, 140, 220, 40, WidgetFlags.VISIBLE, null, 10000));
		m_ButtonText = TextWidget.Cast(workspace.CreateWidgetInWorkspace(WidgetType.TextWidgetTypeID, 35, 148, 200, 28, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR, null, 10001));
		m_StatusText = TextWidget.Cast(workspace.CreateWidgetInWorkspace(WidgetType.TextWidgetTypeID, 24, 185, 420, 70, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR, null, 10001));
		float screenWidth, screenHeight;
		workspace.GetScreenSize(screenWidth, screenHeight);
		m_MarkerRoot = FrameWidget.Cast(workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, 0, 0, workspace.DPIUnscale(screenWidth), workspace.DPIUnscale(screenHeight), WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR, null, 9999));
		if (m_MarkerRoot)
			m_Marker = TextWidget.Cast(workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS, Color.White, 0, m_MarkerRoot));
		if (!m_ButtonBackground || !m_Button || !m_ButtonText || !m_StatusText || !m_Marker)
		{
			Close();
			return;
		}
		m_Button.AddHandler(this);
		m_ButtonBackground.LoadImageTexture(0, "UI/Textures/Sights/1P29/1P29_black_solid_UI.edds");
		m_ButtonText.SetText("Launch Swarm");
		m_ButtonText.SetExactFontSize(22);
		m_ButtonText.SetColor(Color.White);
		m_ButtonText.SetShadow(2);
		m_Marker.SetText("+ SCOUT");
		m_Marker.SetExactFontSize(24);
		m_Marker.SetShadow(2);
		FrameSlot.SetSize(m_Marker, 160, 36);
		m_StatusText.SetText("Select a scout point on the map.");
		SCR_MapEntity.GetOnSelection().Insert(OnMapSelection);
		SCR_PlayerController.s_MURStatusChanged.Insert(OnStatusChanged);
		GetGame().GetCallqueue().CallLater(UpdateMarker, 100, true);
		Refresh();
	}

	void Close()
	{
		SCR_MapEntity.GetOnSelection().Remove(OnMapSelection);
		SCR_PlayerController.s_MURStatusChanged.Remove(OnStatusChanged);
		GetGame().GetCallqueue().Remove(UpdateMarker);
		if (m_ButtonBackground) m_ButtonBackground.RemoveFromHierarchy();
		if (m_Button) m_Button.RemoveFromHierarchy();
		if (m_ButtonText) m_ButtonText.RemoveFromHierarchy();
		if (m_StatusText) m_StatusText.RemoveFromHierarchy();
		if (m_MarkerRoot) m_MarkerRoot.RemoveFromHierarchy();
		m_ButtonBackground = null;
		m_Button = null;
		m_ButtonText = null;
		m_StatusText = null;
		m_Marker = null;
		m_MarkerRoot = null;
	}

	protected void OnMapSelection(vector screenPosition)
	{
		if (!m_Map) return;
		float x, z;
		float screenY = screenPosition[1];
		if (Math.AbsFloat(screenPosition[2]) > 0.5) screenY = screenPosition[2];
		m_Map.ScreenToWorld(screenPosition[0], screenY, x, z);
		m_vScoutPoint = Vector(x, GetGame().GetWorld().GetSurfaceY(x, z), z);
		m_bHasScoutPoint = true;
		m_Marker.SetVisible(true);
		UpdateMarker();
		if (m_StatusText) m_StatusText.SetText(string.Format("Scout point: %1", m_vScoutPoint));
		Refresh();
	}

	protected void UpdateMarker()
	{
		if (!m_Map || !m_Marker || !m_bHasScoutPoint) return;
		int x, y;
		m_Map.WorldToScreen(m_vScoutPoint[0], m_vScoutPoint[2], x, y, true);
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		FrameSlot.SetPos(m_Marker, workspace.DPIUnscale(x) - 8, workspace.DPIUnscale(y) - 16);
	}

	protected void OnStatusChanged(string message, bool active)
	{
		if (m_StatusText) m_StatusText.SetText(message);
		Refresh();
	}

	protected void Refresh()
	{
		if (!m_Button || !m_Controller) return;
		bool hasController = SCR_PlayerController.MUR_HasController(m_Controller.GetControlledEntity());
		if (!hasController && m_StatusText)
			m_StatusText.SetText("Equip a drone controller to launch.");
		else if (m_Controller.MUR_IsActive() && m_StatusText)
			m_StatusText.SetText(m_Controller.MUR_GetStatus());
		bool canLaunch = hasController && m_bHasScoutPoint && !m_Controller.MUR_IsActive();
		m_Button.SetEnabled(canLaunch);
		if (m_ButtonText)
		{
			if (canLaunch) m_ButtonText.SetColor(Color.White);
			else m_ButtonText.SetColor(new Color(0.75, 0.75, 0.75, 1));
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w != m_Button || !m_Controller || !m_bHasScoutPoint) return false;
		m_Controller.MUR_RequestLaunch(m_vScoutPoint);
		Refresh();
		return true;
	}
}

modded class SCR_MapEntity
{
	protected ref MUR_SwarmMapUi m_MURSwarmUi;

	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		GetGame().GetCallqueue().CallLater(MUR_OpenSwarmUi, 100, false);
	}

	protected void MUR_OpenSwarmUi()
	{
		if (!IsOpen()) return;
		if (m_MURSwarmUi) m_MURSwarmUi.Close();
		m_MURSwarmUi = new MUR_SwarmMapUi();
		m_MURSwarmUi.Open(this);
	}

	override void OnMapClose()
	{
		if (m_MURSwarmUi) m_MURSwarmUi.Close();
		m_MURSwarmUi = null;
		super.OnMapClose();
	}
}
