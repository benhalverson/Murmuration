# Autotest

To run the current suite from the diagnostic game executable, point
`-addonsDir` at the directory containing the `murmuration` checkout:

```powershell
ArmaReforgerSteamDiag.exe -addonsDir "<directory containing murmuration>" -addons A524B9D4F8038E71 -autotest MUR_FlightRouteSuite -window -noThrow
```

Replace the placeholder with the actual parent directory. The suite reports to
the run's `autotest.log` under the active profile's `logs` directory (normally
`Documents/My Games/ArmaReforger/logs`). The
game may first enter the main menu before loading the test scenario. The addon
must be listed with `-addons`: using `-gproj` alone loaded the scripts initially
but lost the addon when the runner switched scenarios.

The suite checks waypoint state changes, prefab spawning, a launch near a
controlled entity used as a requester fixture, rejection of an out-of-range
scout point, a spawn far below the requester, and selection of another launch
position when a crate blocks the first corridor, plus short flights with a clear path and with one crate
in the diagnostic test world. The flight obstacle test checks that the crate blocks a
trace, then checks that the drone climbs and returns. It does not establish
general obstacle navigation, a real player launch, or complete multiplayer
behavior.

## Workbench

Open a world and the Murmuration project in Workbench. In Script Editor, open
`Scripts/Game/Murmuration/Tests/MUR_FlightRouteTest.c`, put the cursor inside
the desired test class, and choose **Plugins → Run test** (F4). To run the
suite, open `MUR_FlightRouteSuite.c` and put the cursor inside the suite class.
The test runner opens the World Editor and writes `autotest.log` to the active
Workbench profile's `logs` directory. See the official [Autotest Framework
documentation](https://community.bistudio.com/wiki/Arma_Reforger:Autotest_Framework)
for the Workbench and command-line runners.

On Workbench 1.8.0.13, an earlier revision of the suite ran nine tests
successfully, including the route-state and clear-flight tests; the obstacle
flight timed out after 30 seconds. A separate run of that obstacle test also
timed out. This is evidence of a Workbench limitation or failure in that
environment, not proof of its cause. On game diagnostic 1.9.0.37, the current
suite's 12 tests passed via `-autotest`; that result does not establish that the
same suite passes in Workbench.

Launch selection checks the full takeoff corridor. If an obstacle blocks a
flight and no climb is available, the flight stops at its current position and
reports failure; it does not automatically land or return.

The VIOLET mesh was exported from the original CAD assembly via STEP and STL,
then imported into Workbench as an FBX. This conversion merges the assembly into
one mesh and does not preserve the CAD materials or animate propellers. Its
appearance and scale have not been assessed in detail.

## Dedicated-server experiment

The following diagnostic commands start an opt-in test launch when a player
spawns, after a ten-second delay. Set the paths for the installed game and
server, and use an isolated profile for each process. The example uses a scout point 20 metres from the
player along world X. `-murTestScout` is a test-only offset, not the map UI.
Add `-murTestObstacle 1` to the server command to place a test crate halfway
along the outbound path at the planned flight altitude and sets the time to
noon when the player deploys. The server verifies that the crate is the first
trace hit on the route and reports flight completion and peak altitude. This option
is for a controlled test; verify the crate and the flight on the client.

```powershell
ArmaReforgerServerDiag.exe -server "worlds/MP/CTI_Campaign_Eden.ent" -addonsDir "<directory containing murmuration>" -addons A524B9D4F8038E71 -murTestScout "20 0 0" -profile "<temporary server profile>" -maxFPS 30 -noThrow
ArmaReforgerSteamDiag.exe -client 127.0.0.1 -addonsDir "<directory containing murmuration>" -addons A524B9D4F8038E71 -profile "<temporary client profile>" -window -noThrow
```

Start the client after the server log reports `Starting RPL server`, then
select a faction and spawn. The server log records whether the test drone
spawned. Observe its takeoff, flight, return, and landing from the client;
repeat with an obstacle on the route. Stop the test processes afterward.

The diagnostic server was observed listening on `0.0.0.0:2001`; restrict access
to the test network before running it. On game version 1.9.0.37, a client
connected to the Conflict world and a player spawned, but the first test launch
was rejected because its takeoff corridor was blocked. After adding a search
for nearby positions, a later server run reported a successful drone spawn at a
different player spawn location. With the delayed test launch, the player
confirmed from the connected client that the drone took off, flew to the scout
point, returned, and landed. In an obstacle test, the server confirmed the crate
was the first trace hit, then reported a completed flight with a peak altitude
of 57.2321 m against a planned altitude of 52.2322 m. The player confirmed that
both the crate and the drone's flight were visible on the connected client.
A separate temporary instrumented probe spawned a
drone in the air after a client connected; the client logged proxy positions
changing from `<3819,350.133,5436>` to `<3819,350.372,5436>`. This establishes
that some position updates reached the proxy, not that a player saw a complete
flight. The temporary probe was removed. The smaller `MPTest` and Autotest
worlds failed to load on the client in this setup.
