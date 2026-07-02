# Add Humidifier Device Type to all-devices-app

This document captures the implementation checklist for adding a Humidifier/Dehumidifier device type to `all-devices-app`.

## 1. Create a new all-devices device module

- Add a new device folder under `examples/all-devices-app/all-devices-common/devices/`.
- Implement a device class similar to:
  - `examples/all-devices-app/all-devices-common/devices/soil-sensor/SoilSensorDevice.h`
  - `examples/all-devices-app/all-devices-common/devices/soil-sensor/SoilSensorDevice.cpp`
- Set the device type entry to `Device::Type::kHumidifierDehumidifier` from generated device types.
  - Reference: `zzz_generated/app-common/devices/Types.h`
- Add a GN target in the new device folder (pattern similar to existing device `BUILD.gn` files).

## 2. Wire required clusters in Register/Unregister

At minimum, create and register:

- Identify cluster
- Humidistat cluster

Recommended optional cluster:

- Relative Humidity Measurement cluster

Relevant cluster implementations:

- `src/app/clusters/humidistat-server/HumidistatCluster.h`
- `src/app/clusters/relative-humidity-measurement-server/RelativeHumidityMeasurementCluster.h`

### Notes

- Choose Humidistat feature flags to match intended behavior (humidifier-only vs humidifier+dehumidifier, sensor, etc.).
- `matter-devices.xml` lists Water Tank Level Monitoring for this device type, but there does not appear to be a corresponding code-driven server module in this repo yet.

## 3. Register device in DeviceFactory

Update:

- `examples/all-devices-app/all-devices-common/device-factory/DeviceFactory.h`
- `examples/all-devices-app/all-devices-common/device-factory/BUILD.gn`

Changes:

- Include your new device header.
- Add `RegisterCreator("humidifier-dehumidifier", ...)`.
- Guard with new feature define, e.g. `ALL_DEVICES_ENABLE_HUMIDIFIER_DEHUMIDIFIER`.
- Add dependency for the new module in `device-factory/BUILD.gn`.

## 4. Add enable/disable plumbing (GN + CMake + generated header)

Update all three files in lockstep:

- `examples/all-devices-app/all-devices-common/device-factory/enabled_devices.gni`
- `examples/all-devices-app/all-devices-common/device-factory/enabled_devices.cmake`
- `examples/all-devices-app/all-devices-common/device-factory/enabled_devices_config.h.in`

Required changes:

- Add the new key `humidifier-dehumidifier`.
- Add generated define handling for `ALL_DEVICES_ENABLE_HUMIDIFIER_DEHUMIDIFIER`.
- Add the new `.cpp` files to `ALL_DEVICES_DEVICE_SOURCES` in CMake.

## 5. Expose target variants in build target generator

Update:

- `scripts/build/build/targets.py`

Add `humidifier-dehumidifier` to `_ALL_DEVICES_APP_DEVICES`.

This enables single-device variants for host and ESP32 target generation.

## 6. Update docs

Update supported device list and examples in:

- `examples/all-devices-app/README.md`

Optionally document any missing optional cluster support.

## 7. Build and validate

Build host target:

```bash
scripts/run_in_build_env.sh "./scripts/build/build_examples.py --target linux-x64-all-devices-clang build"
```

Check help output includes the new key:

```bash
./out/linux-x64-all-devices-clang/all-devices-app --help
```

Run app with the new type:

```bash
./out/linux-x64-all-devices-clang/all-devices-app --device humidifier-dehumidifier:1
```

Validate behavior via `chip-tool` Humidistat reads/commands.

## 8. Add tests

Recommended test areas:

- Device lifecycle: create/register/unregister
- Cluster state startup and feature-map behavior
- Command handling for Humidistat `SetSettings`

Existing Humidistat unit tests are in:

- `src/app/clusters/humidistat-server/tests/TestHumidistatCluster.cpp`
