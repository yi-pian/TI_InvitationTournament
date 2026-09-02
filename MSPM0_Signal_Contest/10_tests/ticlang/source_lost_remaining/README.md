# Remaining SOURCE_LOST TI link harness

`validate.ps1` uses the existing MSPM0G3507 startup, linker command and generated SysConfig objects only as a target-link harness. It compiles all eight new modules and their two canonical dependencies, then performs a complete Cortex-M0+ link. It does not edit SysConfig and is not board verification.
