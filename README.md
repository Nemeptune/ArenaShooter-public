# ArenaShooter

A fast, first-person multiplayer arena shooter built in **Unreal Engine 5.8** with C++ and the **Gameplay Ability System**. Two players duel over Steam or LAN with five weapons, rocket jumping, pickups and buff powerups.

The project is mostly about **netcode that feels right on a real connection**. The client predicts weapon swaps, shots, ammo and projectiles so there's no visible input delay, and the server stays authoritative over damage, deaths and match results.

Clickable image with link to youtube video.

[![ArenaShooter gameplay video](https://img.youtube.com/vi/I6yaiGGQ12A/maxresdefault.jpg)](https://www.youtube.com/watch?v=I6yaiGGQ12A)

> This repository contains the C++ source, config, plugins and all Blueprint / logic content (abilities, effects, anim blueprints, UI, materials, Niagara systems, MetaSounds, data assets, maps). Third-party art (textures, meshes, animations, sound waves) is not included. See [What's in this repo](#whats-in-this-repo).

---

## Highlights

### Predicted projectiles: fast-forwarded, synchronized and resimulated
`AASProjectile`, `UASAT_SpawnPredictedProjectile`

Rockets are real simulated projectiles, not hitscan. Each machine sees them differently:

- **Owning client:** a *fake* projectile spawns the frame you fire and is **fast-forwarded** by your ping (`GetForwardPredictionTime`, scaled by a client-bias factor) so it's roughly where the server's copy is.
  - Prediction is capped (`MaxPredictionPing`). On high ping the fake spawn is delayed just enough that the client never extrapolates too far.
- **Linking:** the fake and the authoritative projectile share a `ProjectileId`. When the server projectile replicates, the client links them and **catches up / lerps** the fake toward the authoritative position.
- **Missed predictions** are reconciled explicitly:
  - The fake detonated but the real one didn't → switch to the authoritative projectile after a timeout.
  - The fake detonated in the wrong place (>1 m off) → replay the detonation at the authoritative location.
- **Other clients:** the projectile is **rewound to its replicated spawn transform and resimulated** locally, so it never pops into existence ahead of the shooter's muzzle.
  - Their local hit detection is unreliable (they're behind), so detonation waits for the server's replicated `FDetonationInfo`. A timer is extrapolated from the remaining distance so the explosion lands when the resim arrives.
  - A projectile that detonates on spawn is resimulated at a reduced speed so it's always visible for `MinLifetime`.
- **Tear-off:** the server tears the projectile off after a short delay so the initial state is guaranteed to replicate first.
- **Tunable:** per-projectile options for predicted FX, bounce limits, straighten-on-bounce and replicated final position.
- **Debugging:** an editor-only debug mode draws server vs. client trajectories step by step.

### Predicted weapon switching (slot index + sequence number)
`UASInventoryComponent`, `UASEquipmentComponent`

A weapon swap is instant on the owning client and still converges to the server:

- **Replicated state:** the inventory replicates one `FASActiveSlotState { SlotIndex, Seq }`. The owner keeps a local `PredictedSlot` and `LocalSeq`.
- **Request:** `RequestSwitch` bumps the sequence, applies the swap locally right away and sends `ServerSetActiveSlot(Slot, Seq)`.
- **Reconciliation:** the owner only accepts a replicated slot whose `Seq` matches its latest request, so stale acks from fast key presses never snap the weapon back. Simulated proxies simply follow the replicated index.
- **Ordering:** swap and fire RPCs go through the same actor channel. Reliable ordering then guarantees the server processes the swap before a shot fired right after it.
- **Ability grants:** abilities are granted when a weapon is acquired, not when it's equipped. The activation gate asks the weapon instance whether it's active: predicted on the owner, replicated elsewhere. No spec replication round trip per swap.

### Weapons as data: instances, cosmetics and a clean ownership split
- **`UASWeaponInstance`:** a replicated subobject UObject holding ammo, fire timing and the definition. It lives in the inventory on the PlayerState, so it survives death and knows nothing about pawns.
- **`UASEquipmentComponent`:** on the pawn, not replicated. It *observes* the inventory and spawns a local `AASWeaponCosmetic` on each machine (skipped on dedicated servers), links per-weapon anim layers and runs equip montages and the swap lock.
- **`UASWeaponDefinition`:** data assets define abilities, anim layers, ammo type and starting ammo.
- **Cheap updates:** every change runs one total, idempotent refresh instead of a delta update. This removed a whole class of "arrived in the wrong order" bugs.

### Predicted ammo without gameplay effects
- **Storage:** ammo is one replicated `int32` per weapon instance (owner-only).
- **Cost hooks:** the fire ability overrides `CheckCost` / `ApplyCost` directly.
  - The server consumes ammo.
  - The owning client counts pending shots, so the HUD ticks down instantly and pending shots are retired as replicated values arrive.

### Weapons
| Weapon | Model |
|---|---|
| Pistol, Rifle | Predicted hitscan through GAS target data |
| Shotgun | Multi-pellet trace. All pellet hits travel in **one** gameplay cue via a custom `FASGameplayEffectContext`, and damage is coalesced per target per shot (one number, one proc, one death check) |
| Railgun | Held-fire beam (`WaitInputRelease`): server damage ticks, **lag-compensated** (see below), plus a looping cue actor that traces locally on every client for a latency-free beam end |
| Rocket launcher | The predicted projectile above, with radial falloff, line-of-sight checks and knockback |

### Rocket jumping and knockback
`UKnockbackStatics`, `UASCharacterMovementComponent::AddDampedImpulse`

- **Momentum:** explosions build an impulse from `FMomentumParams` (inner / outer radius falloff, self-damage scaling), damped against the current velocity.
- **Authority:** knockback is **server-authoritative** with a forced client correction. Predicting it was designed and measured, then rejected on purpose: it turns a small constant delay into a rare false start followed by a yank.

### Lag compensation and multithreaded hit tests
`UASLagCompensationSubsystem`, `ASHitboxMath`, `UASCharacterMovementComponent`, `AS.Stress.LagComp`

The railgun is traced on the server, which sees targets about one round trip later than the shooter did. At 150 ms RTT against a strafing target, the server was 90 cm off and hit **6.4%** of the time. The fix is to rewind every other character to the moment the shooter saw on screen, then trace.

**Why multithreading.** In a 1v1 it isn't needed. A hit test costs about 3 µs per ray and real play produces 1–3 rays per call. But lag compensation is the part of a shooter server that grows with player count: every shot tests every character. So the hit test was written from the start as a pure function over plain data, made parallel, and **measured**, to find where threads pay off and where they don't.

**How it works**
- **History:** at the end of every server frame, each character's hit capsules (from its physics asset, with the zone materials used for headshots) are saved into a ~350 ms ring buffer. It's plain data: no UObjects, safe to read from worker threads.
- **Exact rewind time:** the server stamps every character update with its world time. The client stamps every movement packet with the server time of the frame on its screen. The aim travels in the same packet, so the server rewinds to exactly the moment the shooter saw, with the aim that went with it. There's no ping estimate to drift or spike.
- **Hit test in three phases:**
  1. **Game thread:** snapshot every character's capsules at the rewound time, interpolated between the two frames around it.
  2. **`ParallelFor` over the rays:** each ray does a world trace and then the capsule tests, and writes only its own result slot, so nothing is locked. World traces are read-only scene queries, which the engine's own async traces also run on worker threads.
  3. **Game thread:** turn capsule hits into `FHitResult`s for the damage code.
- **Broad phase:** each character gets a bounding sphere. A ray only tests a character's capsules if it enters the sphere before the best hit so far.
- **Small batches stay inline:** batches under `MinRaysPerTask` (8) run on the game thread, where scheduling would cost more than it saves. Dedicated servers run `ParallelFor` single-threaded unless launched with `-useperfthreads`.

**Accuracy:** railgun vs a strafing target, 150 ms emulated RTT, perfect client aim (`AS.Test.AutoAim`)

| Rewind | 600 cm/s | 1500 cm/s |
|---|---|---|
| None | 6.4% | — |
| By averaged ping | 98.6% | 82.6% |
| **By the client's frame timestamp** | **99.8%** | **98.0%** |

Adding just 17 ms to the rewind drops the 1500 cm/s result to 29%, so the timestamp is accurate to a few milliseconds. The remaining misses happen only where the target reverses direction and the client keeps drawing it moving the old way for a moment.

**Performance:** `AS.Stress.LagComp`, Ryzen 5 3600 (6 cores / 12 threads), standalone Development build. Times are the median per batch.

| 100 characters, capsule tests | 1 ray | 17 rays | 1,000 rays | 10,000 rays |
|---|---|---|---|---|
| Every capsule, single-threaded | 28.5 µs | 487 µs | 29.2 ms | 294 ms |
| Broad phase, single-threaded | 1.2 µs | 21.8 µs | 1.75 ms | 17.6 ms |
| Broad phase + `ParallelFor` | 1.2 µs | 10.1 µs | **0.26 ms** | **2.5 ms** |
| **Combined gain** | ×24 | ×48 | **×113** | **×116** |

| Full hit test (world trace + capsules), single-threaded → `ParallelFor` | 1 ray | 4 rays | 17 rays | 100 rays | 1,000 rays |
|---|---|---|---|---|---|
| 2 characters | ×0.8 | ×1.6 | ×2.0 | ×4.0 | ×5.6 |
| 100 characters | ×1.0 | ×1.2 | ×2.3 | ×4.4 | ×6.0 |

What the numbers say:
- **Algorithm first, threads second.** The broad phase alone cut capsule work 17×. Threads added about ×7 on top of it.
- **Pure maths scales past the core count:** ×7.0 on 6 cores at 10,000 rays, because Hyper-Threading adds a bit when nothing is shared. Rays with world traces stop at ×5.5–6.1, because every ray queries the same physics scene.
- **Small batches don't benefit.** A single ray is no faster through `ParallelFor`. Real play (82% of calls are 1 ray) stays on the game thread, and a full 17-ray fan costs about 0.05 ms of a 16.7 ms frame. At 100 characters and 1,000 rays per frame, the same code takes **4.0 ms on one thread and 0.67 ms threaded**.

### Weapon FX that scale
- **Fire cues:** a single persistent, **retriggerable GameplayCue actor per pawn** (`AASGameplayCueNotify_WeaponFireActor`).
  - It keeps resident muzzle, tracer and shell Niagara components and flips a trigger parameter per shot, instead of spawning actors.
  - When idle, it winds down and returns to the GameplayCue recycle pool.
- **Impacts and decals:** written in one batch per shot to a **Niagara Data Channel** (Islands) from C++ (`UImpactStatics::ReportImpacts`), dispatched by physical surface type.
  - They're rebuilt locally from target data the fire cue already replicated, so impacts cost **zero extra RPCs**.
- **Surface-aware footsteps:** `UASAnimNotify_Footstep` + `UASSurfaceSoundSet`.
- **Damage numbers:** batched into one client RPC per frame and rendered by a Niagara-driven number-pop subsystem.

### Combat, buffs and death
- **Health and shields:** `UASCombatAttributeSet`. All damage runs through one execution, `UASDamageExecution`.
- **Passive buffs** (Strength, Resistance) are attributes captured by that execution: new buffs are data only.
- **Reactive buffs** (Vampiric, Reflection) are abilities granted for the buff's lifetime. They trigger on `Event.Damage.Dealt` / `Event.Damage.Taken`, and reflected damage is tagged so it can't ping-pong.
- **Death:** a server-only `GA_Death` ability triggered by a gameplay event.
  - It holds the dead tag across unpossess/respawn (the ASC lives on the PlayerState).
  - Death sets a replicated `bIsDead` that ragdolls on every client. The GameMode owns cancellable respawn timers.
- **Pickups:** `AASPickup` base with a GameplayEffect subclass (health, shield, buffs as data-only Blueprints) and an ammo subclass routed by ammo type tag.

### Match flow
- **Duel game mode:** warmup, timer, kill tracking and tie detection.
- **Result screen:** the result replicates through `AASGameState` (`bMatchEnded` + `WinnerPS`, so a null-winner draw still fires).

### UI architecture
- **Layered CommonUI:** `UASUILayerManager` manages Game / GameMenu / Menu / Modal stacks.
  - The root layout lives as long as the LocalPlayer and is re-parented on each new PlayerController, so UI survives hard travel without world comparisons.
- **MVVM HUD:** gameplay → binder → view model → widget.
  - Focused view models (vitals, score, per-slot hotbar) are fed by binders that *subscribe then pull*. `RefreshAll` renders correctly from any partially replicated state, so late-joining clients never get a stale HUD.
- **Message bus:** discrete events (match end, inventory, slot changes) go over a gameplay message bus, so gameplay code never calls into UI.
- **Settings screen:** built on the GameSettings plugin, with audio volumes driven by **AudioModulation control buses** and submix routing.
- **Steam avatars:** in the scoreboard, requested from Steamworks with polling and retry, keyed off `OnSetUniqueId`.

### Online sessions
`Plugins/MultiplayerSessions`

- **Session layer** on top of OnlineSubsystem, for Steam lobbies and LAN:
  - host, browse and join, with travel owned by the subsystem;
  - a view-model front end;
  - per-machine session-handle tracking, with a single `LeaveSessionAndTravel` path shared by voluntary quit and network failure, so players can re-host immediately after any disconnect.

### Loading and preloading
- **Loading screen:** held during travel by the CommonLoadingScreen plugin.
- **Preload:** `UASPreloadSubsystem` warms gameplay cues, Niagara PSOs and pooled MetaSound operators, and keeps the loading screen up until PSO precaching finishes. This avoids first-shot hitches.

---

## Built with
- **Unreal Engine 5.8**, C++
- **Engine plugins:** GameplayAbilities, EnhancedInput, CommonUI, ModelViewViewModel, Niagara (Data Channels), AudioModulation, OnlineSubsystemSteam
- **Plugins included in this repo:**
  - `MultiplayerSessions`: my session plugin (see above)
  - `GameSettings`, `CommonLoadingScreen`, `GameplayMessageRouter`: Epic Games plugins from the **Lyra Starter Game** sample, © Epic Games, used under the Unreal Engine EULA

## Getting started
1. Install **Unreal Engine 5.8** and **Visual Studio 2022** (the `.vsconfig` lists the required workloads).
2. Clone the repository.
3. Right-click `ArenaShooter.uproject` → **Generate Visual Studio project files**.
4. Open `ArenaShooter.sln` and build **ArenaShooterEditor / Development Editor**.

The project compiles and opens in the editor with every Blueprint, ability, widget, material and Niagara graph readable. Art isn't included, so the Output Log lists missing-asset warnings, and in-game the level is greybox, characters and weapons are invisible and sounds are silent. Gameplay videos and a packaged build are the way to see it running.

> Don't resave content in a fresh clone. Saving an asset permanently clears its references to the missing art.

### Source layout
```
Source/ArenaShooter/
  AbilitySystem/   ASC, globals, effect context, ability sets
    Abilities/       weapon, beam, projectile, buff and death abilities
    AbilityTasks/    predicted projectile spawn, montage-for-mesh
    Attributes/      health / shield / combat attributes
    Executions/      damage execution
    GameplayCues/    weapon fire, beam and buff aura cue actors
  Character/       character, movement component (knockback, predicted dash and bunny hop), anim instance
  Inventory/       inventory (predicted slot switching), equipment, inventory messages
  Weapon/          weapon definition, instance, cosmetic actor, projectile, lag compensation, hitbox maths
  Pickups/         pickup base, effect / ammo / buff pickups
  GameModes/       game mode, duel mode, game state
  Player/          player controller, player state, local player
  Physics/         physical material, knockback statics
  FX/              impact reporting (Niagara Data Channels), footsteps
  Messages/        gameplay message bus tags and payloads
  UI/              layer manager, HUD controller, binders, view models
  Settings/        game settings registry, user and audio settings
  Online/          Steam avatars
  Input/           input config and component
  System/          native gameplay tags, log channels, preload subsystem, profiling markers, test and stress console commands
```

### Testing multiplayer
- **LAN (one machine):** set `DefaultPlatformService=Null` in `Config/DefaultEngine.ini` and start two standalone instances (`UnrealEditor.exe ArenaShooter.uproject -game -windowed`). Host from one, join from the other.
- **Steam (two machines):** set `DefaultPlatformService=Steam`, package, and run on two PCs with different Steam accounts. Uses the Spacewar test AppId (480).

## What's in this repo
- **Included:** every content asset the game actually uses that isn't raw art: Blueprints, gameplay abilities and effects, anim blueprints and linked layers, skeletons and physics assets, widgets, materials and material functions, Niagara systems / emitters / modules and the impact data channel, MetaSounds and the audio mix / submix setup, input actions, weapon / buff / UI data assets, and both maps.
- **Not included: art.** Textures, static and skeletal meshes, animation sequences, montages and sound waves. Much of it comes from Epic / Fab sample and marketplace packs whose licenses don't allow public redistribution.
- **Engine plugin fork.** The shipped build uses a locally patched copy of `OnlineSubsystemSteam` (one change: Steam lobby search uses the worldwide distance filter). It isn't included because it's engine code, and the stock engine plugin works for same-region testing.

## Credits
- Lyra-derived plugins as listed under *Built with*.
