## 1. Known Limitations & Roadmap
| Area                            | Current State                                   | Planned                                                                   |
|---------------------------------|-------------------------------------------------|---------------------------------------------------------------------------|
| Collision                       | Floor-plane only *(y = 0)*                      | Ray-cast vs mesh AABB tree                                                |
| Shadows                         | None                                            | Single directional shadow map                                             |
| Audio                           | Not implemented                                 | OpenAL-Soft or mini audio integration                                     |
| Level loading                   | Hardcoded test level                            | JSON / binary scene format                                                |
| Animation                       | None                                            | Skeletal animation via Assimp                                             |
| Vaulting                        | State stub only                                 | Auto-climb ledges ≤ 2 m high                                              |
| Texture system                  | Stub uniforms                                   | Full material system with normal maps                                     |
| Icons for Crate items           | None                                            | make the icons in pixel art and implement in UI                           |
| Inventory                       | Print to console                                | Diegetic UI *(Deus Ex & Arc Raiders style)*                               |
| AI *(Enemies & NPCs)* / stealth | Not implemented                                 | Visibility cones + noise detection + random movement + patrols            |
| Weapons                         | Not implemented                                 | Blunt + Slash + Guns*(Pistol, Shotgun, Sniper, Semi-Auto) *               |
| Ammo System                     | Not implemented *(Needs Weapon Implementation)* | Pistol*(9mm)*, Shotgun*(12 Gauge)*, Sniper*(7.62mm)*, Semi-Auto*(5.56mm)* |
| Health System                   | Not implemented                                 | Health Bar and death screen/level reload                                  |
| Networking                      | Not planned                                     | No. Will think about it if this gets on steam                             |

# 2. List of current issues/bug
| Bug                                                                                                   | Current State | Potential Fix             |
|-------------------------------------------------------------------------------------------------------|---------------|---------------------------|
| Duplication of items on level load & pickup item naming *(the light duplicate)*                       | In Progress   | Check Level Load Function |
| Need to check if crouch properly resizes the player collision                                         | Planned       | Check it                  |
| Need to press interaction twice to pickup item                                                        | Planned       | Check it                  |
| Door resize instead of pivoting and need to press interaction key multiple times for the door to open | Planned       | Check it                  |
