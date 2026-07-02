# Humidifier / Dehumidifier Device

Simulated Humidifier / Dehumidifier device for the `all-devices-app`.

## Mode State Transitions

The simulation loop runs every 5 seconds and advances `mSimulatedHumidity`
by 2 % per tick.  The behaviour depends on the active **Mode**:

| Mode | Direction | Stops when… | Resulting state |
|---|---|---|---|
| `kOff` | Auto-detects drift | Stays `kOff` only when humidity == setpoint | Wakes into `kHumidifier` or `kDehumidifier` if humidity has drifted |
| `kHumidifier` | Humidifies only | Setpoint reached | Mode → `kOff`, `SystemState = kIdle` |
| `kDehumidifier` | Dehumidifies only | Setpoint reached | Mode → `kOff`, `SystemState = kIdle` |
| `kAuto` | Bidirectional | Never leaves Auto | `SystemState = kIdle` at setpoint; resumes if humidity drifts |
| `kFanOnly` | None | Immediately | `SystemState = kFan` |

### kOff (auto-wake)

The device is nominally off, but the simulation loop keeps running.  On each
tick the measured humidity is compared against the **TargetSetpoint**:

| Humidity vs setpoint | Action |
|---|---|
| Below setpoint | Mode → `kHumidifier`, `SystemState = kHumidifying` |
| Above setpoint | Mode → `kDehumidifier`, `SystemState = kDehumidifying` |
| At setpoint | Stays `kOff`, `SystemState = kOff` |

Once the woken mode reaches its setpoint it auto-offs again (mode → `kOff`,
`SystemState = kIdle`).  The cycle repeats automatically if humidity drifts
again, so `kOff` effectively acts as a hysteresis-free idle state.

### kHumidifier / kDehumidifier (one-shot)

The device operates in a single direction until the **TargetSetpoint** is
reached.  At that point the mode is automatically set to `kOff` and the system
state transitions to `kIdle`.  The device will not restart until the mode is
changed again by a client.

### kAuto (continuous regulation)

The device stays in `kAuto` indefinitely.  It humidifies when the measured
humidity is below the setpoint and dehumidifies when it is above.  When the
setpoint is exactly met the state becomes `kIdle`, but the loop continues
running; if humidity drifts away from the setpoint the device resumes in the
appropriate direction without any client intervention.

### Continuous flag

When the **Continuous** attribute is enabled:

- `kHumidifier` — the cap is raised to 100 % so the device keeps running past
  the setpoint and **never auto-offs**.
- `kDehumidifier` — the floor is lowered to 0 % with the same effect.
- `kAuto` — the cap/floor extends to 100 %/0 %, so the device oscillates
  between the extremes rather than idling at the setpoint.
