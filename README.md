![ARCH](https://shields.microej.com/endpoint?url=https://repository.microej.com/packages/badges/arch_7.14.json)

# Abstraction Layer for the MicroEJ Core Engine over Micrium OS

# Overview

Low Level MicroEJ Core Engine API implementation over Micrium OS.

This component implements the `LLMJVM` Low Level API for MicroEJ platforms connected to a Board Support Package based on [Micrium OS](https://www.silabs.com/developers/micrium-os).

See the MicroEJ documentation for a description of the `LLMJVM` functions:
- [LLMJVM: MicroEJ Core Engine](https://docs.microej.com/en/latest/PlatformDeveloperGuide/appendix/llapi.html#llmjvm-microej-core-engine)
- [MicroEJ Core Engine: Implementation](https://docs.microej.com/en/latest/PlatformDeveloperGuide/coreEngine.html#implementation)

# Usage

1. These sources can be included in the VEE Port with the method you prefer, by using this repository as a submodule or by doing a copy of the sources in the VEE Port repository.

2. Implement the MicroEJ time functions, as described in [microej_time.h](./src/main/c/inc/microej_time.h).

3. The `LLMJVM_IMPL_scheduleRequest` schedule request function in [LLMJVM_MicriumOS.c](./src/main/c/src/LLMJVM_MicriumOS.c) uses a software timer. In order to correctly schedule MicroEJ threads, check the following elements in the MicriumOS configuration file:
 - `OS_CFG_TICK_EN`: needs to be enabled to get access to all time-based services in Micrium OS Kernel. The `OS_ConfigureTickTask` function can be used to configure the tick task stack size and priority.
  The `OS_ConfigureTickRate` function can be used to set the OS tick rate. The MicroEJ task should have a lower priority than the tick task.
 - `OS_CFG_TMR_EN`: needs to be enabled to get access to all software-based timers in Micrium OS Kernel. The `OS_ConfigureTmrTask` function can be used to configure the timer task stack size and priority. 
  The `OSTmrCreate` function can be used to create a timer with the desired periodicity.  The MicroEJ task should have a lower priority than the timer task.

# Requirements

None.

# Validation

This Abstraction Layer implementation can be validated in the target Board Support Package using the [MicroEJ Core Validation](https://github.com/MicroEJ/PlatformQualificationTools/tree/master/tests/core/java/microej-core-validation) Platform Qualification Tools project.

Here is a non exhaustive list of tested environments:
- Hardware
  - Silicon Labs EFR32BG22
- Compilers / development environments:
  - IAR Embedded Workbench 8.50.6
- Micrium OS versions:
  - 5.9.0

## MISRA Compliance

This Abstraction Layer implementation is MISRA-compliant (MISRA C:2012) with some noted exception.
It has been verified with Cppcheck v2.13. Here is the list of deviations from MISRA standard:

| Deviation | Category |                                                 Justification                                                 |
|:---------:|:--------:|:-------------------------------------------------------------------------------------------------------------:|
|  Rule 5.5 | Required | Intentional usage of macros to define native functions.                                                       |
|  Rule 8.4 | Required | A compatible declaration is defined in headers provided by the VEE Port.                                      |
|  Rule 8.7 | Advisory | The `LLMJVM_IMPL_*` functions are the LLAPI of the Core Engine. Only the Core Engine calls them, and the analysis does not see it. |
| Rule 11.4 | Advisory | `LLMJVM_IMPL_getCurrentTaskID()` returns the address of the current Micrium OS task control block as the `int32_t` task identifier of the LLAPI. |


# Dependencies

- MicroEJ Architecture ``7.x`` or higher.
- Micrium OS ``5.9.0`` or higher.

# Source

N/A.

# Restrictions

None.

---
_Copyright 2021-2026 MicroEJ Corp. All rights reserved._
_Use of this source code is governed by a BSD-style license that can be found with this software._
_Build: 7E4D1F7C_
