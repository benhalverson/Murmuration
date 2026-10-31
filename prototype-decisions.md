# Murmuration prototype: decisions

## Agreed

- A player selects a scout point on the map; drones fly autonomously.
- The first milestone uses three drones, launched from a fixed point near the player, to search, report enemy players and vehicles with last-seen markers, and return.
- Test on a private dedicated server controlled by the user, with the user joining as a player.
- A local Ollama model assigns distinct search routes using the scout point, drone positions, and targets the drones have actually observed. Flight control and detection are handled by game code. Exact replanning behavior and deployment arrangement remain open.
- If Ollama is unavailable or slow, the mission waits and tells the player rather than launching on a fallback route.
- Last-seen markers are visible to the requesting player's faction and update only when a drone observes the target again.
- Prefer few or no third-party mod dependencies and use the user's VIOLET X5 design.
- Prototype search area: 500 meters around the selected point; the five-minute search starts when drones reach the area. Scout points are limited to 2 km from the launch point.
- Ollama plans at launch and is consulted again if a drone is lost or a player starts a new mission; sightings update without waiting for the model.
- The private game server and Ollama run on the user's PC for the prototype.
- Last-seen markers expire two minutes after the last observation.
- For each mission, spawn the drones on clear ground near the requesting player; return to that mission's launch point.
- Initial personnel targets are enemy human players; AI soldiers are excluded. Enemy supply trucks and other vehicles remain targets.
- A player needs a drone-controller inventory item to request their own swarm. Each controller owner may run one active mission of up to three drones, and different players may run missions simultaneously.
- If a drone is destroyed, survivors continue their routes while Ollama replans; if replanning fails, they retain their routes and return at the deadline.
- Friendly drones across players share one last-seen marker per target and faction; any new friendly observation refreshes it.
- Model input is structured game data, not drone-camera images.
- Equip the controller, open the map, click a point, and press Launch Swarm; the launch button is unavailable while that player has an active mission.
- Detect human players and vehicles within an initial 300-meter range with unobstructed line of sight; terrain and buildings block sightings. Unoccupied vehicles are marked hostile only when their faction is identifiable as hostile.
- Prioritize reliable autonomous movement and collision handling over detailed propeller physics. Drones visibly take off, fly, search and land.
- A Recall Swarm action ends the mission and sends survivors back. On the requesting player's disconnect, survivors return and despawn after landing.
- The five-minute search clock starts when the first surviving drone reaches the search area.
- A requesting player's death does not end the mission; after respawning they can view faction markers and recall their drones.
- Each new mission provides a fresh swarm of up to three drones; prototype losses do not require inventory replacement.
- Start with the installed `qwen3.5:latest` in Ollama. Allow up to 60 seconds for a valid initial plan; on failure, show an error and do not launch.

## Local inventory observed

On 2026-10-31, Ollama responded locally and listed `qwen3.5:latest`, `llava:13b`, and `laguna-xs-2.1:latest`; the system reported an NVIDIA RTX 3090 with 24 GB VRAM. This is an inventory, not an inference benchmark or proof of performance alongside Reforger.
